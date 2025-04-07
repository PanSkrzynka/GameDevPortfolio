using UnityEngine;

public class BarnesHutTree
{
    private readonly BarnesHutNode _root;
    private readonly float _theta;
    private readonly float _softening;
    private readonly float _gravity;

    public BarnesHutTree(Bounds bounds, float theta, float softening, float gravity)
    {
        _root = new BarnesHutNode(bounds, 0);
        _theta = theta;
        _softening = softening;
        _gravity = gravity;
    }

    public void Insert(GravityObject body)
    {
        _root.Insert(body);
    }

    public void ApplyForce(GravityObject target)
    {
        _root.ApplyForce(target, _theta, _softening, _gravity);
    }
}