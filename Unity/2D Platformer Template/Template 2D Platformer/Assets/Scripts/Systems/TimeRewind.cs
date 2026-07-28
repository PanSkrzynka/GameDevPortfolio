using System.Collections.Generic;
using UnityEngine;

public class TimeRewind : MonoBehaviour
{
    private class RewindFrame
    {
        public Vector2 Position;
        public Vector2 Velocity;
        public readonly Dictionary<IRewindable, object> Snapshot = new();
    }

    [SerializeField] private InputBindings bindings;
    [SerializeField] private float rewindDuration = 3f;
    [SerializeField] private float cooldownMultiplier = 2f;
    [SerializeField] private AudioClip rewindSound;

    private static readonly List<IRewindable> Rewindables = new();

    private readonly List<RewindFrame> _history = new();
    private Rigidbody2D _body;
    private bool _isRewinding;
    private float _rewindRemaining;
    private float _cooldownRemaining;
    private float _fixedDelta;

    public bool IsRewinding => _isRewinding;

    public static void Register(IRewindable rewindable)
    {
        if (rewindable != null && !Rewindables.Contains(rewindable)) Rewindables.Add(rewindable);
    }

    public static void Unregister(IRewindable rewindable)
    {
        Rewindables.Remove(rewindable);
    }

    private void Awake()
    {
        _body = GetComponent<Rigidbody2D>();
        _fixedDelta = Time.fixedDeltaTime;
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

    private bool CanRewind => _cooldownRemaining <= 0f && _history.Count > 0;

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
        int maxFrames = Mathf.CeilToInt(rewindDuration / _fixedDelta);
        if (_history.Count >= maxFrames) _history.RemoveAt(_history.Count - 1);

        RewindFrame frame = new RewindFrame
        {
            Position = _body.position,
            Velocity = _body.linearVelocity
        };

        for (int i = Rewindables.Count - 1; i >= 0; i--)
        {
            IRewindable rewindable = Rewindables[i];
            if (rewindable == null)
            {
                Rewindables.RemoveAt(i);
                continue;
            }

            List<object> buffer = new List<object>();
            rewindable.SaveState(buffer);

            if (buffer.Count == 1) frame.Snapshot[rewindable] = buffer[0];
            else if (buffer.Count > 1) frame.Snapshot[rewindable] = new List<object>(buffer);
        }

        _history.Insert(0, frame);
    }

    private void StepBack()
    {
        if (_history.Count == 0)
        {
            _isRewinding = false;
            return;
        }

        RewindFrame frame = _history[0];
        _body.position = frame.Position;
        _body.linearVelocity = frame.Velocity;

        foreach (KeyValuePair<IRewindable, object> entry in frame.Snapshot)
        {
            entry.Key?.LoadState(entry.Value);
        }

        _history.RemoveAt(0);
    }

    private void OnValidate()
    {
        rewindDuration = Mathf.Max(0.1f, rewindDuration);
        cooldownMultiplier = Mathf.Max(0f, cooldownMultiplier);
    }
}
