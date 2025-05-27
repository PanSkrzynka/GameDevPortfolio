using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class Timer : MonoBehaviour
{
    public static float ElapsedTime { get; private set; }

    private void Update() => ElapsedTime += Time.deltaTime;

    public static void Reset() => ElapsedTime = 0f;
}
