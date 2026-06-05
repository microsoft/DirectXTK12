---
name: directxtk12-usage
description: >-
  Guide for integrating DirectX Tool Kit for DirectX 12 into new projects and understanding the library's API surface.
  Use this skill when asked about how to use DirectXTK12, set up a new project, or get an overview of
  available classes and functionality.
license: MIT
metadata:
  author: chuckw
  version: "1.0"
---

# DirectX Tool Kit for DirectX 12 — Usage Guide

## Overview

The *DirectX Tool Kit for DirectX 12* (DirectXTK12) is a collection of helper classes for writing Direct3D 12 C++ code for Win32 desktop applications (Windows 10+), Xbox Series X|S, Xbox One, and Universal Windows Platform (UWP) apps.

- **Repository**: <https://github.com/microsoft/DirectXTK12>
- **Documentation**: <https://github.com/microsoft/DirectXTK12/wiki>
- **NuGet Packages**: `directxtk12_desktop_win10`, `directxtk12_uwp`
- **vcpkg Port**: `directxtk12`

## Integration Methods

### vcpkg manifest-mode (Recommended)

In your `vcpkg.json` file, add the following:

```json
{
  "$schema": "https://raw.githubusercontent.com/microsoft/vcpkg-tool/main/docs/vcpkg.schema.json",
  "dependencies": [
    "directx-headers",
    "directxmath",
    "directxtk12"
  ]
}
```

If using GameInput for the game input functionality, add the following:

```json
{
  "$schema": "https://raw.githubusercontent.com/microsoft/vcpkg-tool/main/docs/vcpkg.schema.json",
  "dependencies": [
    "directx-headers",
    "directxmath",
    {
      "name": "directxtk12",
      "default-features": false,
      "features": [
        "gameinput"
      ]
    }
  ]
}
```

If using DirectX Tool Kit for Audio and GameInput, add the following:

```json
{
  "$schema": "https://raw.githubusercontent.com/microsoft/vcpkg-tool/main/docs/vcpkg.schema.json",
  "dependencies": [
    "directx-headers",
    "directxmath",
    {
      "name": "directxtk12",
      "default-features": false,
      "features": [
        "gameinput",
        "xaudio2-9"
      ]
    }
  ]
}
```

### vcpkg (classic)

```bash
vcpkg install directxtk12
```

Features: `xaudio2-9` (DirectX Tool Kit for Audio using XAudio 2.9), `gameinput` (Using GameInput for gamepad, keyboard, and mouse), `tools` (command-line tools). Triplets: `x64-windows`, `arm64-windows`, etc.

For DLL usage (`x64-windows` default triplet), define `DIRECTX_TOOLKIT_IMPORT` in your consuming project. For static library usage, use `-static-md` triplet variants.

CMakeLists.txt:

```cmake
find_package(directxtk12 CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE Microsoft::DirectXTK12)
```

