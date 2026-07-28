using UnityEngine;
using UnityEngine.SceneManagement;

public class PauseController : MonoBehaviour
{
    [SerializeField] private InputBindings bindings;
    [SerializeField] private HealthSystem health;
    [SerializeField] private GameObject pauseUI;

    private bool _paused;
    private bool _gameOver;

    public bool IsPaused => _paused || _gameOver;

    private void OnEnable()
    {
        if (health == null) health = FindFirstObjectByType<HealthSystem>();
        if (health != null) health.DeathStateChanged += HandleDeathStateChanged;
    }

    private void OnDisable()
    {
        if (health != null) health.DeathStateChanged -= HandleDeathStateChanged;
        Time.timeScale = 1f;
    }

    private void Start()
    {
        ApplyPaused(false);
    }

    private void Update()
    {
        if (bindings == null) return;

        if (IsPaused && Input.GetKeyDown(bindings.Restart))
        {
            Restart();
            return;
        }

        if (_gameOver) return;

        if (Input.GetKeyDown(bindings.Pause)) ApplyPaused(!_paused);
    }

    private void HandleDeathStateChanged(bool dead)
    {
        _gameOver = dead;
        if (dead && pauseUI != null) pauseUI.SetActive(false);
        ApplyTimeScale();
    }

    private void ApplyPaused(bool paused)
    {
        _paused = paused;
        if (pauseUI != null) pauseUI.SetActive(paused);
        ApplyTimeScale();
    }

    private void ApplyTimeScale()
    {
        Time.timeScale = IsPaused ? 0f : 1f;
    }

    public void Restart()
    {
        Time.timeScale = 1f;
        Timer.ResetElapsed();
        SceneManager.LoadScene(SceneManager.GetActiveScene().name);
    }
}
