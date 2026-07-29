using UnityEngine;

public enum BodyClass
{
    Rocky,
    GasGiant,
    Star,
    BlackHole
}

[CreateAssetMenu(fileName = "BodyVisualProfile", menuName = "OrbitalSystem/Body Visual Profile")]
public class BodyVisualProfile : ScriptableObject
{
    [Header("Classification")]
    public BodyClass bodyClass = BodyClass.Rocky;

    [Header("Surface")]
    [ColorUsage(false, true)] public Color baseColor = new(0.42f, 0.48f, 0.62f);
    [ColorUsage(false, true)] public Color detailColor = new(0.22f, 0.26f, 0.38f);
    [Range(0f, 1f)] public float detailStrength = 0.4f;
    [Range(0f, 1f)] public float bandStrength;
    [Range(0f, 1f)] public float smoothness = 0.2f;

    [Header("Glow")]
    [ColorUsage(false, true)] public Color emissionColor = Color.black;
    [Min(0f)] public float emissionStrength;
    [ColorUsage(false, true)] public Color rimColor = new(0.35f, 0.7f, 1f);
    [Min(0f)] public float rimStrength = 0.6f;
    [Range(0.5f, 12f)] public float rimPower = 3f;

    [Header("Trail")]
    [ColorUsage(false, true)] public Color trailColor = new(0.4f, 0.7f, 1f);
    [Range(0f, 1f)] public float trailAlpha = 0.7f;

    [Header("Light Emission")]
    [Tooltip("Bodies that light their system. Lights are budgeted at runtime, heaviest bodies first.")]
    public bool emitsLight;
    [ColorUsage(false, false)] public Color lightColor = new(1f, 0.93f, 0.78f);
    [Min(0f)] public float lightIntensity = 2.5f;

    [Tooltip("Light range as a multiple of the body's radius.")]
    [Min(1f)] public float lightRangeMultiplier = 60f;

    [Header("Halo")]
    public bool hasHalo;
    [ColorUsage(false, true)] public Color haloColor = new(1f, 0.8f, 0.45f);

    [Tooltip("Halo diameter as a multiple of the body's diameter.")]
    [Min(1f)] public float haloSize = 3.5f;

    [Min(0f)] public float haloIntensity = 1.2f;
    [Range(0.5f, 8f)] public float haloFalloff = 2.5f;
}
