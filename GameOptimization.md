# How I Approach Game Optimization

I start by defining the scope: am I improving the game's overall performance, or investigating a specific bottleneck? Both approaches need repeatable measurements, but they differ in how broadly I investigate the systems involved.

## Establish a Repeatable Benchmark

For general optimization, I write benchmark scripts that reproduce representative gameplay scenarios. These give me a consistent environment for measuring performance and checking whether changes affect game behavior.

I then establish whether the game is CPU-bound or GPU-bound on the target hardware. I also vary the settings to understand roughly where the limiting factor changes.

When investigating a specific bottleneck, I build a repeatable scenario around the affected behavior. This lets me compare changes against the same workload rather than relying on different manual playthroughs.

## Tools I Use

### Unity

I use the **Unity Profiler** to investigate frame-time costs, script execution, and allocations. The **Frame Debugger** helps me inspect rendering events and batching, while the **Memory Profiler** helps me examine memory usage and compare snapshots.

The **Unity Test Framework** supports behavior checks alongside my benchmark scripts, helping me check that an optimization preserves the expected results.

### Unreal Engine

I use **stat commands** for an initial view of performance and **Unreal Insights** to investigate timing, thread activity, and expensive work in more detail. **GPU Visualizer** helps me examine rendering costs, while **Shader Complexity** provides a visual aid for investigating expensive materials.

For memory investigations, I use **Memory Insights**. When the issue involves multiplayer traffic, I use **Network Profiler** to inspect bandwidth usage and replication costs.

### Graphics Analysis Tools

My toolkit also includes **RenderDoc**, **NVIDIA Nsight Graphics**, and **Intel GPA** for frame inspection and graphics analysis. I choose the tool according to the target hardware, graphics API, and the problem I am investigating.

## Focus on Programming Improvements

My main focus is reducing CPU overhead without changing the game's visual style. Where practical, I aim to make GPU rendering, rather than avoidable CPU work, the limiting factor. This gives me room to improve performance through programming without requiring graphical or artistic changes.

On the rendering side, I look for batching issues, redundant refreshes, and unnecessary rendering work that can be addressed in code. I leave model optimization to someone with a 3D graphics background, coordinating with them when profiling points to an asset-related problem.

## Prioritize the Largest Costs

For both general optimization and specific bottlenecks, I start with the work taking the largest share of frame time. I trace those costs into the relevant code and prioritize changes based on the measurements.

Typical changes include batching calls together, improving how data is accessed and reused inside loops, and reducing how often expensive functions run. I use the benchmark scenarios to check whether each change improves performance while preserving the intended behavior.

## Balance Performance and Maintainability

My goal is to reduce CPU costs as far as practical while keeping the code understandable, reusable, and easy to modify. Extreme optimizations can make a system rigid or harder to maintain, so I weigh their benefit against that added complexity.

Benchmarks on the target devices guide how far I take the work. If performance still falls short of the project's requirements, I investigate further optimizations and their tradeoffs. If the requirements are met, I avoid adding complexity for gains that do not justify the maintenance cost.

## Case Study: Grass Cutting in Overgrown Cleaner

Testing Overgrown Cleaner on Steam Deck revealed a significant performance drop while cutting grass: from 60 FPS to an average below 30 FPS. The initial bottleneck was on the CPU.

### Batching Foliage Updates

I traced the CPU cost to the grass-cutting tool. Its implementation called the foliage-editing function separately for each grass instance cut during a frame. I replaced those individual calls with a batch, submitting the foliage changes in one call per frame.

Measurements on my PC showed the following reductions in cutting CPU time:

| Cutting scenario | Before | After | CPU time reduction |
| --- | ---: | ---: | ---: |
| Low/full cuts | 7.91 ms | 1.93 ms | 76% |
| Higher/partial cuts | 0.91 ms | 0.69 ms | 24% |

These measurements describe the CPU cost of cutting, rather than total frame time or Steam Deck frame rates.

### Following the Bottleneck to the GPU

After the batching change, I tested again on Steam Deck. Frame rates were still low, but the workload was now GPU-bound. Further investigation showed that foliage interactions were triggering distance-field lighting updates for the grass every frame.

I disabled the grass's distance-field contribution on lower graphics settings, including the Steam Deck configuration. On higher settings, I reduced the frequency of grass interactions to limit those updates while retaining the distance-field contribution.

The GPU measurements below compare normal cutting with distance-field contribution disabled. A separate test also removed particles to isolate their remaining cost.

| Test | Average GPU time |
| --- | ---: |
| Idle | 4.05 ms |
| Normal cutting | 7.35 ms |
| Cutting with grass distance-field contribution disabled, particles retained | 4.55 ms |
| Same, also without particles | 4.03 ms |

With particles retained, disabling the grass's distance-field contribution reduced average GPU time during cutting from 7.35 ms to 4.55 ms: a reduction of approximately 38%, or about 40%. Removing particles in the diagnostic test brought GPU time close to the idle measurement.

### Outcome and Tradeoffs

The combined changes resolved the grass-cutting performance problem in my follow-up Steam Deck testing. Batching addressed the CPU bottleneck; the graphics-setting changes addressed the GPU cost that became the next limiting factor. The lower preset trades the grass's distance-field lighting contribution for performance, while higher settings retain that contribution with less frequent grass interactions.

This investigation illustrates why I re-profile on the target hardware after each major optimization: reducing one cost can expose a different bottleneck, so the first improvement may not be enough to resolve the player-facing problem.

[Back to the portfolio](README.md)
