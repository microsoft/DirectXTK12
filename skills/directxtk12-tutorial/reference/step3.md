# Step 3: Adding DirectX Tool Kit

Now add DirectX Tool Kit for DirectX 12 to the project via vcpkg. Edit the `vcpkg.json` manifest in the project root to add the `directxtk12` port with GameInput support.

Add the following entry to the `"dependencies"` array in `vcpkg.json`:

```json
{
  "name": "directxtk12",
  "default-features": false,
  "features": [
    "gameinput"
  ]
}
```

For example, the full `vcpkg.json` should look something like:

```json
{
  "$schema": "https://raw.githubusercontent.com/microsoft/vcpkg-tool/main/docs/vcpkg.schema.json",
  "dependencies": [
    {
      "name": "directx-dxc",
      "host": true
    },
    "directx-dxc",
    "directx-headers",
    "directx12-agility",
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

## CMake Projects

For CMake projects, also add the following to `CMakeLists.txt`:

```cmake
find_package(directxtk12 CONFIG REQUIRED)
find_package(gameinput CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Microsoft::DirectXTK12)
```

## Build and Verify

Build the project to verify everything works. Use the same commands from [Step 2](step2.md).

### MSBuild

For x64 systems:

```powershell
msbuild <ProjectName>.vcxproj /p:Configuration=Debug /p:Platform=x64
```

For ARM64 systems:

```powershell
msbuild <ProjectName>.vcxproj /p:Configuration=Debug /p:Platform=ARM64
```

### CMake

For x64 systems:

```powershell
cmake --preset=x64-Debug
cmake --build out\build\x64-Debug
```

For ARM64 systems:

```powershell
cmake --preset=arm64-Debug
cmake --build out\build\arm64-Debug
```

> The first rebuild will take longer as vcpkg fetches and builds DirectXTK12 and its dependencies (including GameInput).

## Verifying the Integration

Once the build succeeds, verify that the DirectXTK12 headers are available by adding a test include to `pch.h`:

```cpp
#include <directxtk12/SpriteBatch.h>
```

> **Important:** Use `<directxtk12/...>` include style (not `"SpriteBatch.h"`) for proper MSBuild+vcpkg integration.

If the build still succeeds, DirectXTK12 is properly integrated.

## Adding the Headers and GraphicsMemory

General advice for C++ projects is that you should only add the headers you actually use, but to simplify the tutorial we will go ahead and add the common ones to `pch.h`:

```cpp
#include <directxtk12/CommonStates.h>
#include <directxtk12/DDSTextureLoader.h>
#include <directxtk12/DescriptorHeap.h>
#include <directxtk12/DirectXHelpers.h>
#include <directxtk12/EffectPipelineStateDescription.h>
#include <directxtk12/Effects.h>
#include <directxtk12/GeometricPrimitive.h>
#include <directxtk12/GraphicsMemory.h>
#include <directxtk12/PrimitiveBatch.h>
#include <directxtk12/RenderTargetState.h>
#include <directxtk12/ResourceUploadBatch.h>
#include <directxtk12/SimpleMath.h>
#include <directxtk12/SpriteBatch.h>
#include <directxtk12/SpriteFont.h>
#include <directxtk12/VertexTypes.h>
#include <directxtk12/WICTextureLoader.h>
```

Next, add C++ namespace using statements to `Game.cpp` to make it easier to use the types:

```cpp
using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;
```

### Setting Up GraphicsMemory

In DirectX 12, the application must manage the lifetime of all video memory resources. The `GraphicsMemory` class is a helper for managing dynamic allocations (constant buffers, dynamic vertex/index buffers, upload heaps). It must be created once and have `Commit` called once per frame.

In **Game.h**, add the following private member variable:

```cpp
std::unique_ptr<DirectX::GraphicsMemory> m_graphicsMemory;
```

In **Game.cpp**, add to the TODO of **CreateDeviceDependentResources**:

```cpp
m_graphicsMemory = std::make_unique<GraphicsMemory>(device);
```

In **Game.cpp**, in **Render** add a call to `GraphicsMemory::Commit` right after the present:

```cpp
// Show the new frame.
m_deviceResources->Present();
m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
```

In **Game.cpp**, add to the TODO of **OnDeviceLost**:

```cpp
m_graphicsMemory.reset();
```

## Technical Notes

**Ask the user:** Would you like to see the Technical Notes for this step, or skip to the next step?

- **Static linking** — The MSBuild template uses `-static-md` vcpkg triplets, so the DirectXTK12 library is statically linked. For CMake, the `DIRECTX_TOOLKIT_IMPORT` define is handled automatically by the CMake targets when using DLL triplets.

- **Header location** — The DirectXTK12 headers are installed in the `include/directxtk12` directory of the `vcpkg_installed` folder for the given host/target triplet combination. The VCPKG+MSBuild integration only adds the root 'include' folder to the search paths for the build, so we have to specify the subdirectory explicitly. For CMake, the CMake targets automatically handle the specific header location, but with the use of the directxmath vcpkg port it also has the root folder in the search paths.

- **GameInput** — A modern input API that provides a unified interface for gamepads, keyboards, and mice. It replaces the older XInput and raw input APIs. The vcpkg `gameinput` feature links the GameInput redistributable library.

> **Important:** You may need to run GameInputRedist.msi which is installed into a tools folder in `vcpkg_installed` as part of the build process.

- **vcpkg features** — The `"default-features": false` disables the default XInput backend so that only GameInput is used for input handling. This avoids linking both input systems. It also disables DirectX Tool Kit for Audio which we can add back later.

- **GraphicsMemory** — In DirectX 12, the application is responsible for managing the lifetime of all video memory resources. The `GraphicsMemory` class is a helper for managing allocations and lifetimes for an 'upload heap'. This is used for constant buffers, dynamic vertex & index buffers, and as a source for copying data to 'dedicated video memory' on the GPU. Lifetime is managed by 'fences' which are injected once per frame, in combination with reference counts. The `Commit` method must be called once-per-frame to ensure proper tracking and cleanup.

- **Shader Model 6** — DirectX Tool Kit for DirectX 12 requires Shader Model 6 support. The template includes a `CheckFeatureSupport` call to validate this at startup.
