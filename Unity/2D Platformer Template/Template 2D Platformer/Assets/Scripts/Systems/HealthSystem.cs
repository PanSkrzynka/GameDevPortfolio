using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.Serialization;

public class HealthSystem : MonoBehaviour, IRewindable
{
    [SerializeField] private int maxHealth = 3;
    [SerializeField] private Image[] heartIcons;
    [SerializeField] private Sprite fullHeart;
    [SerializeField] private Sprite emptyHeart;
    [FormerlySerializedAs("GameOverUI")]
    [SerializeField] private GameObject gameOverUI;
    [SerializeField] private AudioClip hurtSound;
    [SerializeField] private AudioClip gameOverSound;

    private int _currentHealth;
    private bool _deathStateBroadcast;

    public event System.Action<bool> DeathStateChanged;

    public int CurrentHealth => _currentHealth;
    public int MaxHealth => maxHealth;

    private void Start()
    {
        WarnIfUnwired();
        ResetHealth();
    }

    public void ResetHealth()
    {
        _currentHealth = maxHealth;
        SetGameOverVisible(false);
        UpdateUI();
    }

    private void WarnIfUnwired()
    {
        int wired = 0;
        if (heartIcons != null)
        {
            for (int i = 0; i < heartIcons.Length; i++)
            {
                if (heartIcons[i] != null) wired++;
            }
        }

        if (wired == 0)
        {
            Debug.LogWarning($"{nameof(HealthSystem)} on {name} has no heart icons assigned.", this);
        }

        if (gameOverUI == null)
        {
            Debug.LogWarning($"{nameof(HealthSystem)} on {name} has no game over UI assigned.", this);
        }
    }

    public void TakeDamage(int amount)
    {
        if (amount <= 0) return;

        _currentHealth = Mathf.Max(0, _currentHealth - amount);
        SfxPlayer.Play(hurtSound);
        if (_currentHealth <= 0)
        {
            SfxPlayer.Play(gameOverSound);
            SetGameOverVisible(true);
        }
        UpdateUI();
    }

    public void Heal(int amount)
    {
        if (amount <= 0) return;

        _currentHealth = Mathf.Min(maxHealth, _currentHealth + amount);
        if (_currentHealth > 0)
        {
            SetGameOverVisible(false);
        }
        UpdateUI();
    }

    private void UpdateUI()
    {
        if (heartIcons == null) return;

        for (int i = 0; i < heartIcons.Length; i++)
        {
            if (heartIcons[i] == null) continue;
            heartIcons[i].sprite = i < _currentHealth ? fullHeart : emptyHeart;
        }
    }

    public void SaveState(List<object> buffer)
    {
        buffer.Add(_currentHealth);
    }

    public void LoadState(object state)
    {
        _currentHealth = (int)state;
        SetGameOverVisible(_currentHealth <= 0);
        UpdateUI();
    }

    private void SetGameOverVisible(bool visible)
    {
        if (gameOverUI != null)
        {
            gameOverUI.SetActive(visible);
        }

        if (visible == _deathStateBroadcast) return;

        _deathStateBroadcast = visible;
        DeathStateChanged?.Invoke(visible);
    }

    private void OnEnable() => TimeRewind.Register(this);
    private void OnDisable() => TimeRewind.Unregister(this);
}