Use the [d3d12game_vcpkg](https://github.com/walbourn/directx-vs-templates/tree/main/d3d12game_vcpkg) template as a starting point.

### NuGet

Use `directxtk12_desktop_win10` for Win32 desktop applications or `directxtk12_uwp` for UWP apps.

### Project Reference

Add the appropriate `.vcxproj` from the `DirectXTK12/` folder to your solution and add a project reference. Add the `DirectXTK12\Inc` directory to your Additional Include Directories.

## Minimum Requirements

- Windows 10 May 2020 Update SDK (19041) or later
- Visual Studio 2022, Visual Studio 2026, clang for Windows v12+, or MinGW 12.2
- Direct3D Feature Level 11.0 or higher

## Getting Started

For a full step-by-step walkthrough, see the [Getting Started](https://github.com/microsoft/DirectXTK12/wiki/Getting-Started) tutorial on the wiki.

A minimal initialization sequence:

1. Create a `ID3D12Device`.
2. Create a `GraphicsMemory` instance (one per device).
3. Create a `DescriptorHeap` for SRV/CBV/UAV descriptors.
4. Use `ResourceUploadBatch` to upload textures and static buffers to the GPU.
5. Create rendering helpers (e.g., `SpriteBatch`, `Effects`, `Model`) with the appropriate pipeline state.
6. Each frame, call `GraphicsMemory::Commit` after executing command lists.

## API Reference

The public API is defined in the header files in the `Inc/` directory. See the [reference overview](reference/overview.md) for a categorized summary of all classes and helpers.

Full documentation for each class is available on the [GitHub wiki](https://github.com/microsoft/DirectXTK12/wiki).

API signatures are defined in the public headers under the `Inc/` directory. Always consult those headers for the authoritative function signatures, parameters, and SAL annotations.

## Key Concepts

### Resource Ownership

DirectXTK12 classes follow RAII principles. Most objects are created with `std::make_unique` and destroyed automatically. COM resources are managed with `Microsoft::WRL::ComPtr`.

### Device-Dependent vs Device-Independent

Most DirectXTK12 objects are **device-dependent** — they are created with a `ID3D12Device*` and must be recreated if the device is lost. Plan your resource management accordingly.

### Thread Safety

DirectXTK12 rendering classes (SpriteBatch, Effects, PrimitiveBatch, etc.) are **not** thread-safe. Use one instance per thread, or synchronize access externally. Resource creation (texture loading, model loading) can be done from any thread.

## Key Concepts for DirectX 12 Users

- **Pipeline State Objects (PSOs)**: Unlike the DX11 version, DX12 requires explicit PSO management. Use `EffectPipelineStateDescription` and `RenderTargetState` to configure PSOs for effects and rendering helpers.
- **Descriptor Heaps**: Use `DescriptorHeap` to manage shader-visible descriptor heaps. Most rendering classes require descriptor heap indices at draw time.
- **Resource Upload**: GPU resources must be uploaded explicitly. `ResourceUploadBatch` batches uploads into a single command list for efficiency.
- **Graphics Memory**: `GraphicsMemory` manages per-frame dynamic allocations (constant buffers, dynamic vertex/index buffers). Call `Commit` once per frame.

## Namespace

All classes and functions reside in the `DirectX` namespace. Headers that contain Direct3D 12-specific types use `inline namespace DX12` to avoid conflicts when both DX11 and DX12 toolkits are linked together.

```cpp
#include "SpriteBatch.h"

// Usage:
auto spriteBatch = std::make_unique<DirectX::SpriteBatch>(device, ...);
```

## Common Patterns

### Creating an Effect

```cpp
#include "Effects.h"
#include "EffectPipelineStateDescription.h"
#include "RenderTargetState.h"

RenderTargetState rtState(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_D32_FLOAT);

EffectPipelineStateDescription pd(
    &VertexPositionNormalTexture::InputLayout,
    CommonStates::Opaque,
    CommonStates::DepthDefault,
    CommonStates::CullCounterClockwise,
    rtState);

auto effect = std::make_unique<BasicEffect>(device, EffectFlags::Lighting | EffectFlags::Texture, pd);
```

### Loading and Drawing a Model

```cpp
#include "Model.h"

auto model = Model::CreateFromSDKMESH(device, L"mymodel.sdkmesh");

// Upload resources
ResourceUploadBatch upload(device);
upload.Begin();
model->LoadStaticBuffers(device, upload);
upload.End(commandQueue).wait();

// Draw (simplified — see wiki for full parameter details)
model->Draw(commandList, ...);
```

### Texture Loading

```cpp
#include "DDSTextureLoader.h"
#include "ResourceUploadBatch.h"
#include "DescriptorHeap.h"

ResourceUploadBatch upload(device);
upload.Begin();

Microsoft::WRL::ComPtr<ID3D12Resource> texture;
CreateDDSTextureFromFile(device, upload, L"texture.dds", texture.ReleaseAndGetAddressOf());

upload.End(commandQueue).wait();
```

### Input Handling

```cpp
#include "Keyboard.h"
#include "Mouse.h"
#include "GamePad.h"

auto keyboard = std::make_unique<DirectX::Keyboard>();
auto mouse = std::make_unique<DirectX::Mouse>();
auto gamePad = std::make_unique<DirectX::GamePad>();

// In update loop
auto kb = keyboard->GetState();
if (kb.Escape)
    PostQuitMessage(0);

auto pad = gamePad->GetState(0);
if (pad.IsConnected())
{
    if (pad.IsAPressed())
        /* jump */;
}
```

### Audio

```cpp
#include "Audio.h"

// Create audio engine
auto audioEngine = std::make_unique<DirectX::AudioEngine>();

// Load and play a sound
auto soundEffect = std::make_unique<DirectX::SoundEffect>(audioEngine.get(), L"explosion.wav");
soundEffect->Play();

// Per-frame update
audioEngine->Update();
```

> **Note**: Code examples above are simplified for clarity. Consult the [wiki tutorials](https://github.com/microsoft/DirectXTK12/wiki/Getting-Started) and public headers in `Inc/` for complete working code with all required parameters.

## Further Reading

- [DirectXTK Wiki](https://github.com/microsoft/DirectXTK12/wiki)
- [DirectX Tool Kit for Audio](https://github.com/Microsoft/DirectXTK/wiki/Audio)
- [SimpleMath documentation](https://github.com/Microsoft/DirectXTK/wiki/SimpleMath)
- [Project templates](https://github.com/walbourn/directx-vs-templates/tree/main/d3d12game_vcpkg)
