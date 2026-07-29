using UnityEngine;

public class GravityObject : MonoBehaviour
{
    private const float MinMass = 0.0001f;
    private const float MinTrailWidth = 0.001f;

    [Min(MinMass)] public float mass = 1f;
    public Vector3 velocity;

    [SerializeField] private bool autoScaleTrailWidth = true;
    [SerializeField, Min(MinTrailWidth)] private float trailWidthScale = 0.08f;
    [SerializeField] private Vector2 trailWidthLimits = new(0.03f, 1f);

    private Vector3 _accumulatedForce;
    private TrailRenderer _trailRenderer;

    public void AddForce(Vector3 force)
    {
        _accumulatedForce += force;
    }

    public void Integrate(float deltaTime)
    {
        float safeMass = Mathf.Max(MinMass, mass);
        Vector3 acceleration = _accumulatedForce / safeMass;
        velocity += acceleration * deltaTime;
        transform.position += velocity * deltaTime;
        _accumulatedForce = Vector3.zero;
    }

    public void ResetForce() => _accumulatedForce = Vector3.zero;

    public void RefreshTrailAppearance() => UpdateTrailAppearance();

    private void Awake()
    {
        TryConfigureRigidbody();
        UpdateTrailAppearance();
    }

    private void OnValidate()
    {
        mass = Mathf.Max(MinMass, mass);
        trailWidthLimits.x = Mathf.Max(MinTrailWidth, trailWidthLimits.x);
        trailWidthLimits.y = Mathf.Max(trailWidthLimits.x, trailWidthLimits.y);
        trailWidthScale = Mathf.Max(MinTrailWidth, trailWidthScale);

        if (autoScaleTrailWidth && Application.isPlaying)
            UpdateTrailAppearance();
    }

    private void TryConfigureRigidbody()
    {
        if (!TryGetComponent(out Rigidbody rb))
            return;

        rb.useGravity = false;
        rb.isKinematic = true;
    }

    private void UpdateTrailAppearance()
    {
        if (!autoScaleTrailWidth)
            return;

        if (!TryGetComponent(out _trailRenderer))
            return;

        float scaledWidth = Mathf.Sqrt(mass) * trailWidthScale;
        _trailRenderer.widthMultiplier = Mathf.Clamp(scaledWidth, trailWidthLimits.x, trailWidthLimits.y);
    }

    private void OnEnable() => GravityManager.Register(this);
    private void OnDisable() => GravityManager.Unregister(this);
}
