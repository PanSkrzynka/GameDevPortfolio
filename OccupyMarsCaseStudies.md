# Occupy Mars World Generation and Floating Origin

As a gameplay programmer on Occupy Mars, I worked on reducing map-generation stutters and fixing visual precision problems far from the world origin. Both tasks required changes across multiple systems while preserving the player's existing world and its appearance.

## Reducing Map Generation Stutters

### Problem and Responsibility

Driving into unexplored areas caused significant frame-rate drops as new map chunks generated. I was tasked with investigating and resolving the problem.

My initial investigation showed that the MapMagic generation graph was already reasonably optimized, with limited room for significant gains there. Reducing chunk size was a straightforward option: smaller chunks would generate more frequently, but each would require less work.

However, changing chunk size would also change the generated map layout and break compatibility with existing player saves. I ruled out that approach and looked for improvements that preserved the existing world generation layout.

### Spreading Work Across Frames and Reusing Objects

One source of spikes was the destroyed-base generation script, which created an entire base at once. I split that work across multiple frames and capped its cost per frame, distributing the work instead of concentrating it in a single update.

I also introduced object pooling to improve memory usage and reuse generated objects. I extended this to other generated content, including destructible rocks.

### Investigating the Remaining Spikes

Noticeable spikes remained after these changes, so I inspected execution frame by frame. This revealed an interaction between MapMagic's chunk generation and the terrain shader we were using.

I modified the MapMagic chunk-generation code to reuse the shader setup for newly generated chunks. I also edited the terrain shader to remove functionality the game did not use.

The work covered generation scheduling, object reuse, and the integration between terrain generation and rendering. Preserving save compatibility guided the investigation: I kept the chunk layout intact and changed how the work was performed.

## Keeping the Player Near the World Origin

### Problem and Responsibility

When I joined the team, travelling far from the starting point caused visible rendering problems, particularly in small details on in-world UI. I took responsibility for investigating and fixing the issue and identified floating-point precision loss at large coordinates as the cause.

### Implementing World Shifting

I created a floating-origin system that shifted the world to keep the player close to the coordinate origin. This addressed the precision problem, but required other systems to account for the shift.

Mist and terrain shaders needed additional handling to preserve their appearance. I modified them to incorporate the world-shift offset so that shifting the world would not visibly change their output.

### Coordinates for Saving and Spawning

I also created a coordinate system for saving and spawning objects that accounted for world shifts. This allowed those systems to work with object locations consistently as the local world coordinates changed.

My responsibility continued beyond the initial implementation. I maintained the world-shifting system over time and built tools to help other team members use it.

[Back to the portfolio](README.md)
