//--------------------------------------------------------------------------------------
// File: DDSTextureLoader.h
//
// Functions for loading a DDS texture and creating a Direct3D runtime resource for it
//
// Note these functions are useful as a light-weight runtime loader for DDS files. For
// a full-featured DDS file reader, writer, and texture processing pipeline see
// the 'Texconv' sample and the 'DirectXTex' library.
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#pragma once

#ifdef _GAMING_XBOX_SCARLETT
#include <d3d12_xs.h>
#elif (defined(_XBOX_ONE) && defined(_TITLE)) || defined(_GAMING_XBOX)
#include <d3d12_x.h>
#elif defined(USING_DIRECTX_HEADERS)
#include <directx/d3d12.h>
#include <dxguids/dxguids.h>
#else
#include <d3d12.h>
#endif

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

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


namespace DirectX
{
    class ResourceUploadBatch;

#ifndef DDS_ALPHA_MODE_DEFINED
#define DDS_ALPHA_MODE_DEFINED
    enum DDS_ALPHA_MODE : uint32_t
    {
        DDS_ALPHA_MODE_UNKNOWN = 0,
        DDS_ALPHA_MODE_STRAIGHT = 1,
        DDS_ALPHA_MODE_PREMULTIPLIED = 2,
        DDS_ALPHA_MODE_OPAQUE = 3,
        DDS_ALPHA_MODE_CUSTOM = 4,
    };
#endif

    inline namespace DX12
    {
        enum DDS_LOADER_FLAGS : uint32_t
        {
            DDS_LOADER_DEFAULT = 0,
            DDS_LOADER_FORCE_SRGB = 0x1,
            DDS_LOADER_IGNORE_SRGB = 0x2,
            DDS_LOADER_MIP_AUTOGEN = 0x8,
            DDS_LOADER_MIP_RESERVE = 0x10,
            DDS_LOADER_IGNORE_MIPS = 0x20,
        };
    }

    // Standard version
    DIRECTX_TOOLKIT_API
        HRESULT __cdecl LoadDDSTextureFromMemory(
            _In_ ID3D12Device* device,
            _In_reads_bytes_(ddsDataSize) const uint8_t* ddsData,
            size_t ddsDataSize,
            _Outptr_ ID3D12Resource** texture,
            std::vector<D3D12_SUBRESOURCE_DATA>& subresources,
            size_t maxsize = 0,
            _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr,
            _Out_opt_ bool* isCubeMap = nullptr);

    DIRECTX_TOOLKIT_API
        HRESULT __cdecl LoadDDSTextureFromFile(
            _In_ ID3D12Device* device,
            _In_z_ const wchar_t* szFileName,
            _Outptr_ ID3D12Resource** texture,
            std::unique_ptr<uint8_t[]>& ddsData,
            std::vector<D3D12_SUBRESOURCE_DATA>& subresources,
            size_t maxsize = 0,
            _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr,
            _Out_opt_ bool* isCubeMap = nullptr);

        // Standard version with resource upload
    DIRECTX_TOOLKIT_API
        HRESULT __cdecl CreateDDSTextureFromMemory(
            _In_ ID3D12Device* device,
            ResourceUploadBatch& resourceUpload,
            _In_reads_bytes_(ddsDataSize) const uint8_t* ddsData,
            size_t ddsDataSize,
            _Outptr_ ID3D12Resource** texture,
            bool generateMipsIfMissing = false,
            size_t maxsize = 0,
            _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr,
            _Out_opt_ bool* isCubeMap = nullptr);

    DIRECTX_TOOLKIT_API
        HRESULT __cdecl CreateDDSTextureFromFile(
            _In_ ID3D12Device* device,
            ResourceUploadBatch& resourceUpload,
            _In_z_ const wchar_t* szFileName,
            _Outptr_ ID3D12Resource** texture,
            bool generateMipsIfMissing = false,
            size_t maxsize = 0,
            _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr,
            _Out_opt_ bool* isCubeMap = nullptr);

        // Extended version
    DIRECTX_TOOLKIT_API
        HRESULT __cdecl LoadDDSTextureFromMemoryEx(
            _In_ ID3D12Device* device,
            _In_reads_bytes_(ddsDataSize) const uint8_t* ddsData,
            size_t ddsDataSize,
            size_t maxsize,
            D3D12_RESOURCE_FLAGS resFlags,
            DDS_LOADER_FLAGS loadFlags,
            _Outptr_ ID3D12Resource** texture,
            std::vector<D3D12_SUBRESOURCE_DATA>& subresources,
            _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr,
            _Out_opt_ bool* isCubeMap = nullptr);

    DIRECTX_TOOLKIT_API
        HRESULT __cdecl LoadDDSTextureFromFileEx(
            _In_ ID3D12Device* device,
            _In_z_ const wchar_t* szFileName,
            size_t maxsize,
            D3D12_RESOURCE_FLAGS resFlags,
            DDS_LOADER_FLAGS loadFlags,
            _Outptr_ ID3D12Resource** texture,
            std::unique_ptr<uint8_t[]>& ddsData,
            std::vector<D3D12_SUBRESOURCE_DATA>& subresources,
            _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr,
            _Out_opt_ bool* isCubeMap = nullptr);

