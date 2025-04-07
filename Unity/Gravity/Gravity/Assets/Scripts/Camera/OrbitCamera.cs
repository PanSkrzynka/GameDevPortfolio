using UnityEngine;
using System.Collections.Generic;

public class OrbitCamera : MonoBehaviour
{
    [Header("Target Tracking")]
    public List<Transform> targets = new();
    public int currentIndex = 0;

    [Header("Camera Controls")]
    public float distance = 10f;
    public float zoomSpeed = 2f;
    public float rotationSpeed = 100f;

    private float _yaw = 0f;
    private float _pitch = 20f;

    private const float MinPitch = -80f;
    private const float MaxPitch = 80f;
    private const float MinDistance = 2f;
    private const float MaxDistance = 100f;

    private float _updateInterval = 1f;
    private float _updateTimer = 0f;

    private void Update()
    {
        HandleInput();
        UpdateTargetListPeriodically();
    }

    private void LateUpdate()
    {
        if (targets.Count == 0) return;

        Transform target = targets[currentIndex];
        Vector3 offsetDirection = Quaternion.Euler(_pitch, _yaw, 0) * Vector3.back;
        transform.position = target.position + offsetDirection * distance;
        transform.LookAt(target);
    }

    private void HandleInput()
    {
        if (Input.GetMouseButton(1))
        {
            _yaw += Input.GetAxis("Mouse X") * rotationSpeed * Time.deltaTime;
            _pitch -= Input.GetAxis("Mouse Y") * rotationSpeed * Time.deltaTime;
            _pitch = Mathf.Clamp(_pitch, MinPitch, MaxPitch);
        }

        float scroll = Input.GetAxis("Mouse ScrollWheel");
        distance = Mathf.Clamp(distance - scroll * zoomSpeed, MinDistance, MaxDistance);

        if (Input.GetKeyDown(KeyCode.Q))
            currentIndex = (currentIndex - 1 + targets.Count) % targets.Count;

        if (Input.GetKeyDown(KeyCode.E))
            currentIndex = (currentIndex + 1) % targets.Count;
    }

    private void UpdateTargetListPeriodically()
    {
        _updateTimer += Time.deltaTime;
        if (_updateTimer >= _updateInterval)
        {
            _updateTimer = 0f;
            UpdateTargetList();
        }
    }

    private void UpdateTargetList()
    {
        var allBodies = FindObjectsOfType<GravityObject>();
        targets.Clear();

        foreach (var body in allBodies)
            targets.Add(body.transform);

        currentIndex = Mathf.Clamp(currentIndex, 0, targets.Count - 1);
    }
}