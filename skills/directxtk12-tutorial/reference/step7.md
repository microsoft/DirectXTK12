# Step 7: 3D Shapes

This step introduces rendering 3D geometry using `GeometricPrimitive`. We create and render a spinning teapot with default lighting. See the wiki page: [3D shapes](https://github.com/microsoft/DirectXTK12/wiki/3D-shapes).

## Background

In the previous step, we used `PrimitiveBatch` to draw geometry with manually specified vertices. `GeometricPrimitive` takes a different approach — it procedurally generates common 3D shapes (sphere, cube, torus, teapot, etc.) and stores them in vertex and index buffers.

Key differences from `PrimitiveBatch`:

| | PrimitiveBatch | GeometricPrimitive |
| --- | --- | --- |
| Buffer type | Dynamic (from upload heap each frame) | Static (can be uploaded to default heap) |
| Index buffer | Optional | Always used |
| Shader setup | Manual (you create and apply effect) | Manual (you create and apply effect) |
| Use case | Immediate-mode debug/simple drawing | Efficient 3D shape rendering |

> Unlike _DirectX Tool Kit for DirectX 11_, you must create your own effect to render with `GeometricPrimitive` in the DirectX 12 version.

## Code Changes

### Game.cpp — Render (comment out previous step)

Comment out or remove the triangle rendering code from Step 6:

```cpp
// m_effect->Apply(commandList);
// m_batch->Begin(commandList);
// ...
// m_batch->End();
```

### pch.h

Ensure the following includes are in `pch.h`:

```cpp
#include <directxtk12/CommonStates.h>
#include <directxtk12/EffectPipelineStateDescription.h>
#include <directxtk12/Effects.h>
#include <directxtk12/GeometricPrimitive.h>
#include <directxtk12/RenderTargetState.h>
#include <directxtk12/SimpleMath.h>
```

### Game.h

Add the following private member variables to the `Game` class:

```cpp
DirectX::SimpleMath::Matrix m_world;
DirectX::SimpleMath::Matrix m_view;
DirectX::SimpleMath::Matrix m_proj;

std::unique_ptr<DirectX::GeometricPrimitive> m_shape;
std::unique_ptr<DirectX::BasicEffect> m_shapeEffect;
```

### Game.cpp — CreateDeviceDependentResources

In `CreateDeviceDependentResources`, create the pipeline state, effect, and shape:

```cpp
auto device = m_deviceResources->GetD3DDevice();

RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(),
    m_deviceResources->GetDepthBufferFormat());

EffectPipelineStateDescription pd(
    &GeometricPrimitive::VertexType::InputLayout,
    CommonStates::Opaque,
    CommonStates::DepthDefault,
    CommonStates::CullNone,
    rtState);

m_shapeEffect = std::make_unique<BasicEffect>(device, EffectFlags::Lighting, pd);
m_shapeEffect->EnableDefaultLighting();

m_shape = GeometricPrimitive::CreateTeapot();

m_world = Matrix::Identity;
```

### Game.cpp — CreateWindowSizeDependentResources

In `CreateWindowSizeDependentResources`, set up a 3D camera with a perspective projection:

```cpp
auto size = m_deviceResources->GetOutputSize();

m_view = Matrix::CreateLookAt(
    Vector3(2.f, 2.f, 2.f),
    Vector3::Zero,
    Vector3::UnitY);

m_proj = Matrix::CreatePerspectiveFieldOfView(
    XM_PI / 4.f,
    float(size.right) / float(size.bottom),
    0.1f, 10.f);

m_shapeEffect->SetView(m_view);
m_shapeEffect->SetProjection(m_proj);
```

### Game.cpp — Update

In `Update`, rotate the shape over time:

```cpp
auto time = static_cast<float>(timer.GetTotalSeconds());

m_world = Matrix::CreateRotationZ(cosf(time) * 2.f);
```

### Game.cpp — Render

In the `Render` method, after clearing the render target and before `Present`, draw the shape:

```cpp
m_shapeEffect->SetWorld(m_world);

m_shapeEffect->Apply(commandList);

m_shape->Draw(commandList);
```

### Game.cpp — OnDeviceLost

In `OnDeviceLost`, release the shape and effect:

```cpp
m_shape.reset();
m_shapeEffect.reset();
```

## Build and Verify

Build the project using the commands from [Step 2](step2.md). When you run the application, you should see a white, lit teapot rotating on the cornflower blue background.

The teapot is rendered with default lighting (three directional lights) and backface culling disabled, giving it a solid 3D appearance.

## Optimizing with Static Buffers

By default, `GeometricPrimitive` creates its vertex and index buffers using `GraphicsMemory` (upload heap memory). You can upload them to dedicated video memory for faster render performance by using `ResourceUploadBatch`:

```cpp
m_shape = GeometricPrimitive::CreateTeapot();

ResourceUploadBatch resourceUpload(device);

resourceUpload.Begin();

m_shape->LoadStaticBuffers(device, resourceUpload);

auto uploadResourcesFinished = resourceUpload.End(
    m_deviceResources->GetCommandQueue());

uploadResourcesFinished.wait();
```

## Available Shapes

`GeometricPrimitive` can create many shapes. Replace `CreateTeapot` with any of these:

| Factory Method | Shape |
| --- | --- |
| `CreateSphere` | UV sphere |
| `CreateGeoSphere` | Geodesic sphere |
| `CreateCube` | Unit cube |
| `CreateBox` | Axis-aligned box with custom dimensions |
| `CreateCylinder` | Cylinder |
| `CreateCone` | Cone |
| `CreateTorus` | Torus (donut) |
| `CreateTetrahedron` | Tetrahedron (4 faces) |
| `CreateOctahedron` | Octahedron (8 faces) |
| `CreateDodecahedron` | Dodecahedron (12 faces) |
| `CreateIcosahedron` | Icosahedron (20 faces) |
| `CreateTeapot` | Utah teapot |

## Next Steps

See [Next Steps](nextsteps.md) for where to go from here.

## Further Reading

- [3D shapes](https://github.com/microsoft/DirectXTK12/wiki/3D-shapes)
  - Adding textures to shapes
  - Using custom lighting and effects
  - Custom vertex formats
  - Custom geometry
- [Rendering a model](https://github.com/microsoft/DirectXTK12/wiki/Rendering-a-model)

## Technical Notes

**Ask the user:** Would you like to see the Technical Notes for this step, or skip to the next step?

### GeometricPrimitive

`GeometricPrimitive` procedurally generates indexed triangle meshes:

- **Vertex data** — `VertexPositionNormalTexture` format (position, normal, and texture coordinates).
- **Index data** — `uint16_t` indices for indexed rendering, reducing vertex count by sharing vertices between adjacent triangles.
- **Default storage** — Uses `GraphicsMemory` (upload heap) for immediate rendering.
- **Static buffers** — `LoadStaticBuffers` creates `ID3D12Resource` committed resources in the default heap (fast GPU memory) and uses `ResourceUploadBatch` to copy data there.

Unlike the Direct3D 11 version, you must provide your own effect. The DX11 version had a convenience `Draw(world, view, proj)` overload that managed an internal `BasicEffect`, but the DX12 version requires explicit effect and PSO management.

### The 3D Camera

This step introduces a proper 3D camera setup:

- **View matrix** (`CreateLookAt`) — Defines where the camera is (eye position), what it's looking at (focus point), and which direction is "up". This transforms world-space coordinates into camera-space (view-space).
- **Projection matrix** (`CreatePerspectiveFieldOfView`) — Defines the camera's field of view, aspect ratio, and near/far clipping planes. This transforms view-space into clip-space, applying perspective foreshortening so distant objects appear smaller.

Together, these matrices transform the shape's local vertex positions through: **World → View → Projection → Clip Space** → (GPU viewport transform) → **Screen pixels**.

### Upload Heap vs. Default Heap

- **Upload heap** (`GraphicsMemory`) — CPU-writable memory accessible by the GPU. Good for data that changes per-frame (constant buffers, dynamic geometry). This is what `PrimitiveBatch` and default `GeometricPrimitive` use.
- **Default heap** (via `LoadStaticBuffers`) — GPU-only memory. Data must be copied there via a command list. Much faster for the GPU to read, but requires explicit upload management. Used for static geometry that doesn't change.

### The Utah Teapot

The [Utah teapot](https://en.wikipedia.org/wiki/Utah_teapot) is a classic test model in computer graphics, originally created by Martin Newell in 1975. DirectX Tool Kit generates it from Bézier patch data, tessellated to the specified level (default: 8).
