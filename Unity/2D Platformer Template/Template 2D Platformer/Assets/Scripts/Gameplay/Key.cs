using UnityEngine;
using System.Collections.Generic;
public class Key : MonoBehaviour, IRewindable
{
    [SerializeField] private KeyID keyID;
    private bool _collected;

    private void OnTriggerEnter2D(Collider2D other)
    {
        if (_collected || !other.CompareTag("Player")) return;
        if (other.TryGetComponent(out KeyInventory inventory))
        {
            inventory.AddKey(keyID);
            _collected = true;
            gameObject.SetActive(false);
        }
    }

    public void SaveState(List<object> buffer) => buffer.Add(_collected);
    public void LoadState(object state)
    {
        _collected = (bool)state;
        gameObject.SetActive(!_collected);
    }
    
    private void OnEnable() => TimeRewind.Register(this);
    private void OnDisable() => TimeRewind.Unregister(this);
}