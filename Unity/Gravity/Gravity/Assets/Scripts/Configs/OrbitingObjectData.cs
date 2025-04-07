using UnityEngine;

[System.Serializable]
public class OrbitingObjectData
{
    [Header("Child System Configuration")]
    [Tooltip("Nested planetary system configuration.")]
    public SystemConfig systemConfig;

    [Header("Orbital Parameters")]
    [Tooltip("If 0, a random distance will be assigned.")]
    public float distance = 0f;

    [Tooltip("If 0, a random angle (0–360) will be assigned.")]
    public float initialAngle = 0f;

    [Tooltip("Orbital plane inclination angle. Leave at 0 unless inclined orbits are needed.")]
    public float orbitalPlaneAngle = 0f;

    [Tooltip("If 0, mass from child system will be used. If > 0, overrides it.")]
    public float bodyMass = 0f;
}