using UnityEngine;
using UnityEngine.Serialization;

[RequireComponent(typeof(Rigidbody2D), typeof(Collider2D))]
public class PlayerController : MonoBehaviour
{
    [Header("Input")]
    [SerializeField] private InputBindings bindings;

    [Header("Movement")]
    [SerializeField] private float moveSpeed = 8f;
    [SerializeField] private float jumpForce = 14f;
    [SerializeField] private float coyoteTime = 0.15f;
    [SerializeField] private float jumpBufferTime = 0.15f;

    [Header("Ground Check")]
    [SerializeField] private LayerMask groundLayer;
    [FormerlySerializedAs("groundedCheckDistance")]
    [SerializeField] private float groundCheckDistance = 0.1f;
    [SerializeField, Range(0.1f, 1f)] private float groundCheckWidthRatio = 0.9f;
    [SerializeField] private float groundCheckThickness = 0.05f;
    [SerializeField] private float landingVelocityThreshold = 0.1f;

    [Header("Audio")]
    [SerializeField] private AudioClip jumpSound;
    [SerializeField] private AudioClip landSound;

    private Rigidbody2D _rigidbody;
    private Collider2D _collider;
    private float _horizontalInput;
    private float _lastGroundedTime = float.NegativeInfinity;
    private float _lastJumpPressedTime = float.NegativeInfinity;

    public bool Grounded { get; private set; }

    private void Awake()
    {
        _rigidbody = GetComponent<Rigidbody2D>();
        _collider = GetComponent<Collider2D>();
    }

    private void Update()
    {
        _horizontalInput = bindings == null ? 0f : Input.GetAxisRaw(bindings.HorizontalAxis);

        bool wasGrounded = Grounded;
        Grounded = IsGrounded();

        if (Grounded)
        {
            if (!wasGrounded && _rigidbody.linearVelocity.y <= landingVelocityThreshold)
            {
                SfxPlayer.Play(landSound);
            }

            _lastGroundedTime = Time.time;
        }

        if (bindings != null && Input.GetButtonDown(bindings.JumpButton))
        {
            _lastJumpPressedTime = Time.time;
        }
    }

    private void FixedUpdate()
    {
        _rigidbody.linearVelocity = new Vector2(_horizontalInput * moveSpeed, _rigidbody.linearVelocity.y);

        bool withinCoyoteWindow = Time.time - _lastGroundedTime <= coyoteTime;
        bool jumpBuffered = Time.time - _lastJumpPressedTime <= jumpBufferTime;

        if (!jumpBuffered || !withinCoyoteWindow) return;

        _rigidbody.linearVelocity = new Vector2(_rigidbody.linearVelocity.x, jumpForce);
        _lastJumpPressedTime = float.NegativeInfinity;
        _lastGroundedTime = float.NegativeInfinity;
        SfxPlayer.Play(jumpSound);
    }

    private bool IsGrounded()
    {
        GetGroundProbe(out Vector2 origin, out Vector2 size);
        return Physics2D.BoxCast(origin, size, 0f, Vector2.down, groundCheckDistance, groundLayer).collider != null;
    }

    private void GetGroundProbe(out Vector2 origin, out Vector2 size)
    {
        Bounds bounds = _collider.bounds;
        size = new Vector2(bounds.size.x * groundCheckWidthRatio, groundCheckThickness);
        origin = new Vector2(bounds.center.x, bounds.min.y - size.y * 0.5f);
    }

    private void OnDrawGizmosSelected()
    {
        if (_collider == null) _collider = GetComponent<Collider2D>();
        if (_collider == null) return;

        GetGroundProbe(out Vector2 origin, out Vector2 size);
        Gizmos.color = Color.yellow;
        Gizmos.DrawWireCube(origin, size);
        Gizmos.DrawWireCube(origin + Vector2.down * groundCheckDistance, size);
    }

    private void OnValidate()
    {
        moveSpeed = Mathf.Max(0f, moveSpeed);
        jumpForce = Mathf.Max(0f, jumpForce);
        coyoteTime = Mathf.Max(0f, coyoteTime);
        jumpBufferTime = Mathf.Max(0f, jumpBufferTime);
        groundCheckDistance = Mathf.Max(0.01f, groundCheckDistance);
        groundCheckThickness = Mathf.Max(0.01f, groundCheckThickness);
    }
}
