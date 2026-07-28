using UnityEngine;
using TMPro;

[RequireComponent(typeof(Collider2D))]
public class LevelGoal : MonoBehaviour
{
    [SerializeField] private GameObject completionUI;
    [SerializeField] private TMP_Text completionText;
    [SerializeField] private string message = "Vault Opened";
    [SerializeField] private bool freezePlayer = true;
    [SerializeField] private AudioClip completeSound;

    private bool _completed;

    private void Start()
    {
        if (completionUI != null)
        {
            completionUI.SetActive(false);
        }
    }

    private void OnTriggerEnter2D(Collider2D other)
    {
        if (_completed || !other.CompareTag(Tags.Player)) return;

        _completed = true;
        SfxPlayer.Play(completeSound);

        if (completionText != null)
        {
            completionText.text = message + "\n" + Timer.FormattedElapsedTime;
        }

        if (completionUI != null)
        {
            completionUI.SetActive(true);
        }

        if (!freezePlayer) return;

        if (other.TryGetComponent(out PlayerController controller))
        {
            controller.enabled = false;
        }

        if (other.TryGetComponent(out Rigidbody2D body))
        {
            body.linearVelocity = Vector2.zero;
        }
    }
}
