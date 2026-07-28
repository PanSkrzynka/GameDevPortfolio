using UnityEngine;

public class HoverMotion : MonoBehaviour
{
    [SerializeField] private float amplitude = 0.12f;
    [SerializeField] private float cyclesPerSecond = 0.6f;
    [SerializeField] private float tiltDegrees = 0f;

    private Vector3 _origin;
    private float _phase;

    private void Awake()
    {
        _origin = transform.localPosition;
        _phase = Random.value * Mathf.PI * 2f;
    }

    private void OnDisable()
    {
        transform.localPosition = _origin;
    }

    private void Update()
    {
        float wave = Mathf.Sin(Time.time * cyclesPerSecond * Mathf.PI * 2f + _phase);
        transform.localPosition = _origin + new Vector3(0f, wave * amplitude, 0f);

        if (!Mathf.Approximately(tiltDegrees, 0f))
        {
            transform.localRotation = Quaternion.Euler(0f, 0f, wave * tiltDegrees);
        }
    }
}
