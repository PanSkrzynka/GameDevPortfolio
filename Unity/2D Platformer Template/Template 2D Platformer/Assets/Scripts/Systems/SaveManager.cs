using System.Collections.Generic;
using System.IO;
using System.Runtime.Serialization.Formatters.Binary;
using UnityEngine;

[System.Serializable]
public class SaveData
{
    public float ElapsedTime;
    public List<KeyID> CollectedKeys;
}

public static class SaveManager
{
    private static readonly string SavePath = Path.Combine(Application.persistentDataPath, "save.dat");

    public static void Save(KeyInventory inventory)
    {
        var data = new SaveData
        {
            ElapsedTime = Timer.ElapsedTime,
            CollectedKeys = new List<KeyID>(inventory.GetCollectedKeys())
        };

        using var stream = File.Create(SavePath);
        new BinaryFormatter().Serialize(stream, data);
        Debug.Log($"Game saved to {SavePath}");
    }

    public static void Load(KeyInventory inventory)
    {
        if (!File.Exists(SavePath)) return;

        using var stream = File.OpenRead(SavePath);
        var data = (SaveData)new BinaryFormatter().Deserialize(stream);
        typeof(Timer).GetProperty("ElapsedTime")?.SetValue(null, data.ElapsedTime);
        inventory.LoadCollectedKeys(data.CollectedKeys);
        Debug.Log("Game loaded.");
    }
}