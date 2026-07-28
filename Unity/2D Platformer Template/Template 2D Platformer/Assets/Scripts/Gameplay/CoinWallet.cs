using System;
using UnityEngine;

public class CoinWallet : MonoBehaviour
{
    private int _coins;

    public int Coins => _coins;
    public event Action<int> CoinsChanged;

    public void Add(int amount)
    {
        if (amount == 0) return;

        _coins = Mathf.Max(0, _coins + amount);
        CoinsChanged?.Invoke(_coins);
    }

    public void SetCoins(int amount)
    {
        _coins = Mathf.Max(0, amount);
        CoinsChanged?.Invoke(_coins);
    }
}
