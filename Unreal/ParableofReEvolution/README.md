# Parable of ReEvolution  
A Tactical RPG Prototype Built with Unreal Engine 5

**Parable of ReEvolution** is a turn-based tactical RPG prototype inspired by the Fire Emblem series, built in Unreal Engine 5. It focuses on grid-based movement, turn structure, and strategic combat using the Gameplay Ability System (GAS) and AI-driven enemies.

---

##  Gameplay Showcase

 ![Player Turn](gifs/parable1.gif)  ![Combat](gifs/parable2.gif)  ![Enemy AI](gifs/parable3.gif) 

---

##  Core Systems

### Gameplay Ability System (GAS)
- Modular movement, attack, and unit actions
- Abilities control range, execution, and rules
- Easily extendable for future character classes

### Grid-Based Tactical Engine
- Tile-based movement and targeting
- Visual highlights for range and paths
- Turn state machine per unit
- Support terrain modifiers

### Enemy AI
- Pathfinding and range-aware decision-making
- Aggressive behavior using EQS and behavior trees
- Uses the same GAS abilities as player units

### Turn-Based Flow
- Player and enemy phases with unit queues
- Full turn cycle handling
- Unit states: selectable, acted, waiting

---

##  Project Purpose

**Parable of ReEvolution** is designed to demonstrate:
- Mastery of Unreal’s Gameplay Ability System in a turn-based context
- Tactical RPG core mechanics: movement, combat, turn control
- Modular AI and UI systems
- Clean Unreal architecture suited for scale and polish

---

##  Built With

- Unreal Engine 5.4+  
- Gameplay Ability System (GAS)  
- Blueprints & Behavior Trees  
- EQS (Environmental Query System) for targeting  
- Custom grid + unit managers  

---

##  Current Status

This is a gameplay prototype — all visuals are placeholder.  
Focus is on functionality, responsiveness, and expandability.

---

## 📌 Roadmap Concepts

- Class system with passive and active skills  
- Weapon triangle & terrain modifiers  
- Unit promotion, recruitment, and permadeath  
- Dialogue, objectives, and chapter progression  
- Campaign maps with save/load system  

