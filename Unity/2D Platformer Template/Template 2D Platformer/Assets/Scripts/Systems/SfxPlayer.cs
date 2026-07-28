using UnityEngine;

[RequireComponent(typeof(AudioSource))]
public class SfxPlayer : MonoBehaviour
{
    [SerializeField, Range(0f, 1f)] private float masterVolume = 0.7f;

    private static SfxPlayer _instance;
    private AudioSource _source;

    private void Awake()
    {
        _instance = this;
        _source = GetComponent<AudioSource>();
        _source.playOnAwake = false;
        _source.spatialBlend = 0f;
    }

    private void OnDestroy()
    {
        if (_instance == this) _instance = null;
    }

    public static void Play(AudioClip clip, float volume = 1f)
    {
        if (clip == null || _instance == null || _instance._source == null) return;
        _instance._source.PlayOneShot(clip, volume * _instance.masterVolume);
    }
}