        // Extended version with resource upload
    DIRECTX_TOOLKIT_API
        HRESULT __cdecl CreateDDSTextureFromMemoryEx(
            _In_ ID3D12Device* device,
            ResourceUploadBatch& resourceUpload,
            _In_reads_bytes_(ddsDataSize) const uint8_t* ddsData,
            size_t ddsDataSize,
            size_t maxsize,
            D3D12_RESOURCE_FLAGS resFlags,
            DDS_LOADER_FLAGS loadFlags,
            _Outptr_ ID3D12Resource** texture,
            _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr,
            _Out_opt_ bool* isCubeMap = nullptr);

    DIRECTX_TOOLKIT_API
        HRESULT __cdecl CreateDDSTextureFromFileEx(
            _In_ ID3D12Device* device,
            ResourceUploadBatch& resourceUpload,
            _In_z_ const wchar_t* szFileName,
            size_t maxsize,
            D3D12_RESOURCE_FLAGS resFlags,
            DDS_LOADER_FLAGS loadFlags,
            _Outptr_ ID3D12Resource** texture,
            _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr,
            _Out_opt_ bool* isCubeMap = nullptr);

#ifdef __cpp_lib_byte
    DIRECTX_TOOLKIT_API
        inline HRESULT __cdecl LoadDDSTextureFromMemory(
            _In_ ID3D12Device* device,
            _In_reads_bytes_(ddsDataSize) const std::byte* ddsData,
            size_t ddsDataSize,
            _Outptr_ ID3D12Resource** texture,
            std::vector<D3D12_SUBRESOURCE_DATA>& subresources,
            size_t maxsize = 0,
            _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr,
            _Out_opt_ bool* isCubeMap = nullptr)
    {
        return LoadDDSTextureFromMemory(device, reinterpret_cast<const uint8_t*>(ddsData), ddsDataSize, texture, subresources, maxsize, alphaMode, isCubeMap);
    }

    DIRECTX_TOOLKIT_API
        inline HRESULT __cdecl CreateDDSTextureFromMemory(
            _In_ ID3D12Device* device,
            ResourceUploadBatch& resourceUpload,
            _In_reads_bytes_(ddsDataSize) const std::byte* ddsData,
            size_t ddsDataSize,
            _Outptr_ ID3D12Resource** texture,
            bool generateMipsIfMissing = false,
            size_t maxsize = 0,
            _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr,
            _Out_opt_ bool* isCubeMap = nullptr)
    {
        return CreateDDSTextureFromMemory(device, resourceUpload, reinterpret_cast<const uint8_t*>(ddsData), ddsDataSize, texture, generateMipsIfMissing, maxsize, alphaMode, isCubeMap);
    }

    DIRECTX_TOOLKIT_API
        inline HRESULT __cdecl LoadDDSTextureFromMemoryEx(
            _In_ ID3D12Device* device,
            _In_reads_bytes_(ddsDataSize) const std::byte* ddsData,
            size_t ddsDataSize,
            size_t maxsize,
            D3D12_RESOURCE_FLAGS resFlags,
            DDS_LOADER_FLAGS loadFlags,
            _Outptr_ ID3D12Resource** texture,
            std::vector<D3D12_SUBRESOURCE_DATA>& subresources,
            _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr,
            _Out_opt_ bool* isCubeMap = nullptr)
    {
        return LoadDDSTextureFromMemoryEx(device, reinterpret_cast<const uint8_t*>(ddsData), ddsDataSize, maxsize, resFlags, loadFlags, texture, subresources, alphaMode, isCubeMap);
    }

    DIRECTX_TOOLKIT_API
        inline HRESULT __cdecl CreateDDSTextureFromMemoryEx(
            _In_ ID3D12Device* device,
            ResourceUploadBatch& resourceUpload,
            _In_reads_bytes_(ddsDataSize) const std::byte* ddsData,
            size_t ddsDataSize,
            size_t maxsize,
            D3D12_RESOURCE_FLAGS resFlags,
            DDS_LOADER_FLAGS loadFlags,
            _Outptr_ ID3D12Resource** texture,
            _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr,
            _Out_opt_ bool* isCubeMap = nullptr)
    {
        return CreateDDSTextureFromMemoryEx(device, resourceUpload, reinterpret_cast<const uint8_t*>(ddsData), ddsDataSize, maxsize, resFlags, loadFlags, texture, alphaMode, isCubeMap);
    }
#endif //  __cpp_lib_byte

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-dynamic-exception-spec"
#endif

    inline namespace DX12
    {
        DEFINE_ENUM_FLAG_OPERATORS(DDS_LOADER_FLAGS)
    }

#ifdef __clang__
#pragma clang diagnostic pop
#endif
}
