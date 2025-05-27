using System.Collections.Generic;
using UnityEngine;

public class Enemy : MonoBehaviour, IRewindable
{
    [SerializeField] private Transform pointA;
    [SerializeField] private Transform pointB;
    [SerializeField] private float speed = 2f;

    private Vector2 _pointA;
    private Vector2 _pointB;
    private Vector2 _target;

    private void Start()
    {
        _pointA = pointA.position;
        _pointB = pointB.position;
        _target = _pointB;
    }

    private void Update()
    {
        transform.position = Vector2.MoveTowards(transform.position, _target, speed * Time.deltaTime);

        if (Vector2.Distance(transform.position, _target) < 0.05f)
        {
            _target = _target == _pointA ? _pointB : _pointA ;
        }
    }

    public void SaveState(List<object> buffer)
    {
        buffer.Add(transform.position);
        buffer.Add(_target);
    }

    public void LoadState(object state)
    {
        var data = (List<object>)state;
        transform.position = (Vector2)data[0];
        _target = (Vector2)data[1];
    }
    
    private void OnEnable() => TimeRewind.Register(this);
    private void OnDisable() => TimeRewind.Unregister(this);
}