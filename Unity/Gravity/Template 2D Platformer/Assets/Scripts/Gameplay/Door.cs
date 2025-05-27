using System.Collections.Generic;
using UnityEngine;
using UnityEngine.SceneManagement;

public class Door : MonoBehaviour
{
    [SerializeField] private List<KeyID> requiredKeys;

    private void OnEnable()
    {
        KeyInventory.OnKeyCollected += HandleKeyUpdate;
    }

    private void OnDestroy()
    {
        KeyInventory.OnKeyCollected -= HandleKeyUpdate;
    }

    private void HandleKeyUpdate(KeyInventory inventory)
    {
        if (inventory.HasAllKeys(requiredKeys))
            gameObject.SetActive(false);
        else
            gameObject.SetActive(true);
    }
}