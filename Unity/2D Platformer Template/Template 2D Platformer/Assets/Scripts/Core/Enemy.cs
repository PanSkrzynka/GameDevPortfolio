using System.Collections.Generic;
using UnityEngine;

public class Enemy : MonoBehaviour, IRewindable
{
    [SerializeField] private Transform pointA;
    [SerializeField] private Transform pointB;
    [SerializeField] private float speed = 2f;
    [SerializeField] private float arrivalDistance = 0.05f;

    private Vector2 _pointA;
    private Vector2 _pointB;
    private Vector2 _target;

    private struct EnemyState
    {
        public Vector2 Position;
        public Vector2 Target;
    }

    private void Start()
    {
        if (pointA == null || pointB == null)
        {
            Debug.LogError($"{nameof(Enemy)} on {name} requires pointA and pointB references.", this);
            enabled = false;
            return;
        }

        _pointA = pointA.position;
        _pointB = pointB.position;
        _target = _pointB;
    }

    private void Update()
    {
        transform.position = Vector2.MoveTowards(transform.position, _target, speed * Time.deltaTime);

        if (Vector2.Distance(transform.position, _target) < arrivalDistance)
        {
            _target = _target == _pointA ? _pointB : _pointA;
        }
    }

    public void SaveState(List<object> buffer)
    {
        buffer.Add(new EnemyState
        {
            Position = transform.position,
            Target = _target
        });
    }

    public void LoadState(object state)
    {
        if (state is not EnemyState enemyState) return;

        transform.position = enemyState.Position;
        _target = enemyState.Target;
    }
    
    private void OnEnable() => TimeRewind.Register(this);
    private void OnDisable() => TimeRewind.Unregister(this);
}
