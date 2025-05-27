using System.Collections.Generic;
using UnityEngine;

public class TimeRewind : MonoBehaviour
{
    private class RewindFrame
    {
        public Vector2 Position;
        public Vector2 Velocity;
        public Dictionary<IRewindable, object> Snapshot = new();
    }

    [SerializeField] private float rewindDuration = 3f;
    [SerializeField] private KeyCode rewindKey = KeyCode.R;

    private List<RewindFrame> _history = new();
    private Rigidbody2D _rb;
    private IRewindable[] _rewindables;
    private bool _isRewinding;
    private bool _isOnCooldown;
    private float _cooldownRemaining;
    private float _fixedDelta;

    private static readonly List<IRewindable> RegisteredRewindables = new();

    public static void Register(IRewindable rewindable)
    {
        if (!RegisteredRewindables.Contains(rewindable))
            RegisteredRewindables.Add(rewindable);
    }

    public static void Unregister(IRewindable rewindable)
    {
        RegisteredRewindables.Remove(rewindable);
    }
    
    private void Awake()
    {
        _rb = GetComponent<Rigidbody2D>();
        _fixedDelta = Time.fixedDeltaTime;
    }

    private float _rewindTimeRemaining;

    private void Update()
    {
        if (_isOnCooldown)
        {
            _cooldownRemaining -= Time.deltaTime;
            if (_cooldownRemaining <= 0f)
                _isOnCooldown = false;
        }

        if (Input.GetKeyDown(rewindKey) && !_isOnCooldown)
        {
            _isRewinding = true;
            _rewindTimeRemaining = rewindDuration;
            _isOnCooldown = true;
            _cooldownRemaining = rewindDuration*2;
        }

        if (_isRewinding)
        {
            _rewindTimeRemaining -= Time.deltaTime;
            if (_rewindTimeRemaining <= 0f)
                _isRewinding = false;
        }

        if (Input.GetKeyUp(rewindKey))
        {
            _isRewinding = false;
        }
    }

    private void FixedUpdate()
    {
        if (_isRewinding) Rewind();
        else Record();
    }

    private void Record()
    {
        if (_history.Count > Mathf.CeilToInt(rewindDuration / _fixedDelta))
            _history.RemoveAt(_history.Count - 1);

        var frame = new RewindFrame
        {
            Position = _rb.position,
            Velocity = _rb.velocity
        };

        foreach (var obj in RegisteredRewindables)
        {
            var buffer = new List<object>();
            obj.SaveState(buffer);
            if (buffer.Count == 1) frame.Snapshot[obj] = buffer[0];
        }

        _history.Insert(0, frame);
    }

    private void Rewind()
    {
        if (_history.Count == 0)
        {
            _isRewinding = false;
            return;
        }

        var frame = _history[0];
        _rb.position = frame.Position;
        _rb.velocity = frame.Velocity;

        foreach (var pair in frame.Snapshot)
        {
            pair.Key.LoadState(pair.Value);
        }

        _history.RemoveAt(0);
    }
}