using UnityEngine;

public class SystemGenerator : MonoBehaviour
{
    private const float MinMass = 0.0001f;
    private const float FullCircleDegrees = 360f;
    private const float PoleAlignmentThreshold = 0.99f;

    [SerializeField] private SystemConfig rootSystem;
    [SerializeField, Min(MinMass)] private float gravitationalConstant = 10f;
    [SerializeField, Min(MinMass)] private float massToScaleFactor = 0.05f;
    [SerializeField, Min(1)] private int maxGenerationDepth = 6;
    [SerializeField] private bool allowMultipleGenerators = false;
    [SerializeField] private bool clearExistingChildrenOnStart = true;

    [Header("Visual Scale")]
    [Tooltip("Smallest visual scale a body can be given, regardless of mass.")]
    [SerializeField, Min(0.01f)] private float minVisualScale = 1f;

    [Header("Fallback Mass")]
    [Tooltip("Child mass as a fraction of its parent when neither the orbit data nor the child config specifies one.")]
    [SerializeField] private Vector2 randomChildMassFraction = new(0.01f, 0.05f);

    [Header("Fallback Orbit Distance")]
    [Tooltip("Exponent applied to child mass when deriving an unspecified orbit distance.")]
    [SerializeField] private float childMassDistanceExponent = 0.35f;

    [Tooltip("Random padding added to a derived orbit distance.")]
    [SerializeField] private Vector2 randomDistancePadding = new(2f, 5f);

    [SerializeField, Min(0.01f)] private float minOrbitDistance = 1f;

    private static int s_startedGenerators;
    private bool _countedAsStarted;

    private void Start()
    {
        if (!allowMultipleGenerators && s_startedGenerators > 0)
        {
            Debug.LogWarning($"Skipping generation on '{name}' because another {nameof(SystemGenerator)} already ran.", this);
            return;
        }

        s_startedGenerators++;
        _countedAsStarted = true;

        if (rootSystem == null)
        {
            Debug.LogError($"{nameof(SystemGenerator)} on {name} has no root system assigned.", this);
            return;
        }

        if (clearExistingChildrenOnStart)
            ClearChildren();

        GenerateSystemRecursively(rootSystem, transform, transform.position, Vector3.zero, rootSystem.centralMass, 0);
    }

    private void OnDestroy()
    {
        if (!_countedAsStarted)
            return;

        s_startedGenerators = Mathf.Max(0, s_startedGenerators - 1);
        _countedAsStarted = false;
    }

    private void GenerateSystemRecursively(
        SystemConfig config,
        Transform parent,
        Vector3 position,
        Vector3 parentVelocity,
        float centralMass,
        int depth)
    {
        if (depth > maxGenerationDepth)
            return;

        if (config == null)
            return;

        if (config.centralBodyPrefab == null)
        {
            Debug.LogWarning($"System config '{config.name}' has no central body prefab.", this);
            return;
        }

        GameObject centralObject = Instantiate(config.centralBodyPrefab, position, Quaternion.identity, parent);
        centralObject.name = $"{config.name}_Body_{depth}";

        if (!centralObject.TryGetComponent(out GravityObject gravityComponent))
        {
            Debug.LogWarning($"Prefab '{config.centralBodyPrefab.name}' has no {nameof(GravityObject)} component.", centralObject);
            Destroy(centralObject);
            return;
        }

        gravityComponent.mass = Mathf.Max(MinMass, centralMass);
        gravityComponent.velocity = parentVelocity;

        float visualScale = Mathf.Max(minVisualScale, centralMass * massToScaleFactor);
        centralObject.transform.localScale = Vector3.one * visualScale;

        if (centralObject.TryGetComponent(out BodyAppearance appearance))
            appearance.Apply(config.visualProfile, centralMass);

        if (config.orbitingObjects == null)
            return;

        foreach (OrbitingObjectData orbitData in config.orbitingObjects)
        {
            if (orbitData == null || orbitData.systemConfig == null)
                continue;

            float childMass = ResolveChildMass(orbitData, orbitData.systemConfig, centralMass);
            float distance = ResolveOrbitDistance(orbitData, centralMass, childMass);

            Vector3 orbitNormal = ResolveOrbitNormal(orbitData);
            Vector3 radialDirection = GetPerpendicularDirection(orbitNormal);

            float initialAngle = orbitData.initialAngle != 0f
                ? orbitData.initialAngle
                : Random.Range(0f, FullCircleDegrees);

            Vector3 offset = Quaternion.AngleAxis(initialAngle, orbitNormal) * radialDirection * distance;
            Vector3 orbitPosition = position + offset;

            float orbitalSpeed = Mathf.Sqrt(Mathf.Max(MinMass, gravitationalConstant * centralMass / distance));
            Vector3 tangent = Vector3.Cross(orbitNormal, offset.normalized).normalized;
            Vector3 velocity = parentVelocity + tangent * orbitalSpeed;

            GenerateSystemRecursively(orbitData.systemConfig, parent, orbitPosition, velocity, childMass, depth + 1);
        }
    }

    private static Vector3 ResolveOrbitNormal(OrbitingObjectData orbitData)
    {
        if (Mathf.Approximately(orbitData.orbitalPlaneAngle, 0f))
            return Random.onUnitSphere.normalized;

        return (Quaternion.Euler(orbitData.orbitalPlaneAngle, 0f, 0f) * Vector3.up).normalized;
    }

    private static Vector3 GetPerpendicularDirection(Vector3 normal)
    {
        Vector3 axis = Mathf.Abs(normal.y) > PoleAlignmentThreshold ? Vector3.right : Vector3.up;
        Vector3 perpendicular = Vector3.Cross(normal, axis);

        if (perpendicular.sqrMagnitude <= Mathf.Epsilon)
            perpendicular = Vector3.Cross(normal, Vector3.forward);

        return perpendicular.normalized;
    }

    private float ResolveChildMass(OrbitingObjectData orbitData, SystemConfig childConfig, float parentMass)
    {
        if (orbitData.bodyMass > 0f)
            return orbitData.bodyMass;

        if (childConfig.centralMass > 0f)
            return childConfig.centralMass;

        return Random.Range(parentMass * randomChildMassFraction.x, parentMass * randomChildMassFraction.y);
    }

    private float ResolveOrbitDistance(OrbitingObjectData orbitData, float parentMass, float childMass)
    {
        if (orbitData.distance > 0f)
            return orbitData.distance;

        float baseDistance = Mathf.Sqrt(parentMass) * Mathf.Pow(childMass, childMassDistanceExponent);
        return Mathf.Max(minOrbitDistance, baseDistance + Random.Range(randomDistancePadding.x, randomDistancePadding.y));
    }

    private void ClearChildren()
    {
        for (int i = transform.childCount - 1; i >= 0; i--)
        {
            Transform child = transform.GetChild(i);
            if (Application.isPlaying)
                Destroy(child.gameObject);
            else
                DestroyImmediate(child.gameObject);
        }
    }
}
