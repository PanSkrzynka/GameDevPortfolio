using System.Collections.Generic;
using UnityEngine;

public class Door : MonoBehaviour
{
    [SerializeField] private List<KeyID> requiredKeys;
    [SerializeField] private AudioClip unlockSound;
    private KeyInventory _inventory;
    private bool _stateInitialised;

    private void OnEnable()
    {
        KeyInventory.OnKeyCollected += HandleKeyUpdate;
    }

    private void Start()
    {
        _inventory = FindFirstObjectByType<KeyInventory>();
        RefreshState();
    }

    private void OnDisable()
    {
        KeyInventory.OnKeyCollected -= HandleKeyUpdate;
    }

    private void HandleKeyUpdate(KeyInventory inventory)
    {
        if (_inventory == null)
        {
            _inventory = inventory;
        }
        if (inventory != _inventory)
        {
            return;
        }

        RefreshState();
    }

    private void RefreshState()
    {
        bool isLocked = _inventory == null || !_inventory.HasAllKeys(requiredKeys);

        if (_stateInitialised && !isLocked)
        {
            SfxPlayer.Play(unlockSound);
        }

        _stateInitialised = true;
        gameObject.SetActive(isLocked);
    }
}
