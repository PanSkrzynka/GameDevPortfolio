using System.Collections.Generic;
using UnityEngine;

[RequireComponent(typeof(Collider2D))]
public class Key : MonoBehaviour, IRewindable
{
    [SerializeField] private KeyID keyID;
    [SerializeField] private AudioClip collectSound;

    private bool _collected;

    private void OnTriggerEnter2D(Collider2D other)
    {
        if (_collected || !other.CompareTag(Tags.Player)) return;
        if (!other.TryGetComponent(out KeyInventory inventory)) return;

        inventory.AddKey(keyID);
        _collected = true;
        SfxPlayer.Play(collectSound);
        gameObject.SetActive(false);
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
