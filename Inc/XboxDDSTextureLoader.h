//--------------------------------------------------------------------------------------
// File: XboxDDSTextureLoader.h
//
// Functions for loading a DDS texture using the XBOX extended header and creating a
// Direct3D12.X runtime resource for it via the CreatePlacedResourceX API
//
// Note these functions will not load standard DDS files. Use the DDSTextureLoader
// module in the DirectXTex package or as part of the DirectXTK library to load
// these files which use standard Direct3D resource creation APIs.
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#pragma once

#if !(defined(_XBOX_ONE) && defined(_TITLE)) && !defined(_GAMING_XBOX)
#error This module only supports Xbox exclusive apps
#endif

#ifdef _GAMING_XBOX_SCARLETT
#include <d3d12_xs.h>
#else
#include <d3d12_x.h>
#endif

#ifdef _GAMING_XBOX
#pragma comment(lib,"xmem.lib")
#endif

#include <cstddef>
#include <cstdint>

#ifndef DDS_ALPHA_MODE_DEFINED
#define DDS_ALPHA_MODE_DEFINED
namespace DirectX
{
    enum DDS_ALPHA_MODE : uint32_t
    {
        DDS_ALPHA_MODE_UNKNOWN = 0,
        DDS_ALPHA_MODE_STRAIGHT = 1,
        DDS_ALPHA_MODE_PREMULTIPLIED = 2,
        DDS_ALPHA_MODE_OPAQUE = 3,
        DDS_ALPHA_MODE_CUSTOM = 4,
    };
}
#endif

#ifndef DIRECTX_TOOLKIT_API
#ifdef DIRECTX_TOOLKIT_EXPORT
#ifdef __GNUC__
#define DIRECTX_TOOLKIT_API __attribute__ ((dllexport))
#else
#define DIRECTX_TOOLKIT_API __declspec(dllexport)
#endif
#elif defined(DIRECTX_TOOLKIT_IMPORT)
#ifdef __GNUC__
#define DIRECTX_TOOLKIT_API __attribute__ ((dllimport))
#else
#define DIRECTX_TOOLKIT_API __declspec(dllimport)
#endif
#else
#define DIRECTX_TOOLKIT_API
#endif
#endif


namespace Xbox
{
    using DirectX::DDS_ALPHA_MODE;

    //
    //  NOTE: Flush the GPU caches before using textures created
    //  with these functions.
    //
    //  The simplest means of doing this is:
    //
    //      // Load all your textures:
    //      CreateDDSTextureFrom...
    //      CreateDDSTextureFrom...
    //      CreateDDSTextureFrom...
    //
    //      // Flush the GPU caches
    //      ID3D12CommandList::FlushPipelineX(D3D12XBOX_FLUSH_IDLE, 0, 0);
    //
    //      // Now it's safe to use the textures
    //      ... Draw ...
    //
    //  You may wish to consider more fine-grained flushes if
    //  creating textures at run-time. See the documentation for
    //  FlushPipelineX.
    //

    DIRECTX_TOOLKIT_API
    HRESULT __cdecl CreateDDSTextureFromMemory(
        _In_ ID3D12Device* device,
        _In_reads_bytes_(ddsDataSize) const uint8_t* ddsData,
        _In_ size_t ddsDataSize,
        _Outptr_opt_ ID3D12Resource** texture,
        _Outptr_ void** grfxMemory,
        _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr,
        _In_ bool forceSRGB = false,
        _Out_opt_ bool* isCubeMap = nullptr) noexcept;

    DIRECTX_TOOLKIT_API
    HRESULT __cdecl CreateDDSTextureFromFile(
        _In_ ID3D12Device* device,
        _In_z_ const wchar_t* szFileName,
        _Outptr_opt_ ID3D12Resource** texture,
        _Outptr_ void** grfxMemory,
        _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr,
        _In_ bool forceSRGB = false,
        _Out_opt_ bool* isCubeMap = nullptr) noexcept;

    DIRECTX_TOOLKIT_API void FreeDDSTextureMemory(_In_opt_ void* grfxMemory) noexcept;

#ifdef __cpp_lib_byte
    DIRECTX_TOOLKIT_API
    inline HRESULT __cdecl CreateDDSTextureFromMemory(
        _In_ ID3D12Device* device,
        _In_reads_bytes_(ddsDataSize) const std::byte* ddsData,
        _In_ size_t ddsDataSize,
        _Outptr_opt_ ID3D12Resource** texture,
        _Outptr_ void** grfxMemory,
        _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr,
        _In_ bool forceSRGB = false,
        _Out_opt_ bool* isCubeMap = nullptr) noexcept
    {
        return CreateDDSTextureFromMemory(device, reinterpret_cast<const uint8_t*>(ddsData), ddsDataSize, texture, grfxMemory, alphaMode, forceSRGB, isCubeMap);
    }
#endif //  __cpp_lib_byte
}
