using UnityEngine;
using UnityEngine.Serialization;

public class BarnesHutGravityManager : MonoBehaviour
{
    [FormerlySerializedAs("gravity")]
    [SerializeField, Min(0f)] private float gravitationalConstant = 0.01f;
    [SerializeField, Min(0.0001f)] private float softening = 0.5f;
    [SerializeField, Min(0.0001f)] private float theta = 0.5f;
    [FormerlySerializedAs("step")]
    [SerializeField, Min(0.0001f)] private float timeStep = 0.01f;
    [SerializeField, Min(0f)] private float timeScale = 1f;
    [FormerlySerializedAs("boundsSize")]
    [SerializeField, Min(1f)] private float treeSize = 1000f;
    [SerializeField, Min(1)] private int maxSubStepsPerFrame = 8;

    private float _accumulator;

    public float SimulationTimeScale => timeScale;

    public void SetSimulationTimeScale(float newTimeScale)
    {
        timeScale = Mathf.Max(0f, newTimeScale);
    }

    private void Update()
    {
        GravityManager.CleanupNullReferences();

        if (GravityManager.Objects.Count == 0 || timeScale <= 0f)
            return;

        float fixedStep = Mathf.Max(0.0001f, timeStep);
        _accumulator += Time.deltaTime * timeScale;

        int subSteps = 0;
        while (_accumulator >= fixedStep && subSteps < maxSubStepsPerFrame)
        {
            SimulateStep(fixedStep);
            _accumulator -= fixedStep;
            subSteps++;
        }

        if (subSteps == maxSubStepsPerFrame)
            _accumulator = 0f;
    }

    private void SimulateStep(float deltaTime)
    {
        Bounds simulationBounds = new(Vector3.zero, Vector3.one * Mathf.Max(1f, treeSize));
        var tree = new BarnesHutTree(simulationBounds, theta, softening, gravitationalConstant);

        foreach (var obj in GravityManager.Objects)
        {
            if (obj != null)
                tree.Insert(obj);
        }

        foreach (var obj in GravityManager.Objects)
        {
            if (obj != null)
                tree.ApplyForce(obj);
        }

        foreach (var obj in GravityManager.Objects)
        {
            if (obj != null)
                obj.Integrate(deltaTime);
        }
    }
}
