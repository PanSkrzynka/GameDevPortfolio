using UnityEngine;

public class SystemGenerator : MonoBehaviour
{
    [SerializeField] private SystemConfig rootSystem;
    [SerializeField] private float gravitationalConstant = 10f;
    [SerializeField] private float massToScaleFactor = 0.05f;

    private void Start()
    {
        GenerateSystemRecursively(rootSystem, transform, transform.position, Vector3.zero);
    }

    private void GenerateSystemRecursively(SystemConfig config, Transform parent, Vector3 position, Vector3 parentVelocity)
    {
        GameObject centralObject = Instantiate(config.centralBodyPrefab, position, Quaternion.identity, parent);

        var gravityComponent = centralObject.GetComponent<GravityObject>();
        gravityComponent.mass = config.centralMass;
        gravityComponent.velocity = parentVelocity;

        float visualScale = Mathf.Max(1f, config.centralMass * massToScaleFactor);
        centralObject.transform.localScale = Vector3.one * visualScale;

        foreach (var orbitData in config.orbitingObjects)
        {
            float childMass = orbitData.bodyMass > 0f
                ? orbitData.bodyMass
                : Random.Range(config.centralMass * 0.01f, config.centralMass * 0.05f);

            orbitData.systemConfig.centralMass = childMass;

            float baseDistance = Mathf.Sqrt(config.centralMass) * Mathf.Pow(childMass, 0.35f);
            float distance = orbitData.distance > 0f
                ? orbitData.distance
                : baseDistance + Random.Range(2f, 5f);

            Vector3 orbitNormal = orbitData.orbitalPlaneAngle != 0f
                ? Quaternion.Euler(orbitData.orbitalPlaneAngle, 0, 0) * Vector3.up
                : Random.onUnitSphere;

            Vector3 orbitDirection = Vector3.Cross(orbitNormal, Random.onUnitSphere).normalized;
            Vector3 offset = orbitDirection * distance;
            Vector3 orbitPosition = position + offset;

            float orbitalSpeed = Mathf.Sqrt(gravitationalConstant * config.centralMass / distance);
            Vector3 tangent = Vector3.Cross(orbitNormal, offset).normalized;
            Vector3 velocity = parentVelocity + tangent * orbitalSpeed;

            GenerateSystemRecursively(orbitData.systemConfig, parent, orbitPosition, velocity);
        }
    }
}