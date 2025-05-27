using UnityEngine;

[RequireComponent(typeof(Rigidbody2D), typeof(Collider2D))]
public class PlayerController : MonoBehaviour
{
    [Header("Movement Settings")]
    [SerializeField] private float moveSpeed = 8f;
    [SerializeField] private float jumpForce = 14f;
    [SerializeField] private LayerMask groundLayer;
    [SerializeField] private float coyoteTime = 0.1f; // Time window to allow jump after leaving ground

    private Rigidbody2D _rigidbody;
    private Collider2D _collider;
    private float _horizontalInput;
    private float _lastGroundedTime;
    private bool _jumpRequested;

    private void Awake()
    {
        _rigidbody = GetComponent<Rigidbody2D>();
        _collider = GetComponent<Collider2D>();
    }

    private void Update()
    {
        _horizontalInput = Input.GetAxisRaw("Horizontal");

        if (IsGrounded())
        {
            _lastGroundedTime = Time.time;
        }

        if (Input.GetButtonDown("Jump"))
        {
            _jumpRequested = true;
        }
    }

    private void FixedUpdate()
    {
        _rigidbody.velocity = new Vector2(_horizontalInput * moveSpeed, _rigidbody.velocity.y);

        if (_jumpRequested && (Time.time - _lastGroundedTime <= coyoteTime))
        {
            _rigidbody.velocity = new Vector2(_rigidbody.velocity.x, jumpForce);
            _jumpRequested = false;
        }
    }

    private bool IsGrounded()
    {
        Vector2 origin = new Vector2(transform.position.x, _collider.bounds.min.y);
        float rayLength = 0.1f;
        RaycastHit2D hit = Physics2D.Raycast(origin, Vector2.down, rayLength, groundLayer);
        return hit.collider != null;
    }

    private void OnDrawGizmosSelected()
    {
        if (_collider == null) return;

        Vector2 origin = new Vector2(transform.position.x, _collider.bounds.min.y);
        float rayLength = 0.1f;
        Gizmos.color = Color.yellow;
        Gizmos.DrawLine(origin, origin + Vector2.down * rayLength);
    }
}