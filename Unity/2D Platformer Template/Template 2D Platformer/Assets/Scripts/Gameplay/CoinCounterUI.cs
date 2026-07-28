using TMPro;
using UnityEngine;

public class CoinCounterUI : MonoBehaviour
{
    [SerializeField] private TMP_Text label;
    [SerializeField] private CoinWallet wallet;
    [SerializeField] private string format = "x {0}";

    private void Awake()
    {
        if (label == null) label = GetComponent<TMP_Text>();
    }

    private void OnEnable()
    {
        if (wallet == null) wallet = FindFirstObjectByType<CoinWallet>();

        if (wallet != null)
        {
            wallet.CoinsChanged += Refresh;
            Refresh(wallet.Coins);
        }
        else
        {
            Refresh(0);
        }
    }

    private void OnDisable()
    {
        if (wallet != null) wallet.CoinsChanged -= Refresh;
    }

    private void Refresh(int coins)
    {
        if (label != null) label.text = string.Format(format, coins);
    }
}
