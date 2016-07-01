//--------------------------------------------------------------------------------------
// File: WICTextureLoader.h
//
// Function for loading a WIC image and creating a Direct3D runtime texture for it
// (auto-generating mipmaps if possible)
//
// Note: Assumes application has already called CoInitializeEx
//
// Note these functions are useful for images created as simple 2D textures. For
// more complex resources, DDSTextureLoader is an excellent light-weight runtime loader.
// For a full-featured DDS file reader, writer, and texture processing pipeline see
// the 'Texconv' sample and the 'DirectXTex' library.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#pragma once

#if defined(_XBOX_ONE) && defined(_TITLE)
#include <d3d12_x.h>
#else
#include <d3d12.h>
#endif

#include <stdint.h>


namespace DirectX
{
    class ResourceUploadBatch;

    // Standard version
    HRESULT __cdecl CreateWICTextureFromMemory(
        _In_ ID3D12Device* d3dDevice,
        _In_ ResourceUploadBatch& resourceUpload,
        _In_reads_bytes_(wicDataSize) const uint8_t* wicData,
        _In_ size_t wicDataSize,
        _Outptr_ ID3D12Resource** texture,
        _In_ bool generateMips = true,
        _In_ size_t maxsize = 0);

    HRESULT __cdecl CreateWICTextureFromFile(
        _In_ ID3D12Device* d3dDevice,
        _In_ ResourceUploadBatch& resourceUpload,
        _In_z_ const wchar_t* szFileName,
        _Outptr_  ID3D12Resource** texture,
        _In_ bool generateMips = true,
        _In_ size_t maxsize = 0);

    // Extended version
    HRESULT __cdecl CreateWICTextureFromMemoryEx(
        _In_ ID3D12Device* d3dDevice,
        _In_ ResourceUploadBatch& resourceUpload,
        _In_reads_bytes_(wicDataSize) const uint8_t* wicData,
        _In_ size_t wicDataSize,
        _In_ size_t maxsize,
        _In_ D3D12_RESOURCE_FLAGS flags,
        _In_ bool forceSRGB,
        _In_ bool generateMips,
        _Outptr_  ID3D12Resource** texture);

    HRESULT __cdecl CreateWICTextureFromFileEx(
        _In_ ID3D12Device* d3dDevice,
        _In_ ResourceUploadBatch& resourceUpload,
        _In_z_ const wchar_t* szFileName,
        _In_ size_t maxsize,
        _In_ D3D12_RESOURCE_FLAGS flags,
        _In_ bool forceSRGB,
        _In_ bool generateMips,
        _Outptr_ ID3D12Resource** texture);
}