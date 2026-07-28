using NUnit.Framework;
using UnityEngine;

public class CoinWalletTests
{
    private GameObject _host;
    private CoinWallet _wallet;

    [SetUp]
    public void SetUp()
    {
        _host = new GameObject("CoinWalletHost");
        _wallet = _host.AddComponent<CoinWallet>();
    }

    [TearDown]
    public void TearDown()
    {
        Object.DestroyImmediate(_host);
    }

    [Test]
    public void Coins_StartAtZero()
    {
        Assert.AreEqual(0, _wallet.Coins);
    }

    [Test]
    public void Add_AccumulatesValue()
    {
        _wallet.Add(3);
        _wallet.Add(2);

        Assert.AreEqual(5, _wallet.Coins);
    }

    [Test]
    public void Add_CannotDropBelowZero()
    {
        _wallet.Add(2);
        _wallet.Add(-10);

        Assert.AreEqual(0, _wallet.Coins);
    }

    [Test]
    public void Add_Zero_DoesNotRaiseEvent()
    {
        int raised = 0;
        _wallet.CoinsChanged += _ => raised++;

        _wallet.Add(0);

        Assert.AreEqual(0, raised);
    }

    [Test]
    public void Add_RaisesEventWithNewTotal()
    {
        int reported = -1;
        _wallet.CoinsChanged += value => reported = value;

        _wallet.Add(7);

        Assert.AreEqual(7, reported);
    }

    [Test]
    public void SetCoins_ClampsNegativeToZero()
    {
        _wallet.SetCoins(-5);

        Assert.AreEqual(0, _wallet.Coins);
    }
}
