using NUnit.Framework;
using UnityEngine;

public class HealthSystemTests
{
    private GameObject _host;
    private HealthSystem _health;

    [SetUp]
    public void SetUp()
    {
        _host = new GameObject("HealthHost");
        _health = _host.AddComponent<HealthSystem>();
        _health.ResetHealth();
    }

    [TearDown]
    public void TearDown()
    {
        Object.DestroyImmediate(_host);
    }

    [Test]
    public void ResetHealth_RestoresToMax()
    {
        _health.TakeDamage(2);
        _health.ResetHealth();

        Assert.AreEqual(_health.MaxHealth, _health.CurrentHealth);
    }

    [Test]
    public void TakeDamage_ReducesCurrentHealth()
    {
        _health.TakeDamage(1);

        Assert.AreEqual(_health.MaxHealth - 1, _health.CurrentHealth);
    }

    [Test]
    public void TakeDamage_CannotDropBelowZero()
    {
        _health.TakeDamage(_health.MaxHealth + 5);

        Assert.AreEqual(0, _health.CurrentHealth);
    }

    [Test]
    public void TakeDamage_NonPositiveAmountIsIgnored()
    {
        _health.TakeDamage(0);
        _health.TakeDamage(-3);

        Assert.AreEqual(_health.MaxHealth, _health.CurrentHealth);
    }

    [Test]
    public void Heal_CannotExceedMax()
    {
        _health.Heal(99);

        Assert.AreEqual(_health.MaxHealth, _health.CurrentHealth);
    }

    [Test]
    public void DeathStateChanged_RaisedOnceOnDeath()
    {
        int deaths = 0;
        _health.DeathStateChanged += dead => { if (dead) deaths++; };

        _health.TakeDamage(_health.MaxHealth);
        _health.TakeDamage(1);

        Assert.AreEqual(1, deaths);
    }

    [Test]
    public void DeathStateChanged_RaisedOnReviveAfterHeal()
    {
        bool? lastState = null;
        _health.DeathStateChanged += dead => lastState = dead;

        _health.TakeDamage(_health.MaxHealth);
        _health.Heal(1);

        Assert.AreEqual(false, lastState);
    }
}
