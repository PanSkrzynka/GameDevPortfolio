using UnityEngine;

public class BarnesHutNode
{
    private const float MinNodeSize = 0.01f;
    private const int MaxDepth = 15;

    public Bounds Bounds { get; }
    public Vector3 CenterOfMass { get; private set; }
    public float TotalMass { get; private set; }
    public bool IsLeaf => _children == null;

    private int _depth;
    private GravityObject _singleBody;
    private BarnesHutNode[] _children;

    public BarnesHutNode(Bounds bounds, int depth = 0)
    {
        Bounds = bounds;
        _depth = depth;
    }

    public void Insert(GravityObject body)
    {
        Vector3 position = body.transform.position;

        if (IsLeaf)
        {
            if (_singleBody == null)
            {
                _singleBody = body;
                CenterOfMass = position;
                TotalMass = body.mass;
                return;
            }

            if (_depth >= MaxDepth || Bounds.size.x <= MinNodeSize)
                return;

            Subdivide();
            InsertIntoChildren(_singleBody);
            _singleBody = null;
        }

        InsertIntoChildren(body);
        UpdateMass(body);
    }

    private void UpdateMass(GravityObject body)
    {
        float m = body.mass;
        CenterOfMass = (CenterOfMass * TotalMass + body.transform.position * m) / (TotalMass + m);
        TotalMass += m;
    }

    private void InsertIntoChildren(GravityObject body)
    {
        foreach (var child in _children)
        {
            if (child.Bounds.Contains(body.transform.position))
            {
                child.Insert(body);
                return;
            }
        }

        _children[0].Insert(body);
    }

    private void Subdivide()
    {
        _children = new BarnesHutNode[8];
        Vector3 center = Bounds.center;
        Vector3 halfSize = Bounds.extents * 0.5f;

        for (int i = 0; i < 8; i++)
        {
            Vector3 offset = new(
                (i & 1) == 0 ? -1 : 1,
                (i & 2) == 0 ? -1 : 1,
                (i & 4) == 0 ? -1 : 1
            );

            Vector3 newCenter = center + Vector3.Scale(offset, halfSize);
            Vector3 size = Bounds.size * 0.5f;

            _children[i] = new BarnesHutNode(new Bounds(newCenter, size), _depth + 1);
        }
    }

    public void ApplyForce(GravityObject target, float theta, float softening, float gravity)
    {
        if (IsLeaf)
        {
            if (_singleBody == null || _singleBody == target)
                return;

            ApplyDirectForce(target, _singleBody, softening, gravity);
            return;
        }

        Vector3 direction = CenterOfMass - target.transform.position;
        float distance = direction.magnitude;

        if ((Bounds.size.x / distance) < theta)
        {
            float forceMagnitude = gravity * target.mass * TotalMass / Mathf.Pow(distance * distance + softening * softening, 1.5f);
            target.AddForce(direction.normalized * forceMagnitude);
        }
        else
        {
            foreach (var child in _children)
                child?.ApplyForce(target, theta, softening, gravity);
        }
    }

    private void ApplyDirectForce(GravityObject a, GravityObject b, float softening, float gravity)
    {
        Vector3 direction = b.transform.position - a.transform.position;
        float distSqr = direction.sqrMagnitude + softening * softening;
        float forceMagnitude = gravity * a.mass * b.mass / Mathf.Pow(distSqr, 1.5f);
        a.AddForce(direction.normalized * forceMagnitude);
    }
}