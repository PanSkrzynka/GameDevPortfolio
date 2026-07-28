using System;
using System.Collections.Generic;
using System.IO;
using UnityEngine;

[Serializable]
public class SaveData
{
    public float ElapsedTime;
    public List<KeyID> CollectedKeys = new List<KeyID>();
    public int Coins;
}

public static class SaveManager
{
    private const string SaveFileName = "save.json";

    private static string SavePath => Path.Combine(Application.persistentDataPath, SaveFileName);

    public static SaveData CreateData(KeyInventory inventory, CoinWallet wallet, float elapsedTime)
    {
        return new SaveData
        {
            ElapsedTime = Mathf.Max(0f, elapsedTime),
            CollectedKeys = inventory == null ? new List<KeyID>() : new List<KeyID>(inventory.GetCollectedKeys()),
            Coins = wallet == null ? 0 : wallet.Coins
        };
    }

    public static void ApplyData(SaveData data, KeyInventory inventory, CoinWallet wallet)
    {
        if (data == null) return;

        Timer.SetElapsedTime(data.ElapsedTime);

        if (inventory != null)
        {
            inventory.LoadCollectedKeys(data.CollectedKeys ?? new List<KeyID>());
        }

        if (wallet != null)
        {
            wallet.SetCoins(data.Coins);
        }
    }

    public static string Serialize(SaveData data) => JsonUtility.ToJson(data, true);

    public static SaveData Deserialize(string json)
    {
        if (string.IsNullOrWhiteSpace(json)) return null;

        try
        {
            return JsonUtility.FromJson<SaveData>(json);
        }
        catch (Exception exception)
        {
            Debug.LogWarning($"Save data could not be parsed: {exception.Message}");
            return null;
        }
    }

    public static bool Save(KeyInventory inventory, CoinWallet wallet)
    {
        if (inventory == null)
        {
            Debug.LogError("Save failed: KeyInventory reference is null.");
            return false;
        }

        try
        {
            File.WriteAllText(SavePath, Serialize(CreateData(inventory, wallet, Timer.ElapsedTime)));
            Debug.Log($"Game saved to {SavePath}");
            return true;
        }
        catch (Exception exception)
        {
            Debug.LogError($"Save failed: {exception.Message}");
            return false;
        }
    }

    public static bool Load(KeyInventory inventory, CoinWallet wallet)
    {
        if (inventory == null)
        {
            Debug.LogError("Load failed: KeyInventory reference is null.");
            return false;
        }

        if (!File.Exists(SavePath))
        {
            Debug.LogWarning($"Load skipped: save file not found at {SavePath}");
            return false;
        }

        try
        {
            SaveData data = Deserialize(File.ReadAllText(SavePath));
            if (data == null)
            {
                Debug.LogWarning("Load skipped: save data is invalid.");
                return false;
            }

            ApplyData(data, inventory, wallet);
            Debug.Log("Game loaded.");
            return true;
        }
        catch (Exception exception)
        {
            Debug.LogError($"Load failed: {exception.Message}");
            return false;
        }
    }
}
