using System.Collections.Generic;
using NUnit.Framework;
using UnityEngine;

public class KeyInventoryTests
{
    private GameObject _host;
    private KeyInventory _inventory;

    [SetUp]
    public void SetUp()
    {
        _host = new GameObject("KeyInventoryHost");
        _inventory = _host.AddComponent<KeyInventory>();
    }

    [TearDown]
    public void TearDown()
    {
        Object.DestroyImmediate(_host);
    }

    [Test]
    public void HasAllKeys_WithNullRequirement_ReturnsTrue()
    {
        Assert.IsTrue(_inventory.HasAllKeys(null));
    }

    [Test]
    public void HasAllKeys_WithEmptyRequirement_ReturnsTrue()
    {
        Assert.IsTrue(_inventory.HasAllKeys(new List<KeyID>()));
    }

    [Test]
    public void HasAllKeys_WhenOneKeyMissing_ReturnsFalse()
    {
        _inventory.AddKey(KeyID.Red);

        Assert.IsFalse(_inventory.HasAllKeys(new List<KeyID> { KeyID.Red, KeyID.Gold }));
    }

    [Test]
    public void HasAllKeys_WhenAllKeysHeld_ReturnsTrue()
    {
        _inventory.AddKey(KeyID.Red);
        _inventory.AddKey(KeyID.Gold);

        Assert.IsTrue(_inventory.HasAllKeys(new List<KeyID> { KeyID.Red, KeyID.Gold }));
    }

    [Test]
    public void AddKey_IgnoresDuplicates()
    {
        _inventory.AddKey(KeyID.Blue);
        _inventory.AddKey(KeyID.Blue);

        Assert.AreEqual(1, new List<KeyID>(_inventory.GetCollectedKeys()).Count);
    }

    [Test]
    public void LoadCollectedKeys_ReplacesExistingContents()
    {
        _inventory.AddKey(KeyID.Red);
        _inventory.LoadCollectedKeys(new List<KeyID> { KeyID.Skull });

        Assert.IsFalse(_inventory.HasAllKeys(new List<KeyID> { KeyID.Red }));
        Assert.IsTrue(_inventory.HasAllKeys(new List<KeyID> { KeyID.Skull }));
    }
}
