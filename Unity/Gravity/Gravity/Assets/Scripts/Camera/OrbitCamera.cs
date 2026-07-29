using UnityEngine;
using System.Collections.Generic;

public class OrbitCamera : MonoBehaviour
{
    private const float SphereRadiusPerUnitScale = 0.5f;
    private const float MinTargetRadius = 0.05f;
    private const float FallbackTargetRadius = 0.5f;
    private const float FallbackMass = 1f;
    private const float MinZoomStep = 1f;

    [Header("Target Tracking")]
    public List<Transform> targets = new();
    public int currentIndex = 0;

    [Tooltip("How often the tracked body list is rebuilt, in seconds.")]
    [SerializeField, Min(0.05f)] private float targetRefreshInterval = 1f;

    [Header("Camera Controls")]
    public float distance = 10f;
    public float zoomSpeed = 2f;
    public float rotationSpeed = 100f;

    [Tooltip("Framing distance as a multiple of the target's radius.")]
    [SerializeField, Min(1.5f)] private float framingDistanceMultiplier = 6.5f;

    [Tooltip("Closest approach as a multiple of the target's radius.")]
    [SerializeField, Min(1f)] private float minDistanceRadiusMultiplier = 1.35f;

    [Tooltip("Furthest pull-back as a multiple of the target's radius.")]
    [SerializeField, Min(1f)] private float maxDistanceRadiusMultiplier = 25f;

    [Tooltip("Absolute zoom bounds, used when the target is small enough that its radius does not dominate.")]
    [SerializeField] private Vector2 distanceLimits = new(2f, 100f);

    [SerializeField] private Vector2 pitchLimits = new(-80f, 80f);

    [Tooltip("Scales zoom steps with the current distance so far-out zooming is not sluggish.")]
    [SerializeField, Min(0f)] private float zoomDistanceScale = 0.15f;

    [SerializeField] private float initialPitch = 20f;
    [SerializeField, Min(0.01f)] private float followSmoothing = 0.08f;
    [SerializeField, Min(1f)] private float rotationSmoothing = 12f;
    [SerializeField] private bool enableAutoRotate = true;
    [SerializeField, Min(0f)] private float autoRotateSpeed = 5f;

    [Tooltip("Idle time after a manual rotate before auto-rotation resumes, in seconds.")]
    [SerializeField, Min(0f)] private float autoRotateIdleDelay = 1.5f;

    [Header("Input")]
    [SerializeField] private KeyCode previousTargetKey = KeyCode.Q;
    [SerializeField] private KeyCode nextTargetKey = KeyCode.E;
    [SerializeField] private KeyCode resetViewKey = KeyCode.F;
    [SerializeField] private KeyCode pauseSimulationKey = KeyCode.Space;
    [SerializeField] private KeyCode slowDownKey = KeyCode.Minus;
    [SerializeField] private KeyCode slowDownAltKey = KeyCode.KeypadMinus;
    [SerializeField] private KeyCode speedUpKey = KeyCode.Equals;
    [SerializeField] private KeyCode speedUpAltKey = KeyCode.KeypadPlus;
    [SerializeField, Min(0.01f)] private float simulationSpeedStep = 0.25f;

    [Header("Showcase Overlay")]
    [SerializeField] private bool showShowcaseOverlay = true;
    [SerializeField] private string overlayTitle = "GRAVITY SHOWCASE";
    [SerializeField] private Rect overlayRect = new(14f, 14f, 380f, 144f);
    [SerializeField, Min(0)] private int overlayPadding = 8;
    [SerializeField] private Vector2 overlayContentInset = new(12f, 10f);
    [SerializeField, Min(1)] private int overlayTitleFontSize = 15;
    [SerializeField, Min(1)] private int overlayLabelFontSize = 12;
    [SerializeField] private Color overlayPanelColor = new(0.05f, 0.08f, 0.14f, 0.78f);
    [SerializeField] private Color overlayTitleColor = new(0.84f, 0.93f, 1f, 1f);
    [SerializeField] private Color overlayLabelColor = new(0.85f, 0.9f, 0.96f, 1f);

    private float _yaw = 0f;
    private float _pitch;
    private Vector3 _cameraSmoothVelocity;
    private float _lastManualRotateTime;
    private BarnesHutGravityManager _gravityManager;
    private Transform _framedTarget;

    private float _updateTimer = 0f;
    private GUIStyle _panelStyle;
    private GUIStyle _titleStyle;
    private GUIStyle _labelStyle;
    private Texture2D _panelTexture;

    private void Awake()
    {
        _pitch = initialPitch;
    }

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

        FrameTargetIfChanged(target);

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
            _pitch = Mathf.Clamp(_pitch, pitchLimits.x, pitchLimits.y);
            _lastManualRotateTime = Time.time;
        }
        else if (enableAutoRotate && hasTargets && Time.time - _lastManualRotateTime > autoRotateIdleDelay)
        {
            _yaw += autoRotateSpeed * Time.deltaTime;
        }

        float scroll = Input.GetAxis("Mouse ScrollWheel");
        if (!Mathf.Approximately(scroll, 0f))
        {
            float radius = GetTargetRadius(GetCurrentTarget());
            float step = scroll * zoomSpeed * Mathf.Max(MinZoomStep, distance * zoomDistanceScale);
            distance = Mathf.Clamp(distance - step, GetMinDistance(radius), GetMaxDistance(radius));
        }

        if (hasTargets && Input.GetKeyDown(previousTargetKey))
            currentIndex = (currentIndex - 1 + targets.Count) % targets.Count;

        if (hasTargets && Input.GetKeyDown(nextTargetKey))
            currentIndex = (currentIndex + 1) % targets.Count;

        if (hasTargets && Input.GetKeyDown(resetViewKey))
        {
            currentIndex = 0;
            _framedTarget = null;
        }
    }

    private void FrameTargetIfChanged(Transform target)
    {
        if (target == _framedTarget)
            return;

        _framedTarget = target;

        float radius = GetTargetRadius(target);
        distance = Mathf.Clamp(
            radius * framingDistanceMultiplier,
            GetMinDistance(radius),
            GetMaxDistance(radius));
    }

    private static float GetTargetRadius(Transform target)
    {
        if (target == null)
            return FallbackTargetRadius;

        Vector3 scale = target.lossyScale;
        float largest = Mathf.Max(scale.x, Mathf.Max(scale.y, scale.z));
        return Mathf.Max(MinTargetRadius, largest * SphereRadiusPerUnitScale);
    }

    private float GetMinDistance(float radius) =>
        Mathf.Max(distanceLimits.x, radius * minDistanceRadiusMultiplier);

    private float GetMaxDistance(float radius) =>
        Mathf.Max(distanceLimits.y, radius * maxDistanceRadiusMultiplier);

    private void UpdateTargetListPeriodically()
    {
        _updateTimer += Time.deltaTime;
        if (_updateTimer >= targetRefreshInterval)
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

        if (Input.GetKeyDown(slowDownKey) || Input.GetKeyDown(slowDownAltKey))
            _gravityManager.SetSimulationTimeScale(_gravityManager.SimulationTimeScale - simulationSpeedStep);

        if (Input.GetKeyDown(speedUpKey) || Input.GetKeyDown(speedUpAltKey))
            _gravityManager.SetSimulationTimeScale(_gravityManager.SimulationTimeScale + simulationSpeedStep);
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

        return FallbackMass;
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

        GUI.Box(overlayRect, GUIContent.none, _panelStyle);

        GUILayout.BeginArea(new Rect(
            overlayRect.x + overlayContentInset.x,
            overlayRect.y + overlayContentInset.y,
            overlayRect.width - overlayContentInset.x * 2f,
            overlayRect.height - overlayContentInset.y * 2f));

        GUILayout.Label(overlayTitle, _titleStyle);
        GUILayout.Label($"Bodies: {targets.Count}  |  Target: {targetName}", _labelStyle);
        GUILayout.Label($"Target Mass: {targetMass:0.##}  |  Sim Speed: {simScale:0.##}x", _labelStyle);
        GUILayout.Label($"RMB rotate  |  Scroll zoom  |  {previousTargetKey}/{nextTargetKey} switch target  |  {resetViewKey} focus main body", _labelStyle);
        GUILayout.Label($"{pauseSimulationKey} pause/resume  |  {slowDownKey}/{speedUpKey} simulation speed", _labelStyle);
        GUILayout.EndArea();
    }

    private void EnsureOverlayStyles()
    {
        if (_panelStyle != null)
            return;

        _panelTexture = new Texture2D(1, 1);
        _panelTexture.SetPixel(0, 0, overlayPanelColor);
        _panelTexture.Apply();

        _panelStyle = new GUIStyle(GUI.skin.box)
        {
            normal = { background = _panelTexture },
            border = new RectOffset(overlayPadding, overlayPadding, overlayPadding, overlayPadding),
            padding = new RectOffset(overlayPadding, overlayPadding, overlayPadding, overlayPadding)
        };

        _titleStyle = new GUIStyle(GUI.skin.label)
        {
            fontSize = overlayTitleFontSize,
            fontStyle = FontStyle.Bold,
            normal = { textColor = overlayTitleColor }
        };

        _labelStyle = new GUIStyle(GUI.skin.label)
        {
            fontSize = overlayLabelFontSize,
            normal = { textColor = overlayLabelColor }
        };
    }
}
