using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class HealthSystem : MonoBehaviour, IRewindable
{
    [SerializeField] private int maxHealth = 3;
    [SerializeField] private Image[] heartIcons;
    [SerializeField] private Sprite fullHeart;
    [SerializeField] private Sprite emptyHeart;

    private int _currentHealth;

    [SerializeField] private GameObject GameOverUI;

    private void Start()
    {
        _currentHealth = maxHealth;
        UpdateUI();
    }

    public void TakeDamage(int amount)
    {
        _currentHealth = Mathf.Max(0, _currentHealth - amount);
        if (_currentHealth <= 0)
        {
            if (GameOverUI != null)
                GameOverUI.SetActive(true);
        }
        UpdateUI();
    }

    public void Heal(int amount)
    {
        _currentHealth = Mathf.Min(maxHealth, _currentHealth + amount);
        UpdateUI();
    }

    private void UpdateUI()
    {
        for (int i = 0; i < heartIcons.Length; i++)
        {
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
        UpdateUI();
    }

    private void OnEnable() => TimeRewind.Register(this);
    private void OnDisable() => TimeRewind.Unregister(this);
}