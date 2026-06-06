# Step 0: Prerequisites

Before starting the tutorial, verify the user has the necessary tools and environment set up.

**Ask the user:** Have you already installed the tools below, or would you like help checking?

## Required Software

| Tool | Minimum Version | Purpose |
| ------ | ----------------- | --------- |
| **Visual Studio 2022** or **Visual Studio 2026** | 17.4+ / any | IDE and C++ compiler |
| **Desktop development with C++** workload | — | Includes MSVC toolset, Windows SDK, and CMake tools |
| **VCPKG component** (`Microsoft.VisualStudio.Component.Vcpkg`) | — | Package manager integration for dependencies |
| **Windows SDK** | 10.0.22000.0+ (Windows 11) | DirectX 12 headers and libraries |
| **Git** | any | Clone templates and manage source control |

> Visual Studio Community edition is sufficient. The **VCPKG component** is included in the "Desktop development with C++" workload by default in VS 2022 17.7+ and VS 2026.

## Hardware Requirements

- A **Direct3D 12-capable GPU** (Feature Level 11.0 or higher) is required.
- All modern dedicated GPUs from NVIDIA, AMD, and Intel support Direct3D 12.
- Alternatively, the **WARP software adapter** can be used for development without dedicated GPU hardware (debug builds use this as a fallback).

## Optional but Recommended

| Tool | Purpose |
| ------ | --------- |
| **CMake** (3.21+) | Required only if creating a CMake project instead of MSBuild |
| **Windows Terminal** | Better terminal experience for PowerShell commands |
| **PIX on Windows** | GPU debugging and profiling for Direct3D 12 |

## Verification Steps

Run the following checks in a PowerShell terminal to confirm the environment is ready:

### Visual Studio

```powershell
# Verify a VS 2022 or later install with the C++ Desktop workload
& "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -version "[17.0," -requires Microsoft.VisualStudio.Workload.NativeDesktop -property installationPath
```

### Windows SDK

```powershell
# Check for a Windows 10 SDK (10.0.19041.0 or later)
Get-ChildItem "HKLM:\SOFTWARE\Microsoft\Windows Kits\Installed Roots" -ErrorAction SilentlyContinue |
    Get-ItemProperty | Select-Object -ExpandProperty KitsRoot10 -ErrorAction SilentlyContinue
```

### VCPKG

```powershell
# Verify the vcpkg component is present in the VS install
$vsPath = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -version "[17.0," -requires Microsoft.VisualStudio.Component.Vcpkg -property installationPath | Select-Object -First 1
Test-Path "$vsPath\VC\vcpkg\vcpkg.exe"
```

### Git

```powershell
git --version
```

### Direct3D 12 Support

```powershell
# Check for a D3D12-capable adapter using dxdiag (quick check)
dxdiag /t "$env:TEMP\dxdiag.txt"; Start-Sleep -Seconds 3
Select-String -Path "$env:TEMP\dxdiag.txt" -Pattern "Feature Levels:.*12"
Remove-Item "$env:TEMP\dxdiag.txt" -ErrorAction SilentlyContinue
```

## Troubleshooting

- **Missing VCPKG component** — Open the Visual Studio Installer, click **Modify**, and ensure the "Desktop development with C++" workload is checked. In the Individual Components tab, confirm `Microsoft.VisualStudio.Component.Vcpkg` is selected.
- **Windows SDK not found** — Install a Windows 10 SDK (10.0.19041.0 or later) through the Visual Studio Installer under Individual Components.
- **Git not found** — Install Git from <https://git-scm.com/> or via `winget install Git.Git`.
- **No D3D12-capable GPU** — Ensure your GPU drivers are up to date. For development without a capable GPU, builds will fall back to the WARP software adapter in debug mode.

## Technical Notes

**Ask the user:** Would you like to see the Technical Notes for this step, or skip to the next step?

### Why vcpkg?

The tutorial uses vcpkg in **manifest mode** to manage DirectX Tool Kit for DX12 as a dependency. This approach:

1. **Pins dependency versions** via `vcpkg.json` and `vcpkg-configuration.json`, ensuring reproducible builds.
2. **Avoids manual library setup** — no need to download, build, or configure include/library paths yourself.
3. **Supports multiple configurations** — vcpkg handles Debug/Release and x64/ARM64 builds through triplets.

### DirectX 12 and Windows

Direct3D 12 is available on Windows 10 and Windows 11 (all editions). The Windows SDK provides the `d3d12.h`, `dxgi1_4.h`, and related headers. No separate DirectX SDK download is required — the legacy standalone DirectX SDK (June 2010) is not needed for this tutorial.

### Direct3D 12 vs. Direct3D 11

Direct3D 12 is a lower-level API compared to Direct3D 11. It provides more explicit control over GPU resources, memory management, and synchronization. The DirectX Tool Kit for DX12 provides a higher-level abstraction that makes it easier to use Direct3D 12, handling descriptor heaps, resource barriers, and command list management for you.

> For more background on the modern Windows SDK vs. the legacy DirectX SDK, see [this blog post](https://walbourn.github.io/where-is-the-directx-sdk-2024-edition/).
