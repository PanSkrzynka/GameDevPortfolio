using UnityEngine;

public class BarnesHutGravityManager : MonoBehaviour
{
    [SerializeField] private float gravitationalConstant = 0.01f;
    [SerializeField] private float softening = 0.5f;
    [SerializeField] private float theta = 0.5f;
    [SerializeField] private float timeStep = 0.01f;
    [SerializeField] private float timeScale = 1f;
    [SerializeField] private float treeSize = 1000f;

    private void Update()
    {
        SimulateStep(Time.deltaTime * timeScale);
    }

    private void SimulateStep(float deltaTime)
    {
        Bounds simulationBounds = new(Vector3.zero, Vector3.one * treeSize);
        var tree = new BarnesHutTree(simulationBounds, theta, softening, gravitationalConstant);

        foreach (var obj in GravityManager.Objects)
            tree.Insert(obj);

        foreach (var obj in GravityManager.Objects)
            tree.ApplyForce(obj);

        foreach (var obj in GravityManager.Objects)
            obj.Integrate(deltaTime);
    }
}