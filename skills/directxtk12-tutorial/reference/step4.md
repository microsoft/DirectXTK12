# Step 4: Sprites and Textures

This step adds 2D sprite rendering using `SpriteBatch` and loads a texture with `WICTextureLoader`. See the wiki page: [Sprites and textures](https://github.com/microsoft/DirectXTK12/wiki/Sprites-and-textures).

## Setup

Copy the `cat.png` file from this skill's `assets/` folder into the project's working directory (next to the executable, or in the project root for CMake builds):

```powershell
Copy-Item "<SkillDir>\assets\cat.png" -Destination "<ProjectDir>"
```

## Background

In Direct3D 12, rendering a sprite requires significantly more explicit setup than in Direct3D 11:

- A **committed resource** (the texture) containing the bitmap pixel data
- A **shader resource view descriptor** in a **descriptor heap** describing the texture
- A **pipeline state object** encapsulating all rendering state, shaders, and vertex layout
- A **root signature** defining how the GPU accesses resources
- A **ResourceUploadBatch** to manage the texture upload to GPU memory

DirectX Tool Kit's `SpriteBatch` handles the pipeline state, root signature, vertex buffer, and draw calls. You just need to manage the texture resource, descriptor heap, and upload.

## Code Changes

### pch.h

Ensure the following includes are in `pch.h` (they should already be there from Step 3):

```cpp
#include <directxtk12/CommonStates.h>
#include <directxtk12/DescriptorHeap.h>
#include <directxtk12/DirectXHelpers.h>
#include <directxtk12/GraphicsMemory.h>
#include <directxtk12/RenderTargetState.h>
#include <directxtk12/ResourceUploadBatch.h>
#include <directxtk12/SimpleMath.h>
#include <directxtk12/SpriteBatch.h>
#include <directxtk12/WICTextureLoader.h>
```

### Game.h

Add the following private member variables to the `Game` class:

```cpp
std::unique_ptr<DirectX::DescriptorHeap> m_resourceDescriptors;
std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch;
Microsoft::WRL::ComPtr<ID3D12Resource> m_texture;
DirectX::SimpleMath::Vector2 m_screenPos;
DirectX::SimpleMath::Vector2 m_origin;

enum Descriptors
{
    Cat,
    Count
};
```

### Game.cpp — CreateDeviceDependentResources

In `CreateDeviceDependentResources`, after creating `m_graphicsMemory`, create the descriptor heap, load the texture, create the SpriteBatch, and compute the sprite origin:

```cpp
m_resourceDescriptors = std::make_unique<DescriptorHeap>(device,
    Descriptors::Count);

ResourceUploadBatch resourceUpload(device);

resourceUpload.Begin();

DX::ThrowIfFailed(
    CreateWICTextureFromFile(device, resourceUpload, L"cat.png",
    m_texture.ReleaseAndGetAddressOf()));

CreateShaderResourceView(device, m_texture.Get(),
    m_resourceDescriptors->GetCpuHandle(Descriptors::Cat));

RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(),
    m_deviceResources->GetDepthBufferFormat());

SpriteBatchPipelineStateDescription pd(rtState,
    &CommonStates::NonPremultiplied);
m_spriteBatch = std::make_unique<SpriteBatch>(device, resourceUpload, pd);

auto uploadResourcesFinished = resourceUpload.End(
    m_deviceResources->GetCommandQueue());

uploadResourcesFinished.wait();

XMUINT2 catSize = GetTextureSize(m_texture.Get());

m_origin.x = float(catSize.x / 2);
m_origin.y = float(catSize.y / 2);
```

### Game.cpp — CreateWindowSizeDependentResources

In `CreateWindowSizeDependentResources`, set up the viewport and compute the screen center position:

```cpp
auto viewport = m_deviceResources->GetScreenViewport();
m_spriteBatch->SetViewport(viewport);

auto size = m_deviceResources->GetOutputSize();
m_screenPos.x = float(size.right) / 2.f;
m_screenPos.y = float(size.bottom) / 2.f;
```

### Game.cpp — Render

In the `Render` method, after clearing the render target and before `Present`, draw the sprite:

```cpp
ID3D12DescriptorHeap* heaps[] = { m_resourceDescriptors->Heap() };
commandList->SetDescriptorHeaps(static_cast<UINT>(std::size(heaps)), heaps);

m_spriteBatch->Begin(commandList);

m_spriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Cat),
    GetTextureSize(m_texture.Get()),
    m_screenPos, nullptr, Colors::White, 0.f, m_origin);

m_spriteBatch->End();
```

### Game.cpp — OnDeviceLost

In `OnDeviceLost`, release the resources:

```cpp
m_spriteBatch.reset();
m_texture.Reset();
m_resourceDescriptors.reset();
```

## Build and Verify

Build the project using the commands from [Step 2](step2.md). To run the application, make sure the working directory is set to the project folder (where `cat.png` is located):

```powershell
cd <ProjectDir>
```

For MSBuild, the executable will be in a subdirectory like `x64\Debug\<ProjectName>.exe`. For CMake, it will be in `out\build\x64-Debug\<ProjectName>.exe` (or the ARM64 equivalent).

When you run the application, you should see the cat image rendered in the center of the window on top of the cornflower blue background.

## Further Reading

For more things you can do with sprites at this point:

- [Sprites and textures](https://github.com/microsoft/DirectXTK12/wiki/Sprites-and-textures)
  - Using DDS files for textures
  - Using pre-multiplied alpha
  - Rotating a sprite
  - Scaling a sprite
  - Tinting a sprite
  - Tiling a sprite
  - Stretching a sprite
  - Drawing a background image
- [More tricks with sprites](https://github.com/microsoft/DirectXTK/wiki/More-tricks-with-sprites)
  - Animating sprites
  - Creating a scrolling background
  - More to explore

## Technical Notes

**Ask the user:** Would you like to see the Technical Notes for this step, or skip to the next step?

### Key Differences from Direct3D 11

In Direct3D 11, `SpriteBatch` could be created with just a device context and would manage all its own state. In Direct3D 12:

- You must create a **descriptor heap** and manage texture descriptors yourself
- You must create a **ResourceUploadBatch** to upload textures to GPU memory
- You must provide a **RenderTargetState** and create a pipeline state description when constructing `SpriteBatch`
- You must call `SetDescriptorHeaps` on the command list before drawing
- `SpriteBatch::Begin` takes a command list parameter instead of optional state overrides

### SpriteBatch

`SpriteBatch` is a high-performance batched 2D renderer. In Direct3D 12, it manages:

- **Pipeline state object (PSO)** — Contains the compiled vertex and pixel shaders, input layout, blend state, depth/stencil state, and rasterizer state. Created at construction time from the `SpriteBatchPipelineStateDescription`.
- **Root signature** — Defines how shader parameters are bound. SpriteBatch uses a built-in root signature with a descriptor table for the texture SRV and a sampler.
- **Dynamic vertex buffer** (via `GraphicsMemory`) — Holds batched sprite quads. Uses sub-allocation from the upload heap managed by `GraphicsMemory`.
- **Index buffer** — Pre-generated indices for drawing quads as triangle lists.
- **Constant buffer** (via `GraphicsMemory`) — Contains the orthographic projection matrix for screen-space rendering.

When you call `Begin(commandList)`/`End()`, SpriteBatch sets the pipeline state, root signature, and descriptor table bindings on the command list, then issues `DrawIndexedInstanced` calls, batching sprites that share the same texture into a single draw call.

### ResourceUploadBatch

`ResourceUploadBatch` manages the process of uploading texture data from CPU memory to GPU-accessible memory:

1. `Begin()` — Starts recording upload commands.
2. Texture creation functions place data in an intermediate upload heap and record copy commands.
3. `End(commandQueue)` — Submits the copy commands to the GPU and returns a `std::future<void>` for synchronization.
4. `.wait()` — Blocks until the GPU has completed all uploads.

### DescriptorHeap

`DescriptorHeap` is a simple wrapper around `ID3D12DescriptorHeap` that provides easy access to CPU and GPU descriptor handles by index. In Direct3D 12, textures are not bound directly — instead, a shader resource view (SRV) descriptor in a heap describes the texture, and the heap is set on the command list.

### WICTextureLoader

`CreateWICTextureFromFile` in DirectX 12 works differently from DirectX 11:

- Creates an **`ID3D12Resource`** (committed resource) in the default heap as the final texture destination.
- Creates a temporary upload buffer in the upload heap.
- Records a copy command in the `ResourceUploadBatch` to transfer data from the upload heap to the default heap.
- The **shader resource view** must be created separately using `CreateShaderResourceView` (unlike DirectX 11 which created it automatically).
