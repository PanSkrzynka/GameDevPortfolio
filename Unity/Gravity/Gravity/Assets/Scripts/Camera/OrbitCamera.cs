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
    [SerializeField, Min(0.01f)] private float followSmoothing = 0.08f;
    [SerializeField, Min(1f)] private float rotationSmoothing = 12f;
    [SerializeField] private bool enableAutoRotate = true;
    [SerializeField, Min(0f)] private float autoRotateSpeed = 5f;
    [SerializeField] private bool showShowcaseOverlay = true;
    [SerializeField] private KeyCode resetViewKey = KeyCode.F;
    [SerializeField] private KeyCode pauseSimulationKey = KeyCode.Space;

    private float _yaw = 0f;
    private float _pitch = 20f;
    private Vector3 _cameraSmoothVelocity;
    private float _lastManualRotateTime;
    private BarnesHutGravityManager _gravityManager;

    private const float MinPitch = -80f;
    private const float MaxPitch = 80f;
    private const float MinDistance = 2f;
    private const float MaxDistance = 100f;

    private float _updateInterval = 1f;
    private float _updateTimer = 0f;
    private GUIStyle _panelStyle;
    private GUIStyle _titleStyle;
    private GUIStyle _labelStyle;
    private Texture2D _panelTexture;

    private void Start()
    {
        UpdateTargetList();
    }

    private void Update()
    {
        HandleInput();
        UpdateTargetListPeriodically();
        HandleSimulationControls();
    }

    private void LateUpdate()
    {
        Transform target = GetCurrentTarget();
        if (target == null)
            return;

        Vector3 offsetDirection = Quaternion.Euler(_pitch, _yaw, 0f) * Vector3.back;
        Vector3 desiredPosition = target.position + offsetDirection * distance;
        transform.position = Vector3.SmoothDamp(
            transform.position,
            desiredPosition,
            ref _cameraSmoothVelocity,
            followSmoothing);

        Vector3 lookDirection = target.position - transform.position;
        if (lookDirection.sqrMagnitude > Mathf.Epsilon)
        {
            Quaternion desiredRotation = Quaternion.LookRotation(lookDirection, Vector3.up);
            float t = 1f - Mathf.Exp(-rotationSmoothing * Time.deltaTime);
            transform.rotation = Quaternion.Slerp(transform.rotation, desiredRotation, t);
        }
    }

    private void HandleInput()
    {
        bool hasTargets = targets.Count > 0;

        if (Input.GetMouseButton(1))
        {
            _yaw += Input.GetAxis("Mouse X") * rotationSpeed * Time.deltaTime;
            _pitch -= Input.GetAxis("Mouse Y") * rotationSpeed * Time.deltaTime;
            _pitch = Mathf.Clamp(_pitch, MinPitch, MaxPitch);
            _lastManualRotateTime = Time.time;
        }
        else if (enableAutoRotate && hasTargets && Time.time - _lastManualRotateTime > 1.5f)
        {
            _yaw += autoRotateSpeed * Time.deltaTime;
        }

        float scroll = Input.GetAxis("Mouse ScrollWheel");
        if (!Mathf.Approximately(scroll, 0f))
            distance = Mathf.Clamp(distance - scroll * zoomSpeed * Mathf.Max(1f, distance * 0.15f), MinDistance, MaxDistance);

        if (hasTargets && Input.GetKeyDown(KeyCode.Q))
            currentIndex = (currentIndex - 1 + targets.Count) % targets.Count;

        if (hasTargets && Input.GetKeyDown(KeyCode.E))
            currentIndex = (currentIndex + 1) % targets.Count;

        if (hasTargets && Input.GetKeyDown(resetViewKey))
        {
            currentIndex = 0;
            distance = Mathf.Clamp(distance, MinDistance, MaxDistance);
        }
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
        Transform previousTarget = GetCurrentTarget();
        targets.Clear();

        foreach (var body in GravityManager.Objects)
        {
            if (body != null)
                targets.Add(body.transform);
        }

        targets.Sort((left, right) =>
        {
            float leftMass = TryGetMass(left);
            float rightMass = TryGetMass(right);
            return rightMass.CompareTo(leftMass);
        });

        if (targets.Count == 0)
        {
            currentIndex = 0;
            return;
        }

        if (previousTarget != null)
        {
            int previousIndex = targets.IndexOf(previousTarget);
            if (previousIndex >= 0)
            {
                currentIndex = previousIndex;
                return;
            }
        }

        currentIndex = Mathf.Clamp(currentIndex, 0, targets.Count - 1);
    }

    private void HandleSimulationControls()
    {
        if (_gravityManager == null)
            _gravityManager = FindFirstObjectByType<BarnesHutGravityManager>();

        if (_gravityManager == null)
            return;

        if (Input.GetKeyDown(pauseSimulationKey))
        {
            float nextScale = Mathf.Approximately(_gravityManager.SimulationTimeScale, 0f) ? 1f : 0f;
            _gravityManager.SetSimulationTimeScale(nextScale);
        }

        if (Input.GetKeyDown(KeyCode.Minus) || Input.GetKeyDown(KeyCode.KeypadMinus))
            _gravityManager.SetSimulationTimeScale(_gravityManager.SimulationTimeScale - 0.25f);

        if (Input.GetKeyDown(KeyCode.Equals) || Input.GetKeyDown(KeyCode.KeypadPlus))
            _gravityManager.SetSimulationTimeScale(_gravityManager.SimulationTimeScale + 0.25f);
    }

    private Transform GetCurrentTarget()
    {
        if (targets.Count == 0)
            return null;

        currentIndex = Mathf.Clamp(currentIndex, 0, targets.Count - 1);
        return targets[currentIndex];
    }

    private static float TryGetMass(Transform bodyTransform)
    {
        if (bodyTransform != null && bodyTransform.TryGetComponent(out GravityObject gravityObject))
            return gravityObject.mass;

        return 1f;
    }

    private void OnGUI()
    {
        if (!showShowcaseOverlay)
            return;

        EnsureOverlayStyles();

        Transform target = GetCurrentTarget();
        string targetName = target != null ? target.name : "None";
        float targetMass = TryGetMass(target);
        float simScale = _gravityManager != null ? _gravityManager.SimulationTimeScale : 1f;

        Rect panelRect = new(14f, 14f, 380f, 144f);
        GUI.Box(panelRect, GUIContent.none, _panelStyle);

        GUILayout.BeginArea(new Rect(panelRect.x + 12f, panelRect.y + 10f, panelRect.width - 24f, panelRect.height - 16f));
        GUILayout.Label("GRAVITY SHOWCASE", _titleStyle);
        GUILayout.Label($"Bodies: {targets.Count}  |  Target: {targetName}", _labelStyle);
        GUILayout.Label($"Target Mass: {targetMass:0.##}  |  Sim Speed: {simScale:0.##}x", _labelStyle);
        GUILayout.Label("RMB rotate  |  Scroll zoom  |  Q/E switch target  |  F focus main body", _labelStyle);
        GUILayout.Label("Space pause/resume  |  +/- simulation speed", _labelStyle);
        GUILayout.EndArea();
    }

    private void EnsureOverlayStyles()
    {
        if (_panelStyle != null)
            return;

        _panelTexture = new Texture2D(1, 1);
        _panelTexture.SetPixel(0, 0, new Color(0.05f, 0.08f, 0.14f, 0.78f));
        _panelTexture.Apply();

        _panelStyle = new GUIStyle(GUI.skin.box)
        {
            normal = { background = _panelTexture },
            border = new RectOffset(8, 8, 8, 8),
            padding = new RectOffset(8, 8, 8, 8)
        };

        _titleStyle = new GUIStyle(GUI.skin.label)
        {
            fontSize = 15,
            fontStyle = FontStyle.Bold,
            normal = { textColor = new Color(0.84f, 0.93f, 1f, 1f) }
        };

        _labelStyle = new GUIStyle(GUI.skin.label)
        {
            fontSize = 12,
            normal = { textColor = new Color(0.85f, 0.9f, 0.96f, 1f) }
        };
    }
}
