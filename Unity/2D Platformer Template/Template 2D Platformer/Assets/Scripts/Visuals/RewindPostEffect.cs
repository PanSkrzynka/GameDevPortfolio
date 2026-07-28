using UnityEngine;

[RequireComponent(typeof(Camera))]
public class RewindPostEffect : MonoBehaviour
{
    [SerializeField] private Shader effectShader;
    [SerializeField] private TimeRewind rewind;
    [SerializeField] private float fadeInSpeed = 14f;
    [SerializeField] private float fadeOutSpeed = 5f;
    [SerializeField, Range(0f, 1f)] private float maxStrength = 1f;
    [SerializeField] private float minVisibleStrength = 0.001f;

    private static readonly int StrengthProperty = Shader.PropertyToID("_Strength");
    private static readonly int PhaseProperty = Shader.PropertyToID("_Phase");

    private Material _material;
    private float _strength;
    private float _phase;

    private void OnDisable()
    {
        if (_material != null)
        {
            DestroyImmediate(_material);
            _material = null;
        }
        _strength = 0f;
    }

    private void Update()
    {
        if (rewind == null)
        {
            rewind = FindFirstObjectByType<TimeRewind>();
        }

        bool active = rewind != null && rewind.IsRewinding;
        float target = active ? maxStrength : 0f;
        float speed = active ? fadeInSpeed : fadeOutSpeed;

        _strength = Mathf.MoveTowards(_strength, target, speed * Time.unscaledDeltaTime);
        _phase += Time.unscaledDeltaTime;
    }

    private void OnRenderImage(RenderTexture source, RenderTexture destination)
    {
        if (effectShader == null || _strength <= minVisibleStrength)
        {
            Graphics.Blit(source, destination);
            return;
        }

        if (_material == null)
        {
            _material = new Material(effectShader) { hideFlags = HideFlags.HideAndDontSave };
        }

        _material.SetFloat(StrengthProperty, _strength);
        _material.SetFloat(PhaseProperty, _phase);
        Graphics.Blit(source, destination, _material);
    }
}
