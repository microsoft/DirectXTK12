# Step 1: Project Setup

First, determine if the user needs a new project or already has one.

**Ask the user:** Do you want to create a new project, or do you already have an existing Direct3D 12 project?

## Creating a New Project

If the user wants a new project, use the `d3d12game_vcpkg` template from <https://github.com/walbourn/directx-vs-templates/>.

**Ask the user:**

1. What project name would you like? (Default: `Direct3DGame`)
2. Do you want an MSBuild (.vcxproj) project or a CMake project (CMake recommended)?
3. Where should the project be created? (Default: `$Env:USERPROFILE\source`)

Then download the templates from the latest GitHub release and run the appropriate script:

```powershell
# Download and extract the templates repo from the latest release
$release = (Invoke-RestMethod -Uri "https://api.github.com/repos/walbourn/directx-vs-templates/releases/latest").tag_name
$templatesZip = "$env:TEMP\directx-vs-templates.zip"
$templatesDir = "$env:TEMP\directx-vs-templates"
Invoke-WebRequest -Uri "https://github.com/walbourn/directx-vs-templates/archive/refs/tags/$release.zip" -OutFile $templatesZip
Expand-Archive -Path $templatesZip -DestinationPath $templatesDir -Force
$repoRoot = Get-ChildItem -Path $templatesDir -Directory | Select-Object -First 1
```

### MSBuild

```powershell
& "$($repoRoot.FullName)\VSIX\createmsbuild.ps1" d3d12game_vcpkg <ProjectName> <TargetDir>
```

Parameters:
- `TemplateDir`: Use `d3d12game_vcpkg` for the DirectX 12 game template with vcpkg integration.
- `ProjectName`: The project name (default: `Direct3DGame`). Used for the `.vcxproj` filename and solution.
- `TargetDir`: Output directory (default: `$Env:USERPROFILE\source`). A subdirectory named `<ProjectName>` is created here.
- `PlatformToolset`: VS platform toolset (default: `v143` for VS 2022, use `v144` for VS 2026).

### CMake

```powershell
& "$($repoRoot.FullName)\VSIX\createcmake.ps1" d3d12game_vcpkg <ProjectName> <TargetDir>
```

Parameters:
- `TemplateDir`: Use `d3d12game_vcpkg` for the DirectX 12 game template with vcpkg integration.
- `ProjectName`: The project name (default: `Direct3DGame`). Used for the CMake project name.
- `TargetDir`: Output directory (default: `$Env:USERPROFILE\source`). A subdirectory named `<ProjectName>` is created here.

### Cleanup

After the project is created, clean up the temp files:

```powershell
Remove-Item -Path $templatesZip -Force
Remove-Item -Path $templatesDir -Recurse -Force
```

### Note for git repositories

If the project is being created in a git repository, then the `.gitignore` file at the root of the repository should be updated to exclude the vcpkg installed files and build artifacts.

```plaintext
**/vcpkg_installed/
```

## Using an Existing Project

If the user already has a project, verify they have:

1. A Direct3D 12 device and command queue
2. A render loop with SwapChain and Present
3. A command list for rendering
4. DirectXTK12 integrated via vcpkg, NuGet, or project reference (see the **directxtk12-usage** skill for integration guidance)

## Technical Notes

**Ask the user:** Would you like to see the Technical Notes for this step, or skip to the next step?

The `d3d12game_vcpkg` template creates a minimal Direct3D 12 application with the following architecture:

### Overview

The basics of a Win32 Direct3D 12 application include:

1. **Win32 window creation** — A window is created using the Win32 API.
2. **Direct3D 12 device initialization** — The DXGI API is used to enumerate adapters, and then an ID3D12Device object is created which is used for creating resources. Unlike Direct3D 11, there is no immediate context — instead, the application creates command lists and command queues to submit work to the GPU.
3. **Render loop** — For a game application, a loop is run to update the game state and render the scene each frame. Unlike in traditional applications, the game loop is real-time and continuous rather than waiting for user input.
4. **Swap chain management** — A IDXGISwapChain object is used to manage the presentation of rendered frames. If the application rendering gets too far ahead of the display, it will wait until more frames are fully processed.

### Main.cpp — Application Entry Point

