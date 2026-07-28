using UnityEngine;

[RequireComponent(typeof(Collider2D))]
public class Coin : MonoBehaviour
{
    [SerializeField] private int value = 1;
    [SerializeField] private AudioClip collectSound;

    private bool _collected;

    private void OnTriggerEnter2D(Collider2D other)
    {
        if (_collected || !other.CompareTag(Tags.Player)) return;
        if (!other.TryGetComponent(out CoinWallet wallet)) return;

        wallet.Add(value);
        _collected = true;
        SfxPlayer.Play(collectSound);
        gameObject.SetActive(false);
    }
}
