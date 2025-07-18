![DirectX Logo](https://raw.githubusercontent.com/wiki/Microsoft/DirectXTK12/X_jpg.jpg)

# DirectX Tool Kit for DirectX 12

http://go.microsoft.com/fwlink/?LinkID=615561

Copyright (c) Microsoft Corporation.

## July 9, 2025

This package contains the "DirectX Tool Kit", a collection of helper classes for writing Direct3D 12 C++ code for Win32 desktop applications for Windows 11 and Windows 10, game titles for Xbox Series X\|S and Xbox One, and Universal Windows Platform (UWP) apps for Windows 11 and Windows 10.

This code is designed to build with Visual Studio 2019 (16.11), Visual Studio 2022, clang for Windows v12 or later, or MinGW 12.2. Use of the Windows 10 May 2020 Update SDK ([19041](https://walbourn.github.io/windows-10-may-2020-update-sdk/)) or later is required for Visual Studio.

These components are designed to work without requiring any content from the legacy DirectX SDK. For details, see [Where is the DirectX SDK?](https://aka.ms/dxsdk).

## Directory Layout

* ``Inc\``

  * Public Header Files (in the DirectX C++ namespace):

    * Audio.h - low-level audio API using XAudio2 (DirectXTK for Audio public header)
    * BufferHelpers.h - C++ helpers for creating D3D resources from CPU data
    * CommonStates.h - common D3D state combinations
    * DDSTextureLoader.h - light-weight DDS file texture loader
    * DescriptorHeap.h - helper for managing DX12 descriptor heaps
    * DirectXHelpers.h - misc C++ helpers for D3D programming
    * EffectPipelineStateDescription.h - helper for creating PSOs
    * Effects.h - set of built-in shaders for common rendering tasks
    * GamePad.h - gamepad controller helper using Windows.Gaming.Input or GameInput
    * GeometricPrimitive.h - draws basic shapes such as cubes and spheres
    * GraphicsMemory.h - helper for managing dynamic graphics memory allocation
    * Keyboard.h - keyboard state tracking helper
    * Model.h - draws meshes loaded from .CMO, .SDKMESH, or .VBO files
    * Mouse.h - mouse helper
    * PostProcess.h - set of built-in shaders for common post-processing operations
    * PrimitiveBatch.h - simple and efficient way to draw user primitives
    * RenderTargetState.h - helper for communicating render target requirements when creating PSOs
    * ResourceUploadBatch.h - helper for managing texture resource upload to the GPU
    * ScreenGrab.h - light-weight screen shot saver
    * SimpleMath.h - simplified C++ wrapper for DirectXMath
    * SpriteBatch.h - simple & efficient 2D sprite rendering
    * SpriteFont.h - bitmap based text rendering
    * VertexTypes.h - structures for commonly used vertex data formats
    * WICTextureLoader.h - WIC-based image file texture loader
    * XboxDDSTextureLoader.h - Xbox exclusive apps variant of DDSTextureLoader

* ``Src\``

  * DirectXTK12 source files and internal implementation headers

* ``Audio\``

  * DirectXTK12 for Audio source files and internal implementation headers

* ``build\``

  * Contains miscellaneous build files and scripts.

> MakeSpriteFont and XWBTool can be found in the [DirectX Tool Kit for DirectX 11](https://github.com/microsoft/DirectXTK). Audio, GamePad, Keyboard, Mouse, and SimpleMath are identical between both versions of the toolkit.

## Documentation

Documentation is available on the [GitHub wiki](https://github.com/Microsoft/DirectXTK12/wiki).

## Notices

All content and source code for this package are subject to the terms of the [MIT License](https://github.com/microsoft/DirectXTK12/blob/main/LICENSE).

For the latest version of DirectXTK12, bug reports, etc. please visit the project site on [GitHub](https://github.com/microsoft/DirectXTK12).

## Comparisons to DirectX 11 Version

* No support for Visual Studio Directed Graph Shader Language (DGSL) effect shaders (i.e. _DGSLEffect_). CMO files are loaded using BasicEffect or SkinnedEffect materials.

* VertexTypes does not include VertexPositionNormalTangentColorTexture or VertexPositionNormalTangentColorTextureSkinning which were intended for use with the DGSL pipeline.

* DirectX Tool Kit for DirectX 11 supports Feature Level 9.x, while DirectX 12 requires Direct3D Feature Level 11.0. There are no expected DirectX 12 drivers for any lower feature level devices.

* The library assumes it is building for Windows 10 (aka ``_WIN32_WINNT=0x0A00``) so it makes use of XAudio 2.9 and WIC2 as well as DirectX 12.

* DirectX Tool Kit for Audio, GamePad, Keyboard, Mouse, and SimpleMath are identical to the DirectX 11 version.

## Release Notes

FOR SECURITY ADVISORIES, see [GitHub](https://github.com/microsoft/DirectXTK12/security/advisories).

For a full change history, see [CHANGELOG.md](https://github.com/microsoft/DirectXTK12/blob/main/CHANGELOG.md).

* In the June 2024 release, the defaulted parameter `initialState` for the ``CreateUploadBuffer`` function in _BufferHelpers_ was removed. Per the DirectX 12 validation layer, the only valid initial state for an upload buffer is ``D3D12_RESOURCE_STATE_GENERIC_READ``.

* Starting with the February 2023 release, the Mouse class implementation of relative mouse movement was updated to accumulate changes between calls to ``GetState``. By default, each time you call ``GetState`` the deltas are reset which works for scenarios where you use relative movement but only call the method once per frame. If you call it more than once per frame, then add an explicit call to ``EndOfInputFrame`` to use an explicit reset model instead.

* As of the September 2022 release, the library makes use of C++11 inline namespaces for differing types that have the same names in the DirectX 11 and DirectX 12 version of the _DirectX Tool Kit_. This provides a link-unique name such as ``DirectX::DX12::SpriteBatch`` that will appear in linker output messages. In most use cases, however, there is no need to add explicit ``DX12`` namespace resolution in client code.

* Starting with the June 2021 release, this library builds the HLSL shaders with Shader Model 6 via DXC. See [this wiki page](https://github.com/microsoft/DirectXTK12/wiki/Shader-Model-6) for more information. The Microsoft GDK projects have always used Shader Model 6.

* Starting with the June 2020 release, this library makes use of [typed enum bitmask flags](https://walbourn.github.io/modern-c++-bitmask-types/) per the recommendation of the _C++ Standard_ section _17.5.2.1.3 Bitmask types_. This may have _breaking change impacts to client code:

  * You cannot pass the ``0`` literal as your flags value. Instead you must make use of the appropriate default enum value: ``AudioEngine_Default``, ``SoundEffectInstance_Default``, ``ModelLoader_Clockwise``, ``DDS_LOADER_DEFAULT``, or ``WIC_LOADER_DEFAULT``.

  * Use the enum type instead of ``DWORD`` if building up flags values locally with bitmask operations. For example, ```WIC_LOADER_FLAGS flags = WIC_LOADER_DEFAULT; if (...) flags |= WIC_LOADER_FORCE_SRGB;```

* The UWP projects and the Win10 classic desktop project include configurations for the ARM64 platform. Building these requires installing the ARM64 toolset.

* For ARM64/AArch64 development, the VS 2022 compiler is strongly recommended over the VS 2019 toolset. The Windows SDK (26100 or later) is not compatible with VS 2019 for Win32 on ARM64 development. _Note that the ARM32/AArch32 platform is [deprecated](https://learn.microsoft.com/windows/arm/arm32-to-arm64)_.

* When using clang/LLVM for the ARM64/AArch64 platform, the Windows 11 SDK ([22000](https://walbourn.github.io/windows-sdk-for-windows-11/)) or later is required.

* The ``CompileShaders.cmd`` script must have Windows-style (CRLF) line-endings. If it is changed to Linux-style (LF) line-endings, it can fail to build all the required shaders.

* Support for targeting Xbox One using the legacy Xbox One XDK was retired in March 2023. See the February 2023 or earlier releases of _DirectX Tool Kit_ for the required MSBuild project files. It can also be built using the current CMake projects from an _Xbox One XDK Developer Command Prompt_ with the addition of CMake 3.20 or later to the path via the `x64-Debug-Durango` or `x64-Release-Durango` CMake preset.

## Support

For questions, consider using [Stack Overflow](https://stackoverflow.com/questions/tagged/directxtk) with the _directxtk_ tag, or the [DirectX Discord Server](https://discord.gg/directx) in the _dx12-developers_ channel.

For bug reports and feature requests, please use GitHub [issues](https://github.com/microsoft/DirectXTK12/issues) for this project.

## Contributing

This project welcomes contributions and suggestions. Most contributions require you to agree to a Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us the rights to use your contribution. For details, visit https://cla.opensource.microsoft.com.

When you submit a pull request, a CLA bot will automatically determine whether you need to provide a CLA and decorate the PR appropriately (e.g., status check, comment). Simply follow the instructions provided by the bot. You will only need to do this once across all repos using our CLA.

Tests for new features should also be submitted as a PR to the [Test Suite](https://github.com/walbourn/directxtk12test/wiki) repository.

Information on coding-style can be found [here](https://github.com/microsoft/DirectXTK12/wiki/Implementation).

## Code of Conduct

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more informatsion see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

## Trademarks

This project may contain trademarks or logos for projects, products, or services. Authorized use of Microsoft trademarks or logos is subject to and must follow [Microsoft's Trademark & Brand Guidelines](https://www.microsoft.com/en-us/legal/intellectualproperty/trademarks/usage/general). Use of Microsoft trademarks or logos in modified versions of this project must not cause confusion or imply Microsoft sponsorship. Any use of third-party trademarks or logos are subject to those third-party's policies.

## Credits

The _DirectX Tool Kit for DirectX 11_ is the work of Shawn Hargreaves and Chuck Walbourn, with contributions from Aaron Rodriguez Hernandez and Dani Roman.

The _DirectX Tool Kit for DirectX 12_ is the work of Pete Lewis, Justin Saunders, and Chuck Walbourn based heavily on the DirectX Tool Kit for DirectX 11.

Thanks to Shanon Drone for the SDKMESH file format.

Thanks to Adrian Tsai for the geodesic sphere implementation.

Thanks to Garrett Serack for his help in creating the NuGet packages for DirectX Tool Kit.

Thanks to Pete Lewis and Justin Saunders for the normal-mapped and PBR shaders implementation.

Thanks for Travis Johnson for the mGPU support.

Thanks to Roberto Sonnino for his help with the CMO format and the VS Starter Kit animation.

Thanks to Richie Meyer for their contribution of Xbox PIX custom memory and type allocation tracking events support.

Thanks to Andrew Farrier and Scott Matloff for their on-going help with code reviews.
