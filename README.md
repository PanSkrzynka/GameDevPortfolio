# Game Development Portfolio

I'm a gameplay programmer working with Unity and Unreal Engine. My experience includes gameplay systems, developer tools, code optimization, refactoring, and coordinating programming teams.

This portfolio combines my contributions to commercial projects with personal samples in C# and C++.

## Professional Projects

### [Occupy Mars: The Game](https://store.steampowered.com/app/758690/)

**Roles:** Assistant Project Leader and Gameplay Programmer.

- Trained an AI robot companion using ML-Agents.
- Optimized large player-built bases.
- Developed procedural object and loot generation.
- Optimized map generation using MapMagic.
- Created and maintained a floating-origin system, including shader adjustments, coordinates for saving and spawning objects, and tools for the team.
- Configured and fixed vehicle physics.
- Adapted existing player interactions and tool-use mechanics from single-player to cooperative multiplayer.
- Worked on Steam Deck optimization.
- Created tools for level designers and developed a quest system.
- Contributed to the save/load system.
- Worked on resource gathering and player interaction systems.

[Read my world-generation and floating-origin case studies](OccupyMarsCaseStudies.md): reducing generation stutters while preserving existing saves, and fixing visual precision problems far from the world origin.

### [Overgrown Cleaner](https://store.steampowered.com/app/3164790/)

**Role:** Main Gameplay Programmer.

- Created a vertex-based cleaning system.
- Coordinated development work across a team of three programmers.
- Optimized grass cutting by batching foliage updates and reducing distance-field lighting update costs. [Read the case study](GameOptimization.md#case-study-grass-cutting-in-overgrown-cleaner).
- Optimized the game for Steam Deck.
- Worked on Steam and Xbox SDK integration.
- Developed and optimized gameplay tool handling.
- Worked with the Gameplay Ability System and Behavior Trees.
- Contributed to the save/load system.
- Worked on the in-game shop and transitions between maps.
- Developed core gameplay mechanics.

### Additional Optimization Work

- [Dinosaur Fossil Hunter](https://store.steampowered.com/app/864700/)
- [Castle Flipper](https://store.steampowered.com/app/944250/)

### OpenCoop GameINN

As the main programming coordinator, I worked on tools for adding cooperative multiplayer to existing Unity projects.

- Designed the workflow for adapting existing projects to co-op.
- Implemented automated code editing that identifies required changes and flags areas needing manual work.
- Created a machine learning model to assess the feasibility of converting games to co-op.

[Project information](https://www.gov.pl/web/ncbr-en/development-of-an-innovative-opencoop-system-for-creating-and-adding-an-online-cooperative-mode-to-single-player-simulator-games)

## Personal Projects and Samples

### Unreal Engine

#### [The Watch Remembers](Unreal/NPCproperty/README.md)

A foundation for an RPG NPC ownership and crime system, covering room permissions, theft detection, guard communication, and pursuit. Intended for further development, with polish and game-specific NPC responses still to be implemented.

**Built with:** Unreal Engine 5.8, C++, Blueprints, StateTree, AI Perception.

#### [Chaos Vehicle Debug](Unreal/CVehicleArea/README.md)

Debugging and telemetry tools for Chaos Vehicles, including wheel slip, load transfer, driver inputs, lap recording, CSV export, and ghost replays.

**Built with:** Unreal Engine 5.8, C++, Chaos Vehicles.

#### [Parable of ReEvolution](Unreal/ParableofReEvolution/README.md)

A personal tactical RPG project I develop in my free time, inspired by the Fire Emblem series, with grid movement, turn-based combat, and AI opponents. Included as a work-in-progress gameplay showcase; the source code remains private.

**Built with:** Unreal Engine 5, C++, Blueprints, Gameplay Ability System, Behavior Trees, EQS.

### Unity

#### [Gravity](Unity/Gravity/README.md)

A 3D gravity simulation using the Barnes-Hut algorithm, with generated planetary systems, orbital trails, and adjustable simulation speed.

**Built with:** Unity 6, C#.

#### [2D Platformer Template](Unity/2D%20Platformer%20Template/README.md)

A gameplay sample featuring movement, time rewind, keys and doors, health, collectibles, and save/load.

**Built with:** Unity 6, C#.

## How I Approach Game Optimization

[Read about my optimization workflow](GameOptimization.md): defining targets, profiling bottlenecks, prioritizing changes, and checking the results, including a grass-cutting case study from Overgrown Cleaner prompted by Steam Deck testing.

## Technical Experience

### Unity and C#

- **AI and systems:** ML-Agents, DOTS, Zenject.
- **Procedural generation:** MapMagic.
- **Editor tools:** Odin Inspector, editor scripting, ScriptableObjects.
- **Networking:** Mirror, Photon PUN 2, Netcode for GameObjects.
- **Input and persistence:** Rewired, Input System, Input Manager, Easy Save.
- **Rendering and audio:** Built-in Render Pipeline, URP, HDRP, 2D and 3D rendering, shader development, FMOD.
- **Testing and profiling:** Unity Test Framework, Profiler, Frame Debugger, Memory Profiler.

### Unreal Engine, C++, and Blueprints

- **Gameplay and AI:** Gameplay Ability System, Gameplay Tags, Blackboards, EQS, Mass, StateTree, AI Perception.
- **World and rendering:** World Partition, foliage, Lumen, Nanite.
- **Architecture and physics:** Game, Engine, and Editor Subsystems; Chaos Vehicles.
- **Audio:** FMOD.
- **Profiling:** Unreal Insights, stat commands, GPU Visualizer, Shader Complexity, Memory Insights, Network Profiler.

### Tools and Platforms

- **Graphics and performance tools:** RenderDoc, NVIDIA Nsight Graphics, Intel GPA.
- **Platforms:** PC, mobile, Steam Deck.
- **SDKs:** Steam, Epic, GOG.

## Contact

- **Email:** [michal.nowak.dev@gmail.com](mailto:michal.nowak.dev@gmail.com)
- **LinkedIn:** [Michał Nowak](https://www.linkedin.com/in/micha%C5%82-nowak-21045a117/)
