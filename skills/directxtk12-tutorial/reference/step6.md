# Step 6: Simple Rendering

This step introduces rendering custom geometry using `BasicEffect`, `PrimitiveBatch`, and `VertexPositionColor`. We draw a triangle with per-vertex colors (red, green, blue) that blend smoothly across the surface. See the wiki page: [Simple rendering](https://github.com/microsoft/DirectXTK12/wiki/Simple-rendering).

## Background

To draw geometry with Direct3D 12, you need:

- A **vertex buffer** containing the vertices to draw
- A **root signature** which defines how the CPU and GPU shader programs share data
- A **pipeline state object** which defines all state, the vertex input layout, and the compiled shader programs
- A **primitive topology** setting that indicates how to interpret the individual vertices (as a point, a line, a triangle, etc.)

DirectX Tool Kit simplifies this:

| Requirement | DirectXTK12 Class |
| ------------- | ---------------- |
| Vertex buffer + topology | `PrimitiveBatch<T>` |
| Root signature + PSO + shaders | `BasicEffect` + `EffectPipelineStateDescription` |
| State descriptions | `CommonStates` |
| Vertex format | `VertexPositionColor` |
| Render target format | `RenderTargetState` |

## Code Changes

### Game.cpp — Render (comment out sprite drawing keeping the text)

First, comment out or remove the sprite drawing code from Step 4 in the `Render` method, since the triangle will cover the sprite:

```cpp
// m_spriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Cat),
//     GetTextureSize(m_texture.Get()),
//     m_screenPos, nullptr, Colors::White, 0.f, m_origin);
```

### pch.h

Ensure the following includes are in `pch.h`:

```cpp
#include <directxtk12/CommonStates.h>
#include <directxtk12/EffectPipelineStateDescription.h>
#include <directxtk12/Effects.h>
#include <directxtk12/PrimitiveBatch.h>
#include <directxtk12/RenderTargetState.h>
#include <directxtk12/SimpleMath.h>
#include <directxtk12/VertexTypes.h>
```

### Game.h

Add the following private member variables to the `Game` class:

```cpp
using VertexType = DirectX::VertexPositionColor;

std::unique_ptr<DirectX::BasicEffect> m_effect;
std::unique_ptr<DirectX::PrimitiveBatch<VertexType>> m_batch;
```

### Game.cpp — CreateDeviceDependentResources

In `CreateDeviceDependentResources`, create the PrimitiveBatch, define the pipeline state, and create the effect:

```cpp
auto device = m_deviceResources->GetD3DDevice();

m_batch = std::make_unique<PrimitiveBatch<VertexType>>(device);

RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(),
    m_deviceResources->GetDepthBufferFormat());

EffectPipelineStateDescription pd(
    &VertexType::InputLayout,
    CommonStates::Opaque,
    CommonStates::DepthNone,
    CommonStates::CullNone,
    rtState);

m_effect = std::make_unique<BasicEffect>(device, EffectFlags::VertexColor, pd);
```

### Game.cpp — CreateWindowSizeDependentResources

In `CreateWindowSizeDependentResources`, set up a projection matrix so we can use pixel coordinates (matching the SpriteBatch coordinate system):

```cpp
auto size = m_deviceResources->GetOutputSize();

Matrix proj = Matrix::CreateOrthographicOffCenter(
    0.f, float(size.right), float(size.bottom), 0.f, 0.f, 1.f);
m_effect->SetProjection(proj);
```

### Game.cpp — Render

In the `Render` method, after clearing the render target and before `Present`, draw the triangle:

```cpp
m_effect->Apply(commandList);

m_batch->Begin(commandList);

VertexPositionColor v1(Vector3(400.f, 150.f, 0.f), Colors::Red);
VertexPositionColor v2(Vector3(600.f, 450.f, 0.f), Colors::Green);
VertexPositionColor v3(Vector3(200.f, 450.f, 0.f), Colors::Blue);

m_batch->DrawTriangle(v1, v2, v3);

m_batch->End();
```

### Game.cpp — OnDeviceLost

In `OnDeviceLost`, release the resources:

```cpp
m_effect.reset();
m_batch.reset();
```

## Build and Verify

Build the project using the commands from [Step 2](step2.md). When you run the application, you should see a triangle with smoothly interpolated red, green, and blue colors — the classic "rainbow triangle" — rendered on the cornflower blue background.

The per-vertex colors are interpolated by the GPU rasterizer across the triangle surface, producing a smooth gradient between the three corner colors.

## Coordinate Systems

The code above uses **pixel coordinates** (origin at top-left, y-axis pointing down) thanks to the orthographic projection matrix. The triangle vertices are specified in screen pixels, matching the coordinate system used by `SpriteBatch`.

Without the projection matrix, `BasicEffect` uses **normalized device coordinates** (-1 to +1 in both axes, origin at center). You can use either approach depending on your needs.

## More to explore

- [Simple rendering](https://github.com/microsoft/DirectXTK12/wiki/Simple-rendering)
  - Drawing with normalized coordinates
  - State objects and backface culling
  - Drawing with textures
  - Drawing with lighting and normal maps
- [Line drawing and anti-aliasing](https://github.com/microsoft/DirectXTK12/wiki/Line-drawing-and-anti-aliasing)
  - Drawing a grid
  - Anti-aliasing

## Technical Notes

**Ask the user:** Would you like to see the Technical Notes for this step, or skip to the next step?

### Pipeline State Objects (PSOs)

In Direct3D 12, all shader and rendering state must be specified up-front in a monolithic **Pipeline State Object**. This is a critical difference from Direct3D 11 where state could be changed independently at any time.

`EffectPipelineStateDescription` packages together:
- **Input layout** — From `VertexType::InputLayout`, describing vertex element format and offsets
- **Blend state** — From `CommonStates::Opaque` (no alpha blending)
- **Depth/stencil state** — From `CommonStates::DepthNone` (no depth testing for 2D)
- **Rasterizer state** — From `CommonStates::CullNone` (draw both sides)
- **Render target state** — Format of the render target and depth buffer

The `BasicEffect` constructor takes this description and creates the PSO internally, combining it with its built-in root signature and compiled shaders.

> In Direct3D 12, if you want to use different states (e.g., different blend mode or render target format), you must create a new effect instance with a new PSO. You cannot change state after PSO creation.

### BasicEffect

`BasicEffect` in Direct3D 12 manages:

- **Pipeline state object** — Contains the compiled vertex and pixel shader permutation, input layout, and all fixed-function state.
- **Root signature** — Defines the parameter layout for the shaders. Built-in to the effect.
- **Constant buffer** (via `GraphicsMemory`) — Contains the world/view/projection matrices and material colors. Allocated dynamically each frame from the upload heap.

When `Apply(commandList)` is called, it sets the PSO, root signature, and constant buffer on the command list. Unlike Direct3D 11, there is no separate input layout or state to set — it's all in the PSO.

### PrimitiveBatch

`PrimitiveBatch<T>` is a lightweight immediate-mode geometry renderer:

- **Dynamic vertex buffer** (via `GraphicsMemory`) — Allocated from the upload heap each frame. No persistent GPU buffer is needed.
- **Dynamic index buffer** (via `GraphicsMemory`) — Used when drawing indexed primitives.
- **No shader management** — Unlike `SpriteBatch`, `PrimitiveBatch` does not set the PSO or root signature. You must call `IEffect::Apply` yourself.
- **Topology** — Each draw call specifies the primitive topology. `DrawTriangle` uses `D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST`, `DrawLine` uses `LINELIST`.

### The Rendering Pipeline

When you call the render code above, here's what happens at the Direct3D 12 level:

1. `m_effect->Apply(commandList)` — Sets the pipeline state object (PSO), root signature, and uploads the constant buffer with the current projection matrix.
2. `m_batch->Begin(commandList)` — Stores the command list reference for subsequent draw calls.
3. `DrawTriangle` — Allocates space from `GraphicsMemory` for 3 vertices, copies them in, sets the vertex buffer view, primitive topology, and issues a `DrawInstanced(3, ...)` call.
4. `m_batch->End()` — Flushes any remaining batched geometry.
