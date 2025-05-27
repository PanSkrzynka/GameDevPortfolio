# 🎮 2D Platformer Template

A modular Unity 2D platformer showcasing core gameplay systems and clean architecture. Ideal for learning, prototyping, or extending into a complete game.

---

## ✨ Features

### 🕹️ Player Controller
- Responsive horizontal movement
- Jump buffering (coyote time)
- Smooth camera follow

### ⏪ Rewind System
- Rewind of player and world state
- Works on: player, enemies, keys, doors, health
- Event-driven system with `IRewindable` interface

### 🗝️ Keys & Doors
- Collect unique keys (`KeyID`-based)
- Doors auto-unlock when player has required keys
- Event-driven unlocking

### ❤️ Health & UI
- Heart-based health display
- Game Over screen when health reaches 0
- Damage on enemy contact

### 💾 Save System
- Binary save/load (keys + timer)
- No PlayerPrefs
- Easily extendable

---

## 🧠 What It Demonstrates

This project showcases **base Unity knowledge**, including:
- Physics-based movement
- 2D raycasting and collision
- Event-driven game logic
- Rewindable state management
- Modular architecture and serialization


## License

This project is open-source under the MIT License.

