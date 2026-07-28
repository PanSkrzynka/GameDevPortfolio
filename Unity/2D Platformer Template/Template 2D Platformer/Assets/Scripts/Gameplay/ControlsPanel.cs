using System.Collections.Generic;
using System.Text;
using TMPro;
using UnityEngine;

public class ControlsPanel : MonoBehaviour
{
    [SerializeField] private InputBindings bindings;
    [SerializeField] private TMP_Text label;
    [SerializeField] private string lineFormat = "<b>{0}</b>  {1}";

    private void Awake()
    {
        if (label == null) label = GetComponentInChildren<TMP_Text>();
    }

    private void OnEnable()
    {
        Refresh();
    }

    private void OnValidate()
    {
        if (isActiveAndEnabled) Refresh();
    }

    private void Refresh()
    {
        if (label == null || bindings == null) return;

        IReadOnlyList<InputBindings.Binding> entries = bindings.Describe();
        StringBuilder builder = new StringBuilder();

        for (int i = 0; i < entries.Count; i++)
        {
            if (i > 0) builder.AppendLine();
            builder.AppendFormat(lineFormat, entries[i].Keys, entries[i].Action);
        }

        label.text = builder.ToString();
    }
}
