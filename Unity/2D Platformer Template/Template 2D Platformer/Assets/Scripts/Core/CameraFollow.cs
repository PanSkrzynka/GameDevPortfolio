using UnityEngine;

[RequireComponent(typeof(Camera))]
public class CameraFollow : MonoBehaviour
{
    [SerializeField] private Transform target;
    [SerializeField] private float followSpeed = 5f;
    [SerializeField] private Vector3 offset = new Vector3(0f, 0f, -10f);

    [Header("Level Bounds")]
    [SerializeField] private bool useBounds;
    [SerializeField] private Vector2 minBounds;
    [SerializeField] private Vector2 maxBounds;

    private Camera _camera;

    private void Awake()
    {
        _camera = GetComponent<Camera>();
    }

    private void LateUpdate()
    {
        if (target == null) return;

        Vector3 desiredPosition = target.position + offset;
        Vector3 next = Vector3.Lerp(transform.position, desiredPosition, followSpeed * Time.deltaTime);
        transform.position = ClampToBounds(next);
    }

    private Vector3 ClampToBounds(Vector3 position)
    {
        if (!useBounds || _camera == null || !_camera.orthographic) return position;

        float halfHeight = _camera.orthographicSize;
        float halfWidth = halfHeight * _camera.aspect;

        float minX = minBounds.x + halfWidth;
        float maxX = maxBounds.x - halfWidth;
        float minY = minBounds.y + halfHeight;
        float maxY = maxBounds.y - halfHeight;

        position.x = minX > maxX ? (minBounds.x + maxBounds.x) * 0.5f : Mathf.Clamp(position.x, minX, maxX);
        position.y = minY > maxY ? (minBounds.y + maxBounds.y) * 0.5f : Mathf.Clamp(position.y, minY, maxY);

        return position;
    }

    private void OnDrawGizmosSelected()
    {
        if (!useBounds) return;

        Vector3 centre = new Vector3((minBounds.x + maxBounds.x) * 0.5f, (minBounds.y + maxBounds.y) * 0.5f, 0f);
        Vector3 size = new Vector3(maxBounds.x - minBounds.x, maxBounds.y - minBounds.y, 0f);
        Gizmos.color = Color.cyan;
        Gizmos.DrawWireCube(centre, size);
    }
}
