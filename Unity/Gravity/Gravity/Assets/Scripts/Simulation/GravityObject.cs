using UnityEngine;

public class GravityObject : MonoBehaviour
{
    public float mass = 1f;
    public Vector3 velocity;

    private Vector3 _accumulatedForce;

    public void AddForce(Vector3 force)
    {
        _accumulatedForce += force;
    }

    public void Integrate(float deltaTime)
    {
        Vector3 acceleration = _accumulatedForce / mass;
        velocity += acceleration * deltaTime;
        transform.position += velocity * deltaTime;
        _accumulatedForce = Vector3.zero;
    }

    public void ResetForce() => _accumulatedForce = Vector3.zero;

    private void OnEnable() => GravityManager.Register(this);
    private void OnDisable() => GravityManager.Unregister(this);
}