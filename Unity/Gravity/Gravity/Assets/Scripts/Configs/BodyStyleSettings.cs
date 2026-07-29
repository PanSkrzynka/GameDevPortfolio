using UnityEngine;

[System.Serializable]
public class SurfaceStyleSettings
{
    [Header("Surface")]
    [Tooltip("Multiplier applied to the base colour to produce the detail colour.")]
    [Range(0f, 1f)] public float detailColorScale = 0.55f;

    [Range(0f, 1f)] public float detailStrength = 0.55f;
    [Range(0f, 1f)] public float bandStrength;
    [Range(0f, 1f)] public float smoothness = 0.18f;

    [Header("Glow")]
    [Tooltip("Emissive floor that keeps small unlit bodies readable against the background.")]
    [Min(0f)] public float emissionStrength = 0.2f;

    [Tooltip("Peak the rim colour is pushed to. Above 1 the limb drives bloom.")]
    [Min(0f)] public float rimBrightness = 2.4f;

    [Min(0f)] public float rimStrength = 1.6f;
    [Range(0.5f, 12f)] public float rimPower = 2.8f;

    [Header("Trail")]
    [Range(0f, 1f)] public float trailAlpha = 0.6f;
}

[System.Serializable]
public class StarStyleSettings
{
    [Header("Temperature Colours")]
    public Color coolColor = new(1f, 0.55f, 0.22f);
    public Color midColor = new(1f, 0.88f, 0.62f);
    public Color hotColor = new(0.66f, 0.8f, 1f);

    [Header("Surface")]
    [Tooltip("Albedo multiplier. Kept low so the star's own centred light cannot flatten its surface.")]
    [Range(0f, 1f)] public float albedoScale = 0.08f;

    [Range(0f, 1f)] public float detailColorScale = 0.04f;
    [Range(0f, 1f)] public float detailStrength = 0.65f;
    [Range(0f, 1f)] public float bandStrength = 0.15f;
    [Range(0f, 1f)] public float smoothness;

    [Header("Glow")]
    [Tooltip("Emission by temperature. Far above the bloom threshold every channel saturates to white.")]
    public Vector2 emissionStrengthRange = new(1.2f, 1.6f);

    [Min(0f)] public float rimBrightness = 1.6f;
    [Min(0f)] public float rimStrength = 0.9f;
    [Range(0.5f, 12f)] public float rimPower = 2.2f;

    [Header("Trail")]
    [Range(0f, 1f)] public float trailAlpha = 0.85f;

    [Header("Light")]
    public Color coolLightColor = new(1f, 0.82f, 0.6f);
    public Color hotLightColor = new(0.8f, 0.88f, 1f);
    public Vector2 lightIntensityRange = new(2.6f, 4.2f);
    [Min(1f)] public float lightRangeMultiplier = 90f;

    [Header("Halo")]
    [Min(1f)] public float haloSize = 2.8f;
    public Vector2 haloIntensityRange = new(0.5f, 0.85f);
    [Range(0.5f, 8f)] public float haloFalloff = 3.2f;
}

[System.Serializable]
public class BlackHoleStyleSettings
{
    [Header("Surface")]
    [ColorUsage(false, true)] public Color baseColor = new(0.015f, 0.015f, 0.02f);
    [ColorUsage(false, true)] public Color detailColor = new(0.03f, 0.02f, 0.04f);
    [Range(0f, 1f)] public float detailStrength = 0.1f;
    [Range(0f, 1f)] public float smoothness = 0.05f;

    [Header("Photon Ring")]
    [ColorUsage(false, true)] public Color ringColor = new(1.9f, 0.62f, 0.18f);

    [Tooltip("Raising this makes bloom bleed across the disc until it stops reading as black.")]
    [Min(0f)] public float ringStrength = 1.8f;

    [Range(0.5f, 12f)] public float ringPower = 9f;

    [Header("Trail")]
    [ColorUsage(false, true)] public Color trailColor = new(0.85f, 0.45f, 1f);
    [Range(0f, 1f)] public float trailAlpha = 0.8f;

    [Header("Light")]
    public Color lightColor = new(0.72f, 0.45f, 1f);
    [Min(0f)] public float lightIntensity = 1.8f;
    [Min(1f)] public float lightRangeMultiplier = 120f;

    [Header("Halo")]
    [ColorUsage(false, true)] public Color haloColor = new(1.1f, 0.5f, 0.75f);
    [Min(1f)] public float haloSize = 1.9f;
    [Min(0f)] public float haloIntensity = 0.5f;
    [Range(0.5f, 8f)] public float haloFalloff = 4.5f;
}
