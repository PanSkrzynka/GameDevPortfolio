using UnityEngine;

[RequireComponent(typeof(Collider2D))]
public class DamagePlayerOnContact : MonoBehaviour
{
    [SerializeField] private int damage = 1;

    private void OnCollisionEnter2D(Collision2D collision)
    {
        if (collision.collider.CompareTag("Player") && collision.collider.TryGetComponent(out HealthSystem health))
        {
            health.TakeDamage(damage);
            gameObject.SetActive(false);
        }
    }
}