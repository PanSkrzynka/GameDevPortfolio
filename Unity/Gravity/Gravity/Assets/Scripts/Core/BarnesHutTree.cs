using System.Collections.Generic;
using UnityEngine;

public class BarnesHutTree
{
    private readonly BarnesHutNode _root;
    private readonly List<BarnesHutNode.BodySnapshot> _bodies = new();
    private float _theta;
    private float _softening;
    private float _gravity;

    public Bounds Bounds => _root.Bounds;

    public BarnesHutTree(Bounds bounds, float theta, float softening, float gravity)
    {
        _root = new BarnesHutNode(bounds);
        _theta = theta;
        _softening = softening;
        _gravity = gravity;
    }

    public void Rebuild(IReadOnlyList<GravityObject> bodies, Bounds minimumBounds, float theta, float softening, float gravity)
    {
        _bodies.Clear();
        Vector3 min = minimumBounds.min;
        Vector3 max = minimumBounds.max;
        for (int i = 0; i < bodies.Count; i++)
        {
            if (bodies[i] == null) continue;
            var snapshot = new BarnesHutNode.BodySnapshot(bodies[i]);
            _bodies.Add(snapshot);
            min = Vector3.Min(min, snapshot.Position);
            max = Vector3.Max(max, snapshot.Position);
        }

        Vector3 size = max - min;
        float side = Mathf.Max(0.01f, Mathf.Max(size.x, Mathf.Max(size.y, size.z))) * 1.001f;
        _root.Reset(new Bounds(min + size * 0.5f, Vector3.one * side));
        _theta = theta;
        _softening = softening;
        _gravity = gravity;
        foreach (var snapshot in _bodies) _root.Insert(snapshot);
    }

    public void Insert(GravityObject body)
    {
        if (body == null) return;
        var snapshot = new BarnesHutNode.BodySnapshot(body);
        _root.Insert(snapshot);
        _bodies.Add(snapshot);
    }

    public void ApplyForce(GravityObject target)
    {
        _root.ApplyForce(target, _theta, _softening, _gravity);
    }

    public void ApplyForces()
    {
        foreach (var snapshot in _bodies)
            if (snapshot.Body != null)
                _root.ApplyForce(snapshot, _theta, _softening, _gravity);
    }
}
