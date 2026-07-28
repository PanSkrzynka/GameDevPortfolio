using UnityEngine;

public class SaveController : MonoBehaviour
{
    [SerializeField] private InputBindings bindings;
    [SerializeField] private KeyInventory inventory;
    [SerializeField] private CoinWallet wallet;
    [SerializeField] private AudioClip confirmSound;

    private void Awake()
    {
        if (inventory == null) inventory = FindFirstObjectByType<KeyInventory>();
        if (wallet == null) wallet = FindFirstObjectByType<CoinWallet>();
    }

    private void Update()
    {
        if (bindings == null) return;

        if (Input.GetKeyDown(bindings.Save) && SaveManager.Save(inventory, wallet))
        {
            SfxPlayer.Play(confirmSound);
        }

        if (Input.GetKeyDown(bindings.Load) && SaveManager.Load(inventory, wallet))
        {
            SfxPlayer.Play(confirmSound);
        }
    }
}
