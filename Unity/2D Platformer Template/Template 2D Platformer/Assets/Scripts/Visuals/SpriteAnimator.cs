using UnityEngine;

[RequireComponent(typeof(SpriteRenderer))]
public class SpriteAnimator : MonoBehaviour
{
    [SerializeField] private Sprite[] frames;
    [SerializeField] private float framesPerSecond = 8f;
    [SerializeField] private bool randomizeStart = true;

    private SpriteRenderer _renderer;
    private float _timer;
    private int _index;

    private void Awake()
    {
        _renderer = GetComponent<SpriteRenderer>();
    }

    private void OnEnable()
    {
        if (randomizeStart && frames != null && frames.Length > 0)
        {
            _index = Random.Range(0, frames.Length);
            _timer = Random.value / Mathf.Max(0.01f, framesPerSecond);
        }
    }

    private void Update()
    {
        if (frames == null || frames.Length == 0 || framesPerSecond <= 0f) return;

        _timer += Time.deltaTime;
        float step = 1f / framesPerSecond;
        while (_timer >= step)
        {
            _timer -= step;
            _index = (_index + 1) % frames.Length;
        }

        _renderer.sprite = frames[_index];
    }
}
