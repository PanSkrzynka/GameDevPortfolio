using System.Collections.Generic;
using NUnit.Framework;
using UnityEngine;

public class SaveManagerTests
{
    private GameObject _host;
    private KeyInventory _inventory;
    private CoinWallet _wallet;

    [SetUp]
    public void SetUp()
    {
        _host = new GameObject("SaveHost");
        _inventory = _host.AddComponent<KeyInventory>();
        _wallet = _host.AddComponent<CoinWallet>();
    }

    [TearDown]
    public void TearDown()
    {
        Object.DestroyImmediate(_host);
        Timer.ResetElapsed();
    }

    [Test]
    public void CreateData_CapturesKeysCoinsAndTime()
    {
        _inventory.AddKey(KeyID.Red);
        _inventory.AddKey(KeyID.Gold);
        _wallet.Add(12);

        SaveData data = SaveManager.CreateData(_inventory, _wallet, 42.5f);

        Assert.AreEqual(12, data.Coins);
        Assert.AreEqual(42.5f, data.ElapsedTime, 0.001f);
        CollectionAssert.AreEquivalent(new[] { KeyID.Red, KeyID.Gold }, data.CollectedKeys);
    }

    [Test]
    public void CreateData_WithNullSources_ReturnsEmptyData()
    {
        SaveData data = SaveManager.CreateData(null, null, -5f);

        Assert.AreEqual(0, data.Coins);
        Assert.AreEqual(0f, data.ElapsedTime, 0.001f);
        Assert.IsEmpty(data.CollectedKeys);
    }

    [Test]
    public void ApplyData_RestoresKeysAndCoins()
    {
        SaveData data = new SaveData
        {
            ElapsedTime = 7f,
            CollectedKeys = new List<KeyID> { KeyID.Blue },
            Coins = 4
        };

        SaveManager.ApplyData(data, _inventory, _wallet);

        Assert.AreEqual(4, _wallet.Coins);
        Assert.IsTrue(_inventory.HasAllKeys(new List<KeyID> { KeyID.Blue }));
        Assert.AreEqual(7f, Timer.ElapsedTime, 0.001f);
    }

    [Test]
    public void ApplyData_ReplacesPreviouslyHeldKeys()
    {
        _inventory.AddKey(KeyID.Skull);

        SaveManager.ApplyData(new SaveData { CollectedKeys = new List<KeyID> { KeyID.Blue } }, _inventory, _wallet);

        Assert.IsFalse(_inventory.HasAllKeys(new List<KeyID> { KeyID.Skull }));
    }

    [Test]
    public void JsonRoundTrip_PreservesData()
    {
        _inventory.AddKey(KeyID.Skull);
        _wallet.Add(9);

        SaveData original = SaveManager.CreateData(_inventory, _wallet, 3.25f);
        SaveData restored = SaveManager.Deserialize(SaveManager.Serialize(original));

        Assert.IsNotNull(restored);
        Assert.AreEqual(9, restored.Coins);
        Assert.AreEqual(3.25f, restored.ElapsedTime, 0.001f);
        CollectionAssert.AreEquivalent(new[] { KeyID.Skull }, restored.CollectedKeys);
    }

    [Test]
    public void Deserialize_EmptyInput_ReturnsNull()
    {
        Assert.IsNull(SaveManager.Deserialize(null));
        Assert.IsNull(SaveManager.Deserialize(string.Empty));
        Assert.IsNull(SaveManager.Deserialize("   "));
    }

    [Test]
    public void ApplyData_WithNullData_DoesNotThrow()
    {
        Assert.DoesNotThrow(() => SaveManager.ApplyData(null, _inventory, _wallet));
    }
}
