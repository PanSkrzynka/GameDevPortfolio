using System.Collections.Generic;
using UnityEngine;

[CreateAssetMenu(fileName = "InputBindings", menuName = "Platformer/Input Bindings")]
public class InputBindings : ScriptableObject
{
    public readonly struct Binding
    {
        public readonly string Action;
        public readonly string Keys;

        public Binding(string action, string keys)
        {
            Action = action;
            Keys = keys;
        }
    }

    [Header("Movement")]
    [SerializeField] private string horizontalAxis = "Horizontal";
    [SerializeField] private string jumpButton = "Jump";
    [SerializeField] private string horizontalAxisLabel = "A / D";
    [SerializeField] private string jumpButtonLabel = "Space";

    [Header("Actions")]
    [SerializeField] private KeyCode rewind = KeyCode.R;
    [SerializeField] private KeyCode pause = KeyCode.Escape;
    [SerializeField] private KeyCode restart = KeyCode.Return;
    [SerializeField] private KeyCode save = KeyCode.F5;
    [SerializeField] private KeyCode load = KeyCode.F9;

    public string HorizontalAxis => horizontalAxis;
    public string JumpButton => jumpButton;
    public KeyCode Rewind => rewind;
    public KeyCode Pause => pause;
    public KeyCode Restart => restart;
    public KeyCode Save => save;
    public KeyCode Load => load;

    public IReadOnlyList<Binding> Describe()
    {
        return new List<Binding>
        {
            new Binding("Move", horizontalAxisLabel),
            new Binding("Jump", jumpButtonLabel),
            new Binding("Rewind", Label(rewind)),
            new Binding("Pause", Label(pause)),
            new Binding("Restart", Label(restart)),
            new Binding("Save", Label(save)),
            new Binding("Load", Label(load))
        };
    }

    public static string Label(KeyCode key)
    {
        switch (key)
        {
            case KeyCode.Return: return "Enter";
            case KeyCode.Escape: return "Esc";
            default: return key.ToString();
        }
    }
}
