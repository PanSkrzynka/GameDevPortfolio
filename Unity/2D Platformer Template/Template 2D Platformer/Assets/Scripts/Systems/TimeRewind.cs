using System.Collections.Generic;
using UnityEngine;

[RequireComponent(typeof(Rigidbody2D))]
public class TimeRewind : MonoBehaviour
{
    private sealed class ObjectSnapshot
    {
        public IRewindable Target;
        public readonly List<object> Values = new(1);

        public void Clear()
        {
            Target = null;
            Values.Clear();
        }
    }

    private sealed class RewindFrame
    {
        public Vector2 Position;
        public Vector2 Velocity;
        public readonly List<ObjectSnapshot> Snapshots = new();
        public int SnapshotCount;

        public ObjectSnapshot CaptureSlot()
        {
            if (SnapshotCount == Snapshots.Count) Snapshots.Add(new ObjectSnapshot());
            return Snapshots[SnapshotCount++];
        }

        public void Clear()
        {
            for (int i = 0; i < SnapshotCount; i++) Snapshots[i].Clear();
            SnapshotCount = 0;
        }
    }

    [SerializeField] private InputBindings bindings;
    [SerializeField] private float rewindDuration = 3f;
    [SerializeField] private float cooldownMultiplier = 2f;
    [SerializeField] private AudioClip rewindSound;

    private static readonly List<IRewindable> Rewindables = new();

    private RewindFrame[] _history;
    private int _nextFrame;
    private int _frameCount;
    private Rigidbody2D _body;
    private bool _isRewinding;
    private float _rewindRemaining;
    private float _cooldownRemaining;
    private float _fixedDelta;

    public bool IsRewinding => _isRewinding;

    [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.SubsystemRegistration)]
    private static void ResetRegistry() => Rewindables.Clear();

    private static bool IsAlive(IRewindable target)
    {
        return target != null && (target is not Object unityObject || unityObject != null);
    }

    public static void Register(IRewindable rewindable)
    {
        if (IsAlive(rewindable) && !Rewindables.Contains(rewindable)) Rewindables.Add(rewindable);
    }

    public static void Unregister(IRewindable rewindable)
    {
        Rewindables.Remove(rewindable);
    }

    private void Awake()
    {
        _body = GetComponent<Rigidbody2D>();
        _fixedDelta = Time.fixedDeltaTime;
        EnsureHistoryCapacity();
    }

    private void EnsureHistoryCapacity()
    {
        int capacity = Mathf.Max(1, Mathf.CeilToInt(rewindDuration / _fixedDelta));
        if (_history != null && _history.Length == capacity) return;

        RewindFrame[] resized = new RewindFrame[capacity];
        int retained = Mathf.Min(_frameCount, capacity);
        for (int i = 0; i < retained; i++)
        {
            int source = (_nextFrame - retained + i + _history.Length) % _history.Length;
            resized[i] = _history[source];
        }
        for (int i = retained; i < capacity; i++) resized[i] = new RewindFrame();

        _history = resized;
        _frameCount = retained;
        _nextFrame = retained % capacity;
    }

    private void Update()
    {
        if (_cooldownRemaining > 0f) _cooldownRemaining -= Time.deltaTime;
        if (bindings == null) return;

        if (Input.GetKeyDown(bindings.Rewind) && CanRewind) BeginRewind();

        if (_isRewinding)
        {
            _rewindRemaining -= Time.deltaTime;
            if (_rewindRemaining <= 0f) _isRewinding = false;
        }

        if (Input.GetKeyUp(bindings.Rewind)) _isRewinding = false;
    }

    private bool CanRewind => _cooldownRemaining <= 0f && _frameCount > 0;

    private void BeginRewind()
    {
        _isRewinding = true;
        _rewindRemaining = rewindDuration;
        _cooldownRemaining = rewindDuration * cooldownMultiplier;
        SfxPlayer.Play(rewindSound);
    }

    private void FixedUpdate()
    {
        if (_isRewinding) StepBack();
        else Record();
    }

    private void Record()
    {
        EnsureHistoryCapacity();
        RewindFrame frame = _history[_nextFrame];
        frame.Clear();
        frame.Position = _body.position;
        frame.Velocity = _body.linearVelocity;

        for (int i = Rewindables.Count - 1; i >= 0; i--)
        {
            IRewindable rewindable = Rewindables[i];
            if (!IsAlive(rewindable))
            {
                Rewindables.RemoveAt(i);
                continue;
            }

            ObjectSnapshot snapshot = frame.CaptureSlot();
            snapshot.Target = rewindable;
            rewindable.SaveState(snapshot.Values);
        }

        _nextFrame = (_nextFrame + 1) % _history.Length;
        _frameCount = Mathf.Min(_frameCount + 1, _history.Length);
    }

    private void StepBack()
    {
        if (_frameCount == 0)
        {
            _isRewinding = false;
            return;
        }

        _nextFrame = (_nextFrame - 1 + _history.Length) % _history.Length;
        RewindFrame frame = _history[_nextFrame];
        _body.position = frame.Position;
        _body.linearVelocity = frame.Velocity;

        for (int i = 0; i < frame.SnapshotCount; i++)
        {
            ObjectSnapshot snapshot = frame.Snapshots[i];
            if (!IsAlive(snapshot.Target) || snapshot.Values.Count == 0) continue;
            snapshot.Target.LoadState(snapshot.Values.Count == 1 ? snapshot.Values[0] : snapshot.Values);
        }

        frame.Clear();
        _frameCount--;
    }

    private void OnDisable()
    {
        if (_history != null)
            foreach (RewindFrame frame in _history) frame.Clear();
        _frameCount = 0;
        _nextFrame = 0;
        _isRewinding = false;
    }

    private void OnValidate()
    {
        rewindDuration = Mathf.Max(0.1f, rewindDuration);
        cooldownMultiplier = Mathf.Max(0f, cooldownMultiplier);
    }
}
