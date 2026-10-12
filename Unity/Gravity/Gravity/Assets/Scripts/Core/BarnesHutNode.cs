using System;
using System.Collections.Generic;
using UnityEngine;

public class BarnesHutNode
{
    private const float MinNodeSize = 0.01f;
    private const int MaxDepth = 15;

    internal readonly struct BodySnapshot
    {
        public readonly GravityObject Body;
        public readonly Vector3 Position;
        public readonly float Mass;

        public BodySnapshot(GravityObject body)
        {
            Body = body;
            Position = body.transform.position;
            Mass = Mathf.Max(0.0001f, body.mass);
        }
    }

    public Bounds Bounds { get; private set; }
    public Vector3 CenterOfMass { get; private set; }
    public float TotalMass { get; private set; }
    public bool IsLeaf => !_subdivided;

    private readonly Stack<BarnesHutNode> _pool;
    private int _depth;
    private BodySnapshot _singleBody;
    private List<BodySnapshot> _additionalBodies;
    private BarnesHutNode[] _children;
    private bool _subdivided;

    public BarnesHutNode(Bounds bounds, int depth = 0)
        : this(bounds, depth, new Stack<BarnesHutNode>()) { }

    private BarnesHutNode(Bounds bounds, int depth, Stack<BarnesHutNode> pool)
    {
        Bounds = bounds;
        _depth = depth;
        _pool = pool;
    }

    internal void Reset(Bounds bounds, int depth = 0)
    {
        if (_subdivided)
        {
            for (int i = 0; i < _children.Length; i++)
            {
                BarnesHutNode child = _children[i];
                child.Reset(default);
                _pool.Push(child);
                _children[i] = null;
            }
        }
        _subdivided = false;
        _singleBody = default;
        _additionalBodies?.Clear();
        TotalMass = 0f;
        CenterOfMass = Vector3.zero;
        Bounds = bounds;
        _depth = depth;
    }

    public void Insert(GravityObject body)
    {
        if (body != null) Insert(new BodySnapshot(body));
    }

    internal void Insert(BodySnapshot body)
    {
        if (!Bounds.Contains(body.Position))
            throw new ArgumentOutOfRangeException(nameof(body), "Body is outside the node bounds. Rebuild the tree to fit all bodies.");
        InsertContained(body);
    }

    private void InsertContained(BodySnapshot body)
    {
        if (IsLeaf)
        {
            if (_singleBody.Body == null)
            {
                _singleBody = body;
                CenterOfMass = body.Position;
                TotalMass = body.Mass;
                return;
            }
            if (_depth >= MaxDepth || Bounds.size.x <= MinNodeSize)
            {
                _additionalBodies ??= new List<BodySnapshot>();
                _additionalBodies.Add(body);
                UpdateMass(body);
                return;
            }
            Subdivide();
            InsertIntoChild(_singleBody);
            _singleBody = default;
        }
        InsertIntoChild(body);
        UpdateMass(body);
    }

    private void UpdateMass(BodySnapshot body)
    {
        CenterOfMass = (CenterOfMass * TotalMass + body.Position * body.Mass) / (TotalMass + body.Mass);
        TotalMass += body.Mass;
    }

    private void InsertIntoChild(BodySnapshot body)
    {
        Vector3 center = Bounds.center;
        int index = (body.Position.x >= center.x ? 1 : 0)
            | (body.Position.y >= center.y ? 2 : 0)
            | (body.Position.z >= center.z ? 4 : 0);
        _children[index].InsertContained(body);
    }

    private void Subdivide()
    {
        _children ??= new BarnesHutNode[8];
        Vector3 halfSize = Bounds.extents * 0.5f;
        for (int i = 0; i < _children.Length; i++)
        {
            Vector3 offset = new((i & 1) == 0 ? -1 : 1, (i & 2) == 0 ? -1 : 1, (i & 4) == 0 ? -1 : 1);
            Bounds childBounds = new(Bounds.center + Vector3.Scale(offset, halfSize), Bounds.size * 0.5f);
            if (_pool.Count > 0)
            {
                _children[i] = _pool.Pop();
                _children[i].Bounds = childBounds;
                _children[i]._depth = _depth + 1;
            }
            else
            {
                _children[i] = new BarnesHutNode(childBounds, _depth + 1, _pool);
            }
        }
        _subdivided = true;
    }

    public void ApplyForce(GravityObject target, float theta, float softening, float gravity)
    {
        if (target != null) ApplyForce(new BodySnapshot(target), theta, softening, gravity);
    }

    internal void ApplyForce(BodySnapshot target, float theta, float softening, float gravity)
    {
        target.Body.AddForce(CalculateForce(target, theta, softening * softening, gravity));
    }

    private Vector3 CalculateForce(BodySnapshot target, float theta, float softeningSquared, float gravity)
    {
        if (TotalMass <= 0f) return Vector3.zero;
        if (IsLeaf)
        {
            Vector3 force = DirectForce(target, _singleBody, softeningSquared, gravity);
            if (_additionalBodies != null)
                foreach (var body in _additionalBodies)
                    force += DirectForce(target, body, softeningSquared, gravity);
            return force;
        }

        Vector3 direction = CenterOfMass - target.Position;
        float distanceSquared = direction.sqrMagnitude + softeningSquared;
        float distance = Mathf.Sqrt(distanceSquared);
        if (!Bounds.Contains(target.Position) && distance > 0f && Bounds.size.x / distance < theta)
            return Force(direction, target.Mass, TotalMass, distanceSquared, gravity);

        Vector3 sum = Vector3.zero;
        foreach (var child in _children)
            sum += child.CalculateForce(target, theta, softeningSquared, gravity);
        return sum;
    }

    private static Vector3 DirectForce(BodySnapshot target, BodySnapshot source, float softeningSquared, float gravity)
    {
        if (source.Body == null || source.Body == target.Body) return Vector3.zero;
        Vector3 direction = source.Position - target.Position;
        return Force(direction, target.Mass, source.Mass, direction.sqrMagnitude + softeningSquared, gravity);
    }

    private static Vector3 Force(Vector3 direction, float targetMass, float sourceMass, float distanceSquared, float gravity)
    {
        if (distanceSquared <= Mathf.Epsilon) return Vector3.zero;
        float inverseDistance = 1f / Mathf.Sqrt(distanceSquared);
        return direction * (gravity * targetMass * sourceMass * inverseDistance * inverseDistance * inverseDistance);
    }
}
