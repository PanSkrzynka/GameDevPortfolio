using System.Collections.Generic;
using UnityEngine;

[CreateAssetMenu(fileName = "SystemConfig", menuName = "OrbitalSystem/System Config")]
public class SystemConfig : ScriptableObject
{
    [Header("Central Body")]
    public GameObject centralBodyPrefab;
    public float centralMass = 1000f;

    [Header("Orbiting Bodies")]
    public List<OrbitingObjectData> orbitingObjects = new();
}