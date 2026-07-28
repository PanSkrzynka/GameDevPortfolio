using UnityEngine;

[RequireComponent(typeof(SpriteRenderer))]
public class PlayerAnimator : MonoBehaviour
{
    [SerializeField] private PlayerController controller;
    [SerializeField] private Rigidbody2D body;
    [SerializeField] private Sprite[] idleFrames;
    [SerializeField] private Sprite[] runFrames;
    [SerializeField] private Sprite jumpFrame;
    [SerializeField] private Sprite fallFrame;
    [SerializeField] private float idleFramesPerSecond = 3f;
    [SerializeField] private float runFramesPerSecond = 12f;
    [SerializeField] private float moveThreshold = 0.2f;
    [SerializeField] private float riseThreshold = 0.1f;

    private SpriteRenderer _renderer;
    private float _timer;
    private int _index;

    private void Awake()
    {
        _renderer = GetComponent<SpriteRenderer>();
        if (controller == null) controller = GetComponent<PlayerController>();
        if (body == null) body = GetComponent<Rigidbody2D>();
    }

    private void Update()
    {
        if (body == null) return;

        Vector2 velocity = body.linearVelocity;
        if (Mathf.Abs(velocity.x) > moveThreshold)
        {
            _renderer.flipX = velocity.x < 0f;
        }

        bool grounded = controller == null || controller.Grounded;
        if (!grounded)
        {
            Sprite airborne = velocity.y > riseThreshold ? jumpFrame : fallFrame;
            if (airborne != null) _renderer.sprite = airborne;
            _timer = 0f;
            _index = 0;
            return;
        }

        bool running = Mathf.Abs(velocity.x) > moveThreshold;
        Sprite[] frames = running ? runFrames : idleFrames;
        float fps = running ? runFramesPerSecond : idleFramesPerSecond;
        if (frames == null || frames.Length == 0 || fps <= 0f) return;

        _timer += Time.deltaTime;
        float step = 1f / fps;
        while (_timer >= step)
        {
            _timer -= step;
            _index++;
        }

        _renderer.sprite = frames[_index % frames.Length];
    }
}
