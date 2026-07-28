using UnityEngine;
using UnityEngine.Serialization;

[ExecuteAlways]
public class Parallax : MonoBehaviour
{
    [SerializeField] private Transform cameraTransform;
    [FormerlySerializedAs("factor")]
    [SerializeField] private Vector2 parallaxFactor = new Vector2(0.5f, 0.2f);

    private Vector3 _origin;
    private Vector3 _cameraOrigin;

    private void OnEnable()
    {
        if (cameraTransform == null && Camera.main != null)
        {
            cameraTransform = Camera.main.transform;
        }

        _origin = transform.position;
        if (cameraTransform != null)
        {
            _cameraOrigin = cameraTransform.position;
        }
    }

    private void LateUpdate()
    {
        if (cameraTransform == null) return;

        Vector3 delta = cameraTransform.position - _cameraOrigin;
        transform.position = new Vector3(
            _origin.x + delta.x * parallaxFactor.x,
            _origin.y + delta.y * parallaxFactor.y,
            _origin.z);
    }
}