- **Win32 window creation** — Registers a `WNDCLASSEX`, creates an `HWND` with `CreateWindowExW`, and runs a standard `PeekMessage` loop. When no messages are pending, `Game::Tick()` is called to drive the game loop.
- **Window message handling** — The `WndProc` handles resize (`WM_SIZE`), minimize/restore (suspend/resume), ALT+ENTER fullscreen toggle, display changes, and power management.
- **Hybrid GPU preference** — Exports `NvOptimusEnablement` and `AmdPowerXpressRequestHighPerformance` to prefer discrete GPUs on hybrid systems.
- **COM initialization** — Calls `CoInitializeEx` (multithreaded) which is required for WIC texture loading later.

> For details on the fullscreen management, see [this blog post](https://walbourn.github.io/care-and-feeding-of-modern-swapchains/).

### DeviceResources — Direct3D 12 Device Wrapper

The `DeviceResources` class encapsulates all Direct3D 12 device and swap chain management:

- **`IDXGIFactory6`** — Created via `CreateDXGIFactory2`. Used to enumerate adapters, create the swap chain, and check feature support (tearing, HDR, flip model).
- **`ID3D12Device`** — Created via `D3D12CreateDevice` with a preferred hardware adapter (falls back to WARP in debug builds). The device is the factory for creating all GPU resources.
- **`ID3D12CommandQueue`** — The queue for submitting command lists to the GPU for execution.
- **`ID3D12CommandAllocator`** (per back buffer) — Backing memory for command list recording. One per frame in flight to avoid GPU/CPU synchronization stalls.
- **`ID3D12GraphicsCommandList`** — Records rendering commands (draw calls, resource barriers, state changes) for submission to the command queue.
- **`IDXGISwapChain3`** — Created via `IDXGIFactory2::CreateSwapChainForHwnd`. Uses `DXGI_SWAP_EFFECT_FLIP_DISCARD` for modern flip-model presentation.
- **`ID3D12Resource`** (render targets) — The swap chain's back buffer resources.
- **`ID3D12DescriptorHeap`** (RTV) — Render target view descriptor heap for the swap chain back buffers.
- **`ID3D12Resource`** (depth stencil) — Created with `D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL` for depth testing.
- **`ID3D12DescriptorHeap`** (DSV) — Depth stencil view descriptor heap.
- **`ID3D12Fence`** — Used for CPU/GPU synchronization. Ensures the CPU does not overwrite command allocators or resources still in use by the GPU.
- **Debug layer** — In debug builds, enables `D3D12GetDebugInterface` and configures `ID3D12InfoQueue` to break on errors/corruption.

> Unlike Direct3D 11, the application is responsible for all synchronization between CPU and GPU. The fence mechanism ensures safe reuse of per-frame resources.

> Since we are going to ultimately be rendering a 3D scene in the tutorial, we are using the default values for the DeviceResources ctor. If we were only rendering 2D, we would pass ``DXGI_FORMAT_UNKNOWN`` for the *depthBufferFormat* parameter.

### Game Loop

The template's game loop follows a fixed pattern:

1. `Game::Tick()` → calls `Update` (game logic with a step timer) then `Render`
2. `Render` → calls `DeviceResources::Prepare` (resets command allocator and command list, transitions back buffer to render target), performs `Clear` (sets render target and depth stencil views, clears them), draws, calls `DeviceResources::Present`
3. `Present` → transitions back buffer to present state, closes and executes command list, calls `IDXGISwapChain1::Present1` (with tearing flag if supported), advances fence value
4. Device lost handling — if `Present` returns `DXGI_ERROR_DEVICE_REMOVED`, calls `HandleDeviceLost` which recreates all resources and notifies the `Game` via `IDeviceNotify`

### Key Differences from Direct3D 11

| Aspect | Direct3D 11 | Direct3D 12 |
| --- | --- | --- |
| Command submission | Immediate context | Command lists + command queue |
| Resource state | Driver-managed | Application-managed (resource barriers) |
| Descriptor binding | Bind slots | Descriptor heaps + root signatures |
| Pipeline state | Individual state objects | Monolithic pipeline state objects (PSOs) |
| Synchronization | Driver-managed | Application-managed (fences) |
| Memory management | Driver-managed | Application-managed |
