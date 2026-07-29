using UnityEngine;

[ExecuteAlways]
[RequireComponent(typeof(Camera))]
[AddComponentMenu("Gravity/Rendering/Bloom Effect")]
[ImageEffectAllowedInSceneView]
public class BloomEffect : MonoBehaviour
{
    private const int MaxPyramidLevels = 16;
    private const int MinPyramidSize = 2;
    private const int PyramidBaseDivisor = 2;
    private const int HalfResolutionDivisor = 2;
    private const float MinKnee = 1e-4f;
    private const float KneeRangeScale = 2f;
    private const float KneeCurveScale = 0.25f;
    private const string ShaderName = "Hidden/Gravity/Bloom";

    private const int PassPrefilter = 0;
    private const int PassDownsample = 1;
    private const int PassUpsample = 2;
    private const int PassComposite = 3;

    [Header("Threshold")]
    [Tooltip("Luminance a pixel must reach before it starts to bloom. Emissive bodies sit above 1.")]
    [SerializeField, Min(0f)] private float threshold = 0.95f;

    [Tooltip("Widens the threshold into a gradient so bodies fade into glowing instead of popping.")]
    [SerializeField, Range(0f, 1f)] private float softKnee = 0.6f;

    [Header("Appearance")]
    [SerializeField, Min(0f)] private float intensity = 1.15f;

    [Tooltip("How far the glow spreads. Each level roughly doubles the radius.")]
    [SerializeField, Range(1, 12)] private int diffusion = 6;

    [Tooltip("Spread of the upsample filter. Above ~1.5 the glow starts to look blocky.")]
    [SerializeField, Range(0.5f, 2f)] private float sampleScale = 1f;

    [SerializeField, ColorUsage(false, false)] private Color tint = Color.white;

    [Header("Performance")]
    [Tooltip("Halves the resolution the pyramid starts at. Cheaper, slightly softer glow.")]
    [SerializeField] private bool halfResolution = true;

    private static readonly int FilterParamsId = Shader.PropertyToID("_FilterParams");
    private static readonly int SampleScaleId = Shader.PropertyToID("_SampleScale");
    private static readonly int IntensityId = Shader.PropertyToID("_Intensity");
    private static readonly int TintId = Shader.PropertyToID("_Tint");
    private static readonly int BloomTexId = Shader.PropertyToID("_BloomTex");

    private readonly RenderTexture[] _pyramid = new RenderTexture[MaxPyramidLevels];

    private Material _material;
    private Camera _camera;
    private bool _warnedAboutShader;

    private void OnEnable()
    {
        _camera = GetComponent<Camera>();

        if (_camera != null)
            _camera.allowHDR = true;

        EnsureMaterial();
    }

    private void OnDisable()
    {
        ReleasePyramid();

        if (_material == null)
            return;

        if (Application.isPlaying)
            Destroy(_material);
        else
            DestroyImmediate(_material);

        _material = null;
    }

    private void EnsureMaterial()
    {
        if (_material != null)
            return;

        Shader shader = Shader.Find(ShaderName);
        if (shader == null || !shader.isSupported)
        {
            if (!_warnedAboutShader)
            {
                Debug.LogWarning($"{nameof(BloomEffect)} could not find a usable '{ShaderName}' shader; bloom is disabled.", this);
                _warnedAboutShader = true;
            }

            return;
        }

        _material = new Material(shader) { hideFlags = HideFlags.HideAndDontSave };
    }

    private void OnRenderImage(RenderTexture source, RenderTexture destination)
    {
        EnsureMaterial();

        if (_material == null || intensity <= 0f)
        {
            Graphics.Blit(source, destination);
            return;
        }

        int divider = PyramidBaseDivisor * (halfResolution ? HalfResolutionDivisor : 1);
        int width = Mathf.Max(1, source.width / divider);
        int height = Mathf.Max(1, source.height / divider);

        if (width < MinPyramidSize || height < MinPyramidSize)
        {
            Graphics.Blit(source, destination);
            return;
        }

        RenderTextureFormat format = source.format;

        float knee = Mathf.Max(threshold * softKnee, MinKnee);
        _material.SetVector(FilterParamsId, new Vector4(
            threshold,
            threshold - knee,
            KneeRangeScale * knee,
            KneeCurveScale / knee));
        _material.SetFloat(SampleScaleId, sampleScale);
        _material.SetFloat(IntensityId, intensity);
        _material.SetColor(TintId, tint);

        _pyramid[0] = RenderTexture.GetTemporary(width, height, 0, format);
        _pyramid[0].filterMode = FilterMode.Bilinear;
        Graphics.Blit(source, _pyramid[0], _material, PassPrefilter);

        int levelCount = 1;
        for (int level = 1; level < Mathf.Min(diffusion, MaxPyramidLevels); level++)
        {
            width = Mathf.Max(1, width / PyramidBaseDivisor);
            height = Mathf.Max(1, height / PyramidBaseDivisor);

            if (width < MinPyramidSize || height < MinPyramidSize)
                break;

            _pyramid[level] = RenderTexture.GetTemporary(width, height, 0, format);
            _pyramid[level].filterMode = FilterMode.Bilinear;
            Graphics.Blit(_pyramid[level - 1], _pyramid[level], _material, PassDownsample);
            levelCount++;
        }

        for (int level = levelCount - 2; level >= 0; level--)
            Graphics.Blit(_pyramid[level + 1], _pyramid[level], _material, PassUpsample);

        _material.SetTexture(BloomTexId, _pyramid[0]);
        Graphics.Blit(source, destination, _material, PassComposite);

        ReleasePyramid();
    }

    private void ReleasePyramid()
    {
        for (int level = 0; level < MaxPyramidLevels; level++)
        {
            if (_pyramid[level] == null)
                continue;

            RenderTexture.ReleaseTemporary(_pyramid[level]);
            _pyramid[level] = null;
        }
    }
}
