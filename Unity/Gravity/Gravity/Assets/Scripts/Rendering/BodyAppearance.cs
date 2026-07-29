using UnityEngine;
using UnityEngine.Rendering;

[AddComponentMenu("Gravity/Rendering/Body Appearance")]
public class BodyAppearance : MonoBehaviour
{
    private const float SphereRadiusPerUnitScale = 0.5f;
    private const float MinBodyRadius = 0.01f;
    private const float FallbackMass = 1f;
    private const float MinColorPeak = 0.0001f;
    private const float QuadHalfSize = 0.5f;
    private const float HaloMeshBoundsSize = 4f;
    private const string HaloObjectName = "Halo";
    private const string HaloMeshName = "HaloQuad";

    [Header("Profile")]
    [Tooltip("Explicit look for this body. Leave empty to derive one from mass.")]
    [SerializeField] private BodyVisualProfile profileOverride;

    [Tooltip("Material used for the billboard halo on stars and black holes.")]
    [SerializeField] private Material haloMaterial;

    [Header("Auto-Classification")]
    [Tooltip("Mass at or above which a body is treated as a gas giant.")]
    [SerializeField, Min(0f)] private float gasGiantMassThreshold = 1.5f;

    [Tooltip("Mass at or above which a body is treated as a star.")]
    [SerializeField, Min(0f)] private float starMassThreshold = 8f;

    [Tooltip("Mass at or above which a body is treated as a black hole.")]
    [SerializeField, Min(0f)] private float blackHoleMassThreshold = 400f;

    [Tooltip("Mass range mapped to star colour, from cool orange to hot blue-white.")]
    [SerializeField] private Vector2 starTemperatureMassRange = new(8f, 60f);

    [Tooltip("How much of a star's colour comes from mass rather than its palette hue.")]
    [SerializeField, Range(0f, 1f)] private float starTemperatureWeight = 0.35f;

    [Header("Palettes")]
    [SerializeField] private Color[] rockyPalette =
    {
        new(0.45f, 0.50f, 0.58f),
        new(0.52f, 0.38f, 0.32f),
        new(0.32f, 0.45f, 0.48f),
        new(0.48f, 0.44f, 0.36f),
        new(0.38f, 0.36f, 0.50f),
        new(0.30f, 0.42f, 0.38f)
    };

    [SerializeField] private Color[] gasGiantPalette =
    {
        new(0.20f, 0.52f, 0.74f),
        new(0.60f, 0.28f, 0.54f),
        new(0.42f, 0.28f, 0.70f),
        new(0.74f, 0.48f, 0.20f),
        new(0.18f, 0.60f, 0.53f),
        new(0.66f, 0.30f, 0.34f)
    };

    [SerializeField] private Color[] starPalette =
    {
        new(1.00f, 0.72f, 0.30f),
        new(0.40f, 0.85f, 1.00f),
        new(0.62f, 0.72f, 1.00f),
        new(0.85f, 0.55f, 1.00f),
        new(1.00f, 0.50f, 0.62f),
        new(0.55f, 1.00f, 0.85f)
    };

    [Header("Derived Styles")]
    [SerializeField] private SurfaceStyleSettings rockyStyle = new();

    [SerializeField] private SurfaceStyleSettings gasGiantStyle = new()
    {
        detailColorScale = 0.45f,
        detailStrength = 0.7f,
        bandStrength = 0.85f,
        smoothness = 0.32f,
        emissionStrength = 0.25f,
        rimBrightness = 2.4f,
        rimStrength = 1.4f,
        rimPower = 2.6f,
        trailAlpha = 0.7f
    };

    [SerializeField] private StarStyleSettings starStyle = new();
    [SerializeField] private BlackHoleStyleSettings blackHoleStyle = new();

    [Header("Trail")]
    [Tooltip("Multiplier applied to the head colour to produce the faded tail colour.")]
    [SerializeField, Range(0f, 1f)] private float trailTailScale = 0.35f;

    [Header("Halo")]
    [Tooltip("Multiplier from halo intensity to the brightness of its hot core.")]
    [SerializeField, Min(0f)] private float haloCoreIntensityScale = 1.4f;

    [Header("Light Budget")]
    [Tooltip("Cap on real-time lights across the whole simulation. The heaviest bodies get them first.")]
    [SerializeField, Min(0)] private int maxSceneLights = 8;

