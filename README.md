# Trvzxs

A toy Vulkan renderer written in C17, with a thin C++20 layer on top. I built it to learn how Vulkan works from the ground up: devices, swapchains, pipelines, memory, and synchronization across queues.

> **This is a learning project, not a library, but it builds and runs in its current state.** It has plenty of loose ends: hard-coded paths, leftover debug output, half-finished features and code that was never cleaned up. Most of the effort went into the per-frame hot path and the data layout behind it, not into polish.

## Where the effort went

### The per-frame hot path

- **Hot/cold data split.** Transforms live in three parallel arrays: *hot* (12 bytes: parent index, lifecycle, pass flags, subtree size), *world* (the 64-byte world matrix, exactly one cache line) and *cold* (position, rotation, scale). The per-frame walk only touches the hot array, so it stays in L2 cache with 20K+ entities.
- **Pre-order hierarchy with subtree pruning.** The scene graph is flattened into one contiguous array, parents before children. Every node stores the size of its subtree, so skipping a node skips all of its descendants with a single index jump.
- **Per-node flags decide what gets skipped.** `DISABLED` cascades down to children, `DIRTY` propagates down so only transforms that actually changed get recomputed, and `FRUSTUM_CULLED` lets the transform pass skip nodes that are out of view.
- **Frustum culling before transform resolve.** It uses Gribb/Hartmann plane extraction and either an Arvo AABB test or a cheaper bounding-sphere test. When a parent is out of view, its whole subtree is culled in bulk. Dirty nodes are never culled against stale matrices.
- **LOD.** Each model gets three discrete levels of detail, generated with meshoptimizer and selected by projected size on screen.

### The Vulkan side

- **A small render graph.** Passes declare their inputs and outputs, and the graph orders them and inserts the barriers, including queue-family ownership transfers between the graphics and async compute queues.
- **Synchronization.** Frames are synchronized with timeline semaphores, and debug builds record GPU timestamps per pass.
- **Passes:** depth prepass, model pass, a light-volume pass that reads a geometry buffer, and compute-driven particles on the compute queue.
- **Memory** is handled by VMA. Descriptors are moving to bindless through descriptor indexing (in progress).

The demo scene has 100 light groups, each parenting 200 model instances, for 20,100 entities in the hierarchy. A benchmark mode flies a fixed camera path and marks random transforms dirty every frame.
