using UnityEngine;

[RequireComponent(typeof(Collider2D))]
public class DamagePlayerOnContact : MonoBehaviour
{
    [SerializeField] private int damage = 1;
    [SerializeField] private bool disableAfterDamage;

    private void OnCollisionEnter2D(Collision2D collision)
    {
        TryDamage(collision.collider);
    }

    private void OnTriggerEnter2D(Collider2D other)
    {
        TryDamage(other);
    }

    private void TryDamage(Collider2D other)
    {
        if (!other.CompareTag(Tags.Player) || !other.TryGetComponent(out HealthSystem health))
        {
            return;
        }

        health.TakeDamage(damage);
        if (disableAfterDamage)
        {
            gameObject.SetActive(false);
        }
    }
}