    private static readonly int ColorId = Shader.PropertyToID("_Color");
    private static readonly int DetailColorId = Shader.PropertyToID("_DetailColor");
    private static readonly int EmissionColorId = Shader.PropertyToID("_EmissionColor");
    private static readonly int EmissionStrengthId = Shader.PropertyToID("_EmissionStrength");
    private static readonly int RimColorId = Shader.PropertyToID("_RimColor");
    private static readonly int RimStrengthId = Shader.PropertyToID("_RimStrength");
    private static readonly int RimPowerId = Shader.PropertyToID("_RimPower");
    private static readonly int DetailStrengthId = Shader.PropertyToID("_DetailStrength");
    private static readonly int BandStrengthId = Shader.PropertyToID("_BandStrength");
    private static readonly int GlossinessId = Shader.PropertyToID("_Glossiness");
    private static readonly int IntensityId = Shader.PropertyToID("_Intensity");
    private static readonly int CoreIntensityId = Shader.PropertyToID("_CoreIntensity");
    private static readonly int FalloffId = Shader.PropertyToID("_Falloff");
    private static readonly int SizeId = Shader.PropertyToID("_Size");

    private static MaterialPropertyBlock s_propertyBlock;
    private static Mesh s_haloMesh;
    private static int s_activeLights;
    private static int s_spawnCounter;

    private Renderer _renderer;
    private TrailRenderer _trailRenderer;
    private Light _light;
    private bool _applied;
    private bool _ownsLight;

    public void Apply(BodyVisualProfile profile, float mass)
    {
        _applied = true;

        BodyVisualProfile source = profile != null ? profile : profileOverride;
        ResolvedStyle style = source != null ? ResolvedStyle.From(source) : DeriveStyle(mass);

        ApplySurface(style);
        ApplyTrail(style);
        ApplyLight(style);
        ApplyHalo(style);
    }

    private void Start()
    {
        if (_applied)
            return;

        float mass = TryGetComponent(out GravityObject gravityObject) ? gravityObject.mass : FallbackMass;
        Apply(null, mass);
    }

    private void OnDestroy()
    {
        if (!_ownsLight)
            return;

        s_activeLights = Mathf.Max(0, s_activeLights - 1);
        _ownsLight = false;
    }

    [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.SubsystemRegistration)]
    private static void ResetStatics()
    {
        s_activeLights = 0;
        s_spawnCounter = 0;
    }

    private void ApplySurface(ResolvedStyle style)
    {
        if (_renderer == null && !TryGetComponent(out _renderer))
            return;

        s_propertyBlock ??= new MaterialPropertyBlock();

        _renderer.GetPropertyBlock(s_propertyBlock);
        s_propertyBlock.SetColor(ColorId, style.BaseColor);
        s_propertyBlock.SetColor(DetailColorId, style.DetailColor);
        s_propertyBlock.SetColor(EmissionColorId, style.EmissionColor);
        s_propertyBlock.SetFloat(EmissionStrengthId, style.EmissionStrength);
        s_propertyBlock.SetColor(RimColorId, style.RimColor);
        s_propertyBlock.SetFloat(RimStrengthId, style.RimStrength);
        s_propertyBlock.SetFloat(RimPowerId, style.RimPower);
        s_propertyBlock.SetFloat(DetailStrengthId, style.DetailStrength);
        s_propertyBlock.SetFloat(BandStrengthId, style.BandStrength);
        s_propertyBlock.SetFloat(GlossinessId, style.Smoothness);
        _renderer.SetPropertyBlock(s_propertyBlock);
    }

    private void ApplyTrail(ResolvedStyle style)
    {
        if (_trailRenderer == null && !TryGetComponent(out _trailRenderer))
            return;

        Color head = style.TrailColor;
        Color tail = head * trailTailScale;

        _trailRenderer.colorGradient = new Gradient
        {
            colorKeys = new[]
            {
                new GradientColorKey(head, 0f),
                new GradientColorKey(tail, 1f)
            },
            alphaKeys = new[]
            {
                new GradientAlphaKey(style.TrailAlpha, 0f),
                new GradientAlphaKey(0f, 1f)
            }
        };

        if (TryGetComponent(out GravityObject gravityObject))
            gravityObject.RefreshTrailAppearance();
    }

    private void ApplyLight(ResolvedStyle style)
    {
        bool wantsLight = style.EmitsLight && s_activeLights < maxSceneLights;

        if (!wantsLight)
        {
            if (_ownsLight && _light != null)
            {
                _light.enabled = false;
                s_activeLights = Mathf.Max(0, s_activeLights - 1);
                _ownsLight = false;
            }

            return;
        }

        if (_light == null && !TryGetComponent(out _light))
            _light = gameObject.AddComponent<Light>();

        if (!_ownsLight)
        {
            s_activeLights++;
            _ownsLight = true;
        }

        float radius = Mathf.Max(MinBodyRadius, transform.localScale.x * SphereRadiusPerUnitScale);

        _light.enabled = true;
        _light.type = LightType.Point;
        _light.color = style.LightColor;
        _light.intensity = style.LightIntensity;
        _light.range = radius * style.LightRangeMultiplier;
        _light.shadows = LightShadows.None;
        _light.renderMode = LightRenderMode.Auto;
    }

    private void ApplyHalo(ResolvedStyle style)
    {
        if (!style.HasHalo || haloMaterial == null)
            return;

        Transform halo = transform.Find(HaloObjectName);
        if (halo == null)
        {
            GameObject haloObject = new(HaloObjectName);
            haloObject.transform.SetParent(transform, false);

            MeshFilter filter = haloObject.AddComponent<MeshFilter>();
            filter.sharedMesh = GetHaloMesh();

            MeshRenderer meshRenderer = haloObject.AddComponent<MeshRenderer>();
            meshRenderer.sharedMaterial = haloMaterial;
            meshRenderer.shadowCastingMode = ShadowCastingMode.Off;
            meshRenderer.receiveShadows = false;
            meshRenderer.lightProbeUsage = LightProbeUsage.Off;
            meshRenderer.reflectionProbeUsage = ReflectionProbeUsage.Off;

            halo = haloObject.transform;
        }

        if (!halo.TryGetComponent(out Renderer haloRenderer))
            return;

        s_propertyBlock ??= new MaterialPropertyBlock();

        haloRenderer.GetPropertyBlock(s_propertyBlock);
        s_propertyBlock.SetColor(ColorId, style.HaloColor);
        s_propertyBlock.SetFloat(IntensityId, style.HaloIntensity);
        s_propertyBlock.SetFloat(CoreIntensityId, style.HaloIntensity * haloCoreIntensityScale);
        s_propertyBlock.SetFloat(FalloffId, style.HaloFalloff);
        s_propertyBlock.SetFloat(SizeId, style.HaloSize);
        haloRenderer.SetPropertyBlock(s_propertyBlock);
    }

    private static Mesh GetHaloMesh()
    {
        if (s_haloMesh != null)
            return s_haloMesh;

        s_haloMesh = new Mesh
        {
            name = HaloMeshName,
            vertices = new[]
            {
                new Vector3(-QuadHalfSize, -QuadHalfSize, 0f),
                new Vector3(QuadHalfSize, -QuadHalfSize, 0f),
                new Vector3(-QuadHalfSize, QuadHalfSize, 0f),
                new Vector3(QuadHalfSize, QuadHalfSize, 0f)
            },
            uv = new[]
            {
                new Vector2(0f, 0f),
                new Vector2(1f, 0f),
                new Vector2(0f, 1f),
                new Vector2(1f, 1f)
            },
            triangles = new[] { 0, 2, 1, 2, 3, 1 }
        };

        s_haloMesh.RecalculateNormals();
        s_haloMesh.bounds = new Bounds(Vector3.zero, Vector3.one * HaloMeshBoundsSize);
        s_haloMesh.hideFlags = HideFlags.HideAndDontSave;

        return s_haloMesh;
    }

    private ResolvedStyle DeriveStyle(float mass)
    {
        uint variant = (uint)name.GetHashCode() + (uint)s_spawnCounter++;

        if (mass >= blackHoleMassThreshold)
            return BuildBlackHoleStyle();

        if (mass >= starMassThreshold)
        {
            float temperature = Mathf.InverseLerp(starTemperatureMassRange.x, starTemperatureMassRange.y, mass);
            return BuildStarStyle(temperature, Pick(starPalette, variant));
        }

        if (mass >= gasGiantMassThreshold)
            return BuildSurfaceStyle(Pick(gasGiantPalette, variant), gasGiantStyle);

        return BuildSurfaceStyle(Pick(rockyPalette, variant), rockyStyle);
    }

    private static ResolvedStyle BuildSurfaceStyle(Color baseColor, SurfaceStyleSettings settings)
    {
        Color rim = Brighten(baseColor, settings.rimBrightness);

        return new ResolvedStyle
        {
            BaseColor = baseColor,
            DetailColor = baseColor * settings.detailColorScale,
            EmissionColor = baseColor,
            EmissionStrength = settings.emissionStrength,
            RimColor = rim,
            RimStrength = settings.rimStrength,
            RimPower = settings.rimPower,
            DetailStrength = settings.detailStrength,
            BandStrength = settings.bandStrength,
            Smoothness = settings.smoothness,
            TrailColor = rim,
            TrailAlpha = settings.trailAlpha,
            EmitsLight = false,
            HasHalo = false
        };
    }

    private ResolvedStyle BuildStarStyle(float temperature, Color hue)
    {
        temperature = Mathf.Clamp01(temperature);

        Color byTemperature = temperature < 0.5f
            ? Color.Lerp(starStyle.coolColor, starStyle.midColor, temperature * 2f)
            : Color.Lerp(starStyle.midColor, starStyle.hotColor, (temperature - 0.5f) * 2f);

        Color core = Color.Lerp(hue, byTemperature, Mathf.Clamp01(starTemperatureWeight));

        return new ResolvedStyle
        {
            BaseColor = core * starStyle.albedoScale,
            DetailColor = core * starStyle.detailColorScale,
            EmissionColor = core,
            EmissionStrength = Mathf.Lerp(starStyle.emissionStrengthRange.x, starStyle.emissionStrengthRange.y, temperature),
            RimColor = Brighten(core, starStyle.rimBrightness),
            RimStrength = starStyle.rimStrength,
            RimPower = starStyle.rimPower,
            DetailStrength = starStyle.detailStrength,
            BandStrength = starStyle.bandStrength,
            Smoothness = starStyle.smoothness,
            TrailColor = core,
            TrailAlpha = starStyle.trailAlpha,
            EmitsLight = true,
            LightColor = Color.Lerp(starStyle.coolLightColor, starStyle.hotLightColor, temperature),
            LightIntensity = Mathf.Lerp(starStyle.lightIntensityRange.x, starStyle.lightIntensityRange.y, temperature),
            LightRangeMultiplier = starStyle.lightRangeMultiplier,
            HasHalo = true,
            HaloColor = core,
            HaloSize = starStyle.haloSize,
            HaloIntensity = Mathf.Lerp(starStyle.haloIntensityRange.x, starStyle.haloIntensityRange.y, temperature),
            HaloFalloff = starStyle.haloFalloff
        };
    }

    private ResolvedStyle BuildBlackHoleStyle() => new()
    {
        BaseColor = blackHoleStyle.baseColor,
        DetailColor = blackHoleStyle.detailColor,
        EmissionColor = Color.black,
        EmissionStrength = 0f,
        RimColor = blackHoleStyle.ringColor,
        RimStrength = blackHoleStyle.ringStrength,
        RimPower = blackHoleStyle.ringPower,
        DetailStrength = blackHoleStyle.detailStrength,
        BandStrength = 0f,
        Smoothness = blackHoleStyle.smoothness,
        TrailColor = blackHoleStyle.trailColor,
        TrailAlpha = blackHoleStyle.trailAlpha,
        EmitsLight = true,
        LightColor = blackHoleStyle.lightColor,
        LightIntensity = blackHoleStyle.lightIntensity,
        LightRangeMultiplier = blackHoleStyle.lightRangeMultiplier,
        HasHalo = true,
        HaloColor = blackHoleStyle.haloColor,
        HaloSize = blackHoleStyle.haloSize,
        HaloIntensity = blackHoleStyle.haloIntensity,
        HaloFalloff = blackHoleStyle.haloFalloff
    };

    private static Color Pick(Color[] palette, uint variant) => palette[variant % (uint)palette.Length];

    private static Color Brighten(Color color, float factor)
    {
        float peak = Mathf.Max(color.r, Mathf.Max(color.g, color.b));
        if (peak <= MinColorPeak)
            return color;

        return color * (factor / peak);
    }

    private struct ResolvedStyle
    {
        public Color BaseColor;
        public Color DetailColor;
        public Color EmissionColor;
        public float EmissionStrength;
        public Color RimColor;
        public float RimStrength;
        public float RimPower;
        public float DetailStrength;
        public float BandStrength;
        public float Smoothness;

        public Color TrailColor;
        public float TrailAlpha;

        public bool EmitsLight;
        public Color LightColor;
        public float LightIntensity;
        public float LightRangeMultiplier;

        public bool HasHalo;
        public Color HaloColor;
        public float HaloSize;
        public float HaloIntensity;
        public float HaloFalloff;

        public static ResolvedStyle From(BodyVisualProfile profile) => new()
        {
            BaseColor = profile.baseColor,
            DetailColor = profile.detailColor,
            EmissionColor = profile.emissionColor,
            EmissionStrength = profile.emissionStrength,
            RimColor = profile.rimColor,
            RimStrength = profile.rimStrength,
            RimPower = profile.rimPower,
            DetailStrength = profile.detailStrength,
            BandStrength = profile.bandStrength,
            Smoothness = profile.smoothness,
            TrailColor = profile.trailColor,
            TrailAlpha = profile.trailAlpha,
            EmitsLight = profile.emitsLight,
            LightColor = profile.lightColor,
            LightIntensity = profile.lightIntensity,
            LightRangeMultiplier = profile.lightRangeMultiplier,
            HasHalo = profile.hasHalo,
            HaloColor = profile.haloColor,
            HaloSize = profile.haloSize,
            HaloIntensity = profile.haloIntensity,
            HaloFalloff = profile.haloFalloff
        };
    }
}
