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
        if (body == null)
            return;

        Vector3 position = body.transform.position;
        float bodyMass = Mathf.Max(0.0001f, body.mass);

        if (IsLeaf)
        {
            if (_singleBody == null)
            {
                _singleBody = body;
                CenterOfMass = position;
                TotalMass = bodyMass;
                return;
            }

            if (_depth >= MaxDepth || Bounds.size.x <= MinNodeSize)
            {
                UpdateMass(position, bodyMass);
                return;
            }

            Subdivide();
            InsertIntoChildren(_singleBody);
            _singleBody = null;
        }

        InsertIntoChildren(body);
        UpdateMass(position, bodyMass);
    }

    private void UpdateMass(Vector3 position, float mass)
    {
        if (TotalMass <= 0f)
        {
            CenterOfMass = position;
            TotalMass = mass;
            return;
        }

        CenterOfMass = (CenterOfMass * TotalMass + position * mass) / (TotalMass + mass);
        TotalMass += mass;
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
        if (target == null || TotalMass <= 0f)
            return;

        if (IsLeaf)
        {
            if (_singleBody == null || _singleBody == target)
                return;

            ApplyDirectForce(target, _singleBody, softening, gravity);
            return;
        }

        Vector3 targetPosition = target.transform.position;
        Vector3 direction = CenterOfMass - targetPosition;
        float softenedDistSqr = direction.sqrMagnitude + softening * softening;

        if (softenedDistSqr <= Mathf.Epsilon)
            return;

        float softenedDistance = Mathf.Sqrt(softenedDistSqr);
        bool nodeContainsTarget = Bounds.Contains(targetPosition);

        if (!nodeContainsTarget && (Bounds.size.x / softenedDistance) < theta)
        {
            float invDist = 1f / softenedDistance;
            float invDistCubed = invDist * invDist * invDist;
            float forceScalar = gravity * target.mass * TotalMass * invDistCubed;
            target.AddForce(direction * forceScalar);
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

        if (distSqr <= Mathf.Epsilon)
            return;

        float invDist = 1f / Mathf.Sqrt(distSqr);
        float invDistCubed = invDist * invDist * invDist;
        float forceScalar = gravity * a.mass * b.mass * invDistCubed;
        a.AddForce(direction * forceScalar);
    }
}
