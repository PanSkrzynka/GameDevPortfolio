using System;
using UnityEngine;

public class Timer : MonoBehaviour
{
    private const string DefaultFormat = @"mm\:ss\.ff";

    [SerializeField] private string displayFormat = DefaultFormat;

    private static string _format = DefaultFormat;

    public static float ElapsedTime { get; private set; }
    public static string FormattedElapsedTime => TimeSpan.FromSeconds(ElapsedTime).ToString(_format);

    private void Awake() => _format = displayFormat;

    private void Update() => ElapsedTime += Time.deltaTime;

    public static void ResetElapsed() => ElapsedTime = 0f;

    public static void SetElapsedTime(float elapsedTime) => ElapsedTime = Mathf.Max(0f, elapsedTime);
}
