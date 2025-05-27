using System.Collections.Generic;
using UnityEngine;
using System;

public class KeyInventory : MonoBehaviour, IRewindable
{
    private readonly HashSet<KeyID> _collectedKeys = new();
    public static event Action<KeyInventory> OnKeyCollected;

    public void AddKey(KeyID keyID)
    {
        if (_collectedKeys.Add(keyID))
        {
            Debug.Log($"Collected key: {keyID}");
            OnKeyCollected?.Invoke(this);
        }
    }

    public bool HasAllKeys(List<KeyID> requiredKeys) => requiredKeys.TrueForAll(_collectedKeys.Contains);

    public IEnumerable<KeyID> GetCollectedKeys() => _collectedKeys;

    public void LoadCollectedKeys(IEnumerable<KeyID> keys)
    {
        _collectedKeys.Clear();
        foreach (var key in keys)
            _collectedKeys.Add(key);

        OnKeyCollected?.Invoke(this);
    }

    public void SaveState(List<object> buffer) => buffer.Add(new HashSet<KeyID>(_collectedKeys));
    public void LoadState(object state) => LoadCollectedKeys((HashSet<KeyID>)state);
    
    private void OnEnable() => TimeRewind.Register(this);
    private void OnDisable() => TimeRewind.Unregister(this);
}