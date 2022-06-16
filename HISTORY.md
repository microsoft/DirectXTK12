# DirectX Tool Kit for DirectX 12

http://go.microsoft.com/fwlink/?LinkID=615561

Release available for download on [GitHub](https://github.com/microsoft/DirectXTK12/releases)

## Release History

### June 15, 2022
* GamePad, Keyboard, and Mouse updated to use GameInput on PC for the Gaming.Desktop.x64 platform
* CMake project updates
* Minor code review

### May 9, 2022
* C++20 spaceship operator updates for SimpleMath
* Fixed missing VertexPositionNormal::InputLayout
* Minor updates for VS 2022 (17.2)
* CMake project updates (now supports MSVC, clang/LLVM, and MinGW)
* Updated D3DX12 internal copy with latest changes from DirectX-Headers GitHub
* Retired VS 2017 projects
* Minor code review
* Reformat source using updated .editorconfig settings

## March 24, 2022
* Fixed bug in UWP implementation of Mouse that combined vertical/horizontal scroll-wheel input
* Code refactoring for input classes (GamePad, Keyboard, and Mouse)
* Update build switches for SDL recommendations
* CMake project updates and UWP platform CMakePresets
* Fixed constexpr compat issue with DEFINE_ENUM_FLAG_OPERATORS in legacy Xbox One XDK
* Dropped support for legacy Xbox One XDK prior to April 2018

### February 28, 2022
* Minor fix to DescriptorHeap Increment() to return uint32_t instead of size_t
* Updated D3DX12 internal copy with latest changes from GitHub
* SimpleMath Matrix updated with ToEuler and Vector3 version of CreateFromYawPitchRoll methods
* SimpleMath Quaternion updated with ToEuler, RotateTowards, FromToRotation, LookRotation, and Angle methods
* Keyboard updated with new IME On/Off v-keys
* Win32 Mouse now uses ``WM_ACTIVATE`` for more robust behavior
* *DirectX Tool Kit for Audio* updated for Advanced Format (4Kn) wavebank streaming
* Code and project review including fixing clang v13 warnings
* Added CMakePresets.json

### November 8, 2021
* VS 2022 support
* Updated D3DX12 internal copy with latest change from GitHub
* Minor code and project review

### October 18, 2021
* Fixed loading of skinned PBR models from SDKMESH v2
* Minor code review updates

### October 13, 2021
* Added skinning support for **NormalMapEffect** and **PBREffect**
* Common states updated with support for reverse z-buffer rendering with **DepthReverseZ** and **DepthReadReverseZ** methods.
* Effect factory updates
  * Updated to use ``SkinnedNormalMapEffect`` / ``SkinnedPBREffect`` as appropriate.
  * PBR now supports 'untextured' models (always requires texture coordinates) with use of diffuse color for constant albedo, and specular power for an estimated constant roughness.
* Model loader updates
  * SDKMESH loader no longer requires precomputed vertex tangents for normal mapping as we don't use them.
  * Added ``ModelLoader_DisableSkinning`` flag when dealing with legacy SDKMESH files with too many skinning bone influences for _MaxBone_
* Fix for BGRA auto-generation of mipmaps on some hardware
* Minor update for the Mouse implementation for GameInput
* Project and code cleanup

### September 30, 2021
* Added ModelBone support for transformation hierarchies
  * Rigid-body & skinned animation Draw support added to Model
* Support for loading Visual Studio ``CMO`` models added using BasicEffect or SkinnedEffect materials
* Added type aliases ``Model::EffectCollection``, ``ModelMeshPart::InputLayoutCollection``, ``GeometricPrimitive::VertexCollection`` and ``IndexCollection``
* Fixed handle leak in ResourceUploadBatch
* Updated ScopeBarrier to conform with C++14 ``std::initializer_list``
* VS 2017 projects updated to require the Windows 10 SDK (19401) and make use of Shader Model 6.0
* Code review updates

### August 1, 2021
* DebugEffect, NormalMapEffect, and PBREffect updated with instancing support
* GeometricPrimitive updated with DrawInstanced method
* ToneMapPostProcess updated with SetColorRotation method
* Added VS 2022 Preview projects
* Minor code review

### June 9, 2021
* VS 2019 projects now use Shader Model 6 to build shaders (CMake has build option)
* DirectX Tool Kit for Audio updates:
  * Fixed mono source panning
  * Added ``EnableDefaultMultiChannel`` helper to AudioEmitter for multi-channel source setup
  * Added ``GetChannelCount`` accessor to SoundEffectInstance and SoundStreamInstance
  * ``Apply3D`` can now use X3DAUDIO_LISTENER and X3DAUDIO_EMITTER directly or the library helper structs.
* Minor code review

### April 6, 2021
* DDSTextureLoader updated to accept nVidia Texture Tool v1 single-channel and dual-channel files marked as RGB instead of LUMINANCE
* Fixed ScreenGrab for reserved and MSAA resources
* Minor code and project cleanup

### January 9, 2021
* Code review for improved conformance
* CMake updated to support package install

### November 11, 2020
* Fixed ``/analyze`` warnings in GameInput usage
* Updated D3DX12 internal copy with latest change from GitHub
* Sync'd DirectX Tool Kit for Audio with DX11 version
* Minor code and project cleanup

### September 30, 2020
* GamePad class updated with ``c_MostRecent`` constant for ``-1`` player index special behavior
  * For GameInput API implementation, also added ``c_MergedInput``
* Fixed bug in WICTextureLoader that resulted in ``WINCODEC_ERR_INSUFFICIENTBUFFER`` for some resize requests
* Fixed ``.wav`` file reading of MIDILoop chunk
* Minor code cleanup

### August 15, 2020
* *breaking change* Converted default bool parameters on some effects to ``EffectFlags``:
  * Added new effects flags ``Specular``, ``Emissive``, ``Fresnel``, and ``Velocity``
  * Removed EnvironmentMapEffect ``fresnelEnabled``, ``specularEnabled`` parameters
  * Removed NormalMapEffect ``specularMap`` parameter
  * Removed PBREffect ``emissive``, ``generateVelocity`` parameters
  * Removed SkinnedEffect ``weightsPerVertex`` parameter (always uses 4 bones)
* EnvironmentMapEffect now supports cubemaps, spherical, and dual-parabola environment maps
* Fixed bug with ScreenGrab with 'small alignment' textures
* Code review and project updates
* Added GDK projects

### July 2, 2020
* Improved SpriteFont drawing performance in Debug builds
* Regenerated shaders using Windows 10 May 2020 Update SDK (19041)
* Code cleanup for some new VC++ 16.7 warnings
* CMake updates

### June 15, 2020
* DescriptorHeap / DescriptorPile updated with additional ctor
* EffectTextureFactory ctor parameter updated with default value
* Code cleanup for some new VC++ 16.7 warnings and static code analysis

### June 1, 2020
* Added BufferHelpers header with functions **CreateStaticBuffer** and **CreateTextureFromMemory**
* Added **IsPowerOf2** helper to DirectXHelpers
* SpriteBatch now supports providing a new heap-based sampler on calls to **Begin**
* Converted to typed enum bitmask flags (see release notes for details on this potential *breaking change*)
  + ``AUDIO_ENGINE_FLAGS``, ``DDS_LOADER_FLAGS``, ``ModelLoaderFlags``, ``SOUND_EFFECT_INSTANCE_FLAGS``, and ``WIC_LOADER_FLAGS``
* WICTextureLoader for ``PNG`` codec now checks ``gAMA`` chunk to determine colorspace if the ``sRGB`` chunk is not found for legacy sRGB detection.
* ``WIC_LOADER_SRGB_DEFAULT`` flag added when loading image via WIC without explicit colorspace metadata
* CMake project updates

### May 10, 2020
* **ResourceUploadBatch** updated to support usage with copy & compute queues
* **Transition** methods added for GeometricPrimtive and Model for use with static VBs/IBs
* WICTextureLoader updated with new loader flags: ``FORCE_RGBA32``, ``FIT_POW2``, and ``MAKE_SQUARE``
* SimpleMath no longer forces use of d3d11.h or d3d12.h (can be used with d3d9.h for example)
* *DirectX Tool Kit for Audio* updated with **SoundStreamInstance** class for async I/O playback from XACT-style streaming wavebanks
* Code cleanup
* Updated D3DX12 internal copy to Windows 10 SDK (19041) version

### April 3, 2020
* Updated D3DX12 internal copy to latest version
* SpriteFont **MeasureString** / **MeasureDrawBounds** fixes for !ignoreWhitespace
* Regenerated shaders using Windows 10 SDK (19041)
  * Upgraded to use root signature 1.1 which requires Windows 10 (14393) or later
* Code review (``constexpr`` / ``noexcept`` usage)
* CMake updated for PCH usage with 3.16 or later

### February 24, 2020
* *breaking change* **Model::CreateFromxxx** parameter order changed and added ModelLoaderFlags
* Added ``ignoreWhitespace`` defaulted parameter to SpriteFont Measure methods
* Sync'd DirectX Tool Kit for Audio and GamePad with DX11 version
* Fixed encoding issue with Utilities.fxh
* Code and project cleanup
* Retired VS 2015 projects

### December 17, 2019
* Added ARM64 platform to VS 2019 Win32 desktop Win10 project
* Added Vector ``operator/`` by float scalar to SimpleMath
* Added **GetStatistics** method to GraphicsMemory
* Reduced fence object usage in GraphicsMemory's **LinearAllocator**
* Updated CMake project
* Code cleaup

### October 17, 2019
* Added optional ``forceSRGB`` parameter to SaveWICTextureToFile
* GamePad updated to report VID/PID (when supported)
* Minor code cleanup

### August 21, 2019
* Updated D3DX12 internal copy to latest version
* Code cleanup

### June 30, 2019
* Clang/LLVM warning cleanup
* Renamed ``DirectXTK_Windows10.vcxproj`` to ``_Windows10_2017.vcxproj``
* Added VS 2019 UWP project

### May 30, 2019
* PBREffect updated with additional set methods
* Additional debugging output for GraphicsMemory in error cases
* Added CMake project files
* Code cleanup

### April 26, 2019
* Updated auto-generated mipmaps support to make it more robust
* Added optional **LoadStaticBuffers** method for GeometricPrimitive
* Added VS 2019 desktop projects
* Fixed guards w.r.t. to windows.h usage in Keyboard/Mouse headers
* Added C++/WinRT SetWindow helper to Keyboard/Mouse
* Update HLSL script to use Shader Model 5.1 instead of 5.0
* Code cleanup

### February 7, 2019
* Model now supports loading ``SDKMESH v2`` models
* **PBREffectFactory** added to support PBR materials
* PBREffect and NormalMapEffect shaders updated to support ``BC5_UNORM`` compressed normal maps
* SpriteFont: **DrawString** overloads for UTF-8 chars in addition to UTF-16LE wide chars
* Fixed bug with GraphicsMemory dtor introduced with mGPU handling
* Made library agnostic to legacy Windows SDK pix.h vs. latest pix3.h from NuGet

### November 16, 2018
* VS 2017 updated for Windows 10 October 2018 Update SDK (17763)
* ARM64 platform configurations added to UWP projects
* Minor code review

### October 31, 2018
* Model loader for SDKMESH now attempts to use legacy DE3CN compressed normals
  + This is an approximation only and emits a warning in debug builds
* IEffectTextureFactory's CreateTexture interface method now returns the 'slot'
  + This is for use with **GetResource** method
* Minor code review

### October 25, 2018
* Use UTF-8 instead of ANSI for narrow strings
* Updated D3DX12 internal copy to latest version
* Improved debug diagnostics
* Minor code review

### September 13, 2018
* Broke DescriptorHeap header dependency on D3DX12.H

### August 17, 2018
* Improved validation for 16k textures and other large resources
* Improved debug output for failed texture loads and screengrabs
* Updated for VS 2017 15.8
* Code cleanup

### July 3, 2018
* Model **LoadStaticBuffers** method to use static vs. dynamic VB/IB
* *breaking change* Custom Model loaders and renderers should be updated for changes to ModelMeshPart
* ModelMeshPart **DrawInstanced** method added
* Code and project cleanup

### May 31, 2018
* VS 2017 updated for Windows 10 April 2018 Update SDK (17134)
* Regenerated shaders using Windows 10 April 2018 Update SDK (17134)

### May 14, 2018
* EffectPipelineStateDescription updated with **GetDesc** method
* Updated for VS 2017 15.7 update warnings
* Code and project cleanup

### April 23, 2018
* **AlignUp**, **AlignDown** template functions in DirectXHelpers.h
* **ScopedBarrier** added to DirectXHelpers.h
* Mouse support for cursor visibility
* SimpleMath and VertexTypes updated with default copy and move ctors
* SimpleMath updates to use constexpr
* Basic multi-GPU support added
* More debug object naming for PIX
* PostProcess updated with 'big triangle' optimization
* Code and project file cleanup

### February 7, 2018
* Mouse fix for cursor behavior when using Remote Desktop for Win32
* Updated for a few more VS 2017 warnings

### December 13, 2017
* **PBREffect** and **DebugEffect** added
* **NormalMapEffect** no longer requires or uses explicit vertex tangents
* Updated for VS 2017 15.5 update warnings
* Code cleanup

### November 1, 2017
* VS 2017 updated for Windows 10 Fall Creators Update SDK (16299)
* Regenerated shaders using Windows 10 Fall Creators Update SDK (16299)
* Updated D3DX12 internal copy to latest version

### September 22, 2017
* Updated for VS 2017 15.3 update ``/permissive-`` changes
* ScreenGrab updated to use non-sRGB metadata for PNG
* Mouse use of ``WM_INPUT`` updated for Remote Desktop scenarios

### July 28, 2017
* Fix for WIC writer when codec target format requires a palette
* Fix for error detection in ResourceUploadBatch::End method
* Code cleanup

### June 21, 2017
* Post-processing support with the **BasicPostProcess**, **DualPostProcess**, and **ToneMapPostProcess** classes
* Added **DescriptorPile** utility class
* SDKMESH loader fix when loading legacy files with all zero materials
* DirectXTK for Audio: Minor fixes for environmental audio
* Optimized root signatures for Effects shaders
* Minor code cleanup

### April 24, 2017
* Regenerated shaders using Windows 10 Creators Update SDK (15063)
* Fixed NormalMapEffect shader selection for specular texture usage
* Fixed Direct3D validation layer issues when using Creators Update
* Fixed AudioEngine enumeration when using Single Threaded Apartment (STA)
* Fixed bug with GamePad (``Windows.Gaming.Input``) when no user bound
* Updated D3DX12 internal copy to latest version

### April 7, 2017
* VS 2017 updated for Windows Creators Update SDK (15063)
* XboxDDSTextureLoader updates

### February 10, 2017
* SpriteBatch default rasterizer state now matches DirectX 11 version
* DDSTextureLoader now supports loading planar video format textures
* **GamePad** now supports special value of -1 for 'most recently connected controller'
* WIC format 40bppCMYKAlpha should be converted to RGBA8 rather than RGBA16
* DDS support for L8A8 with bitcount 8 rather than 16
* Updated D3DX12 internal copy to latest version
* Minor code cleanup

### December 5, 2016
* Mouse and Keyboard classes updated with **IsConnected** method
* Windows10 project ``/ZW`` switch removed to support use in C++/WinRT projection apps
* VS 2017 RC projects added
* Updated D3DX12 internal copy to latest version
* Minor code cleanup

### October 6, 2016
* SDKMESH loader and BasicEffects support for compressed vertex normals with biasing
* *breaking change*
  + DDSTextureLoader Ex bool ``forceSRGB`` and ``generateMipsIfMissing`` parmeters are now a ``DDS_LOADER`` flag
  + WICTextureLoader Ex bool ``forceSRGB`` and ``generateMips`` parameters are now a ``WIC_LOADER`` flag
  + Add ``vertexCount`` member to ModelMeshPart
* Minor code cleanup

### September 15, 2016
* Rebuild shaders using 1.0 Root Signature for improved compatibility
* Minor code cleanup

### September 1, 2016
* **EffectPipelineStateDescription** is now in it's own header
* Additional debug object naming
* Fixed Tier 1 hardware support issues with BasicEffect and generating mipmaps
* Fixed default graphics memory alignment to resolve rendering problems on some hardware
* Added ``forceSRGB`` optional parameter to SpriteFont ctor
* EffectFactory method **EnableForceSRGB** added
* Removed problematic ABI::Windows::Foundation::Rect interop for SimpleMath
* Updated D3DX12 internal copy for the Windows 10 Anniversary Update SDK (14393)
* Minor code cleanup

### August 4, 2016
* GraphicsMemory fix for robustness during cleanup
* Regenerated shaders using Windows 10 Anniversary Update SDK (14393)

### August 2, 2016
* Updated for VS 2015 Update 3 and Windows 10 SDK (14393)

### August 1, 2016
* Model effects array is now indexed by part rather than by material
* GamePad capabilities information updated for Universal Windows and Xbox One platforms
* Specular falloff lighting computation fix in shaders

### July 18, 2016
* *breaking changes* to CommonStates, DescriptorHeap, Effects, Model, EffectPipelineStateDescription, and SpriteBatchPipelineStateDescription
  + added texture sampler control to Effects and SpriteBatch
  + fixed Model control of blend and rasterizer state
  + fixed problems with PerPixelLighting control (EffectFactory defaults to per-pixel lighting)
  + fixed control of weights-per-vertex optimization for SkinnedEffect
  + removed unnecesary "one-light" shader permutations
  + fixed bug in AlphaTestEfect implementation
  + improved debug messages for misconfigured effects NormalMapEffect for normal-map with optional specular map rendering
* **EnvironmentMapEffect** now supports per-pixel lighting
* Effects updated with **SetMatrices** and **SetColorAndAlpha** methods
* GraphicsMemory support for SharedGraphicsResource shared_ptr style smart-pointer
* PrimitiveBatch fix for DrawQuad
* ScreenGrab handles resource state transition
* SimpleMath: improved interop with DirectXMath constants
* WICTextureLoader module LoadWICTexture* methods
* Fixed bugs with GenerateMips for sRGB and BGRA formats
* Code cleanup

### June 30, 2016
* Original release
