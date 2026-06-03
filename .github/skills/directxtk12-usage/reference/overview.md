# API Reference Overview

This document provides a high-level overview of the DirectX Tool Kit for DirectX 12 public API. For authoritative function signatures, parameters, and SAL annotations, consult the public headers in the `Inc/` directory.

For detailed documentation on each class, see the [DirectXTK12 Wiki](https://github.com/microsoft/DirectXTK12/wiki).

## Resource Management

### GraphicsMemory (`Inc/GraphicsMemory.h`)

Manages per-frame dynamic graphics memory allocation for constant buffers, dynamic vertex buffers, and dynamic index buffers. One instance should be created per `ID3D12Device`.

- Call `Commit` once per frame after executing command lists to reclaim memory from completed frames.
- Provides `GraphicsResource` for sub-allocated memory regions.

### ResourceUploadBatch (`Inc/ResourceUploadBatch.h`)

Batches GPU resource uploads (textures, static buffers) into a single command list for efficient submission.

- Call `Begin` to start recording uploads.
- Call `End` to submit the upload command list; returns a future to wait on.
- Supports automatic mipmap generation during upload.

### DescriptorHeap (`Inc/DescriptorHeap.h`)

Helper for creating and managing Direct3D 12 descriptor heaps (SRV/CBV/UAV, Sampler, RTV, DSV).

- Simplifies index-based descriptor allocation.
- Provides CPU and GPU handle accessors by index.

### BufferHelpers (`Inc/BufferHelpers.h`)

Free functions for creating static and constant buffers from CPU data.

- `CreateStaticBuffer` — uploads vertex/index/structured buffer data.
- `CreateTextureFromMemory` — creates a texture resource from in-memory data.

### DirectXHelpers (`Inc/DirectXHelpers.h`)

Miscellaneous utility functions for Direct3D 12 programming.

- Resource barrier helpers.
- Shader resource view creation helpers.

## Pipeline State

### RenderTargetState (`Inc/RenderTargetState.h`)

A lightweight struct that communicates render target format and sample count information needed when creating PSOs.

### EffectPipelineStateDescription (`Inc/EffectPipelineStateDescription.h`)

Bundles input layout, blend state, depth-stencil state, rasterizer state, and render target state into a single description used by effect constructors to create PSOs.

## Rendering

| Header | Classes / Functions | Purpose |
| --- | --- | --- |
| `SpriteBatch.h` | `SpriteBatch` | Hardware-accelerated 2D sprite rendering with batching, rotation, and scaling. |
| `SpriteFont.h` | `SpriteFont` | Bitmap font rendering using SpriteBatch. Supports MakeSpriteFont and BMFont formats. |
| `PrimitiveBatch.h` | `PrimitiveBatch<T>` | Low-level immediate-mode rendering of user-defined vertex types. |
| `GeometricPrimitive.h` | `GeometricPrimitive` | Factory for common 3D shapes (sphere, cube, cylinder, torus, teapot, etc.). |
| `Model.h` | `Model`, `ModelMesh`, `ModelMeshPart`, `ModelBone` | Loading and rendering of 3D models from CMO, SDKMESH and VBO file formats. |

> Model requires RTTI (`/GR`) to be enabled.

## Effects (Shaders)

| Header | Classes | Purpose |
| --- | --- | --- |
| `Effects.h` | `BasicEffect` | Per-pixel lighting with texture, vertex color, and fog support. |
| | `AlphaTestEffect` | Alpha testing (clip pixels below a threshold). |
| | `DualTextureEffect` | Two-layer multitexture blending (e.g., lightmaps). |
| | `EnvironmentMapEffect` | Cubic environment map reflections. |
| | `SkinnedEffect` | Vertex skinning with up to 72 bones. |
| | `SkinnedNormalMapEffect` | Skinned rendering with normal maps. |
| | `NormalMapEffect` | Normal map and optional specular map rendering. |
| | `PBREffect` | Physically-based rendering (metalness/roughness workflow). |
| | `SkinnedPBREffect` | PBR with vertex skinning. |
| | `DebugEffect` | Diagnostic visualization (normals, tangents, texture coordinates). |
| | `IEffect`, `IEffectMatrices`, `IEffectLights`, `IEffectFog`, `IEffectSkinning` | Interfaces implemented by effects. |
| | `IEffectFactory`, `EffectFactory`, `PBREffectFactory` | Model-driven effect creation and caching. |
| | `IEffectTextureFactory`, `EffectTextureFactory` | Texture loading and caching for models. |

Effects are constructed with an `EffectFlags` bitmask and an `EffectPipelineStateDescription`. Each instance of an effect internally creates a ID3D12PipelineState object, so is immutable after creation.

## Textures

| Header | Functions | Purpose |
| --- | --- | --- |
| `DDSTextureLoader.h` | `CreateDDSTextureFromFile`, `CreateDDSTextureFromMemory` | Load DDS textures (supports all D3D12 formats, mipmaps, cubemaps, arrays). |
| `WICTextureLoader.h` | `CreateWICTextureFromFile`, `CreateWICTextureFromMemory` | Load common image formats (PNG, JPEG, BMP, TIFF, GIF) via WIC. |
| `XboxDDSTextureLoader.h` | `CreateDDSTextureFromFile`, `CreateDDSTextureFromMemory` | Xbox-optimized DDS loader for tiled resources. |
| `ScreenGrab.h` | `SaveDDSTextureToFile`, `SaveWICTextureToFile` | Capture render targets to DDS or WIC image files. |

## States and Helpers

| Header | Classes / Functions | Purpose |
| --- | --- | --- |
| `CommonStates.h` | `CommonStates` | Pre-built blend, depth-stencil, rasterizer, and sampler state objects. |
| `DirectXHelpers.h` | `CreateInputLayoutFromEffect`, `SetDebugObjectName` | Utility functions for input layouts, debug naming, and resource manipulation. |
| `BufferHelpers.h` | `CreateStaticBuffer`, `CreateTextureFromMemory` | Helper functions for creating D3D12 buffers and textures. |
| `GraphicsMemory.h` | `GraphicsMemory` | Per-frame memory management for dynamic resources. |
| `VertexTypes.h` | `VertexPosition`, `VertexPositionColor`, `VertexPositionTexture`, `VertexPositionNormal`, `VertexPositionNormalTexture`, `VertexPositionNormalColor`, `VertexPositionNormalColorTexture`, `VertexPositionNormalTangentColorTexture`, `VertexPositionNormalTangentColorTextureSkinning` | Pre-defined vertex structure types compatible with the built-in effects provided with `D3D12_INPUT_LAYOUT_DESC`. |

## Post-Processing

| Header | Classes | Purpose |
| --- | --- | --- |
| `PostProcess.h` | `IPostProcess` | Base interface for post-processing effects. |
| | `BasicPostProcess` | Copy, monochrome, sepia, down-scale, bloom extract/blur. |
| | `DualPostProcess` | Merge/blend two textures (bloom combine, weighted average). |
| | `ToneMapPostProcess` | HDR tone mapping (Reinhard, ACESFilmic, etc.) and OETF curves. |

## Input

| Header | Classes | Purpose |
| --- | --- | --- |
| `Keyboard.h` | `Keyboard`, `Keyboard::State`, `Keyboard::KeyboardStateTracker` | Keyboard state polling and key-press/release tracking. |
| `Mouse.h` | `Mouse`, `Mouse::State`, `Mouse::ButtonStateTracker` | Mouse state polling with absolute and relative modes. |
| `GamePad.h` | `GamePad`, `GamePad::State`, `GamePad::ButtonStateTracker` | Gamepad state polling, vibration, and button tracking. Supports GameInput, XInput, and Windows.Gaming.Input backends. |

See [DirectX Tool Kit wiki: GamePad](https://github.com/Microsoft/DirectXTK/wiki/GamePad), [DirectX Tool Kit wiki: Keyboard](https://github.com/Microsoft/DirectXTK/wiki/Keyboard), and [DirectX Tool Kit wiki: Mouse](https://github.com/Microsoft/DirectXTK/wiki/Mouse) for full documentation.

## Audio

| Header | Classes | Purpose |
| --- | --- | --- |
| `Audio.h` | `AudioEngine` | XAudio2-based audio engine with 3D audio, mastering voice, and device management. |
| | `SoundEffect` | Loads WAV files and XACT-style wave banks for playback. |
| | `SoundEffectInstance` | Controls playback of a sound (play, pause, stop, loop, volume, pitch, pan). |
| | `SoundStreamInstance` | Streaming playback from disk for large audio files. |
| | `WaveBank` | Loads XACT-style wave bank (.xwb) files containing multiple sounds. |
| | `DynamicSoundEffectInstance` | Programmatic audio generation via callback-driven buffer submission. |
| | `AudioListener`, `AudioEmitter` | 3D audio positioning for spatialized sound. |


See the [DirectX Tool Kit for Audio wiki](https://github.com/Microsoft/DirectXTK/wiki/Audio) for full documentation.

## Math

| Header | Classes / Types | Purpose |
| --- | --- | --- |
| `SimpleMath.h` | `Vector2`, `Vector3`, `Vector4`, `Matrix`, `Quaternion`, `Plane`, `Color`, `Ray`, `Viewport`, `Rectangle` | Wrapper types around DirectXMath (XMFLOAT/XMVECTOR) providing operator overloads and convenience methods. |

All types support standard arithmetic operators and implicit conversion to/from DirectXMath SIMD types (`XMVECTOR`, `XMMATRIX`).

See the [SimpleMath wiki](https://github.com/Microsoft/DirectXTK/wiki/SimpleMath) for full documentation.

## Header Inclusion

All public headers are standalone — include only what you need:

```cpp
#include "SpriteBatch.h"    // For 2D sprite rendering
#include "Effects.h"        // For shader effects
#include "Model.h"          // For 3D model loading
#include "Audio.h"          // For audio playback
#include "SimpleMath.h"     // For math helper types
```

There is no single umbrella header. Each header declares its dependencies via forward declarations or includes as needed.

### VCPKG Usage

When using VCPKG with MSBuild integration (both classic and manifest mode), the public headers must be included with a `directxtk12/` prefix.

```cpp
#include "directxtk12/SpriteBatch.h"
#include "directxtk12/Effects.h"
#include "directxtk12/Model.h"
#include "directxtk12/Audio.h"
#include "directxtk12/SimpleMath.h"
```

When using VCPKG with CMake integration via `find_package`, the include path is added directly so the prefix is not required, but will generally work as well.
