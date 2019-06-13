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
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#pragma once

#if !defined(_XBOX_ONE) || !defined(_TITLE)
#error This module only supports Xbox One exclusive apps
#endif

#include <d3d12_x.h>

#include <stdint.h>

namespace Xbox
{
    enum DDS_ALPHA_MODE
    {
        DDS_ALPHA_MODE_UNKNOWN       = 0,
        DDS_ALPHA_MODE_STRAIGHT      = 1,
        DDS_ALPHA_MODE_PREMULTIPLIED = 2,
        DDS_ALPHA_MODE_OPAQUE        = 3,
        DDS_ALPHA_MODE_CUSTOM        = 4,
    };

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

    HRESULT __cdecl CreateDDSTextureFromMemory(
        _In_ ID3D12Device* d3dDevice,
        _In_reads_bytes_(ddsDataSize) const uint8_t* ddsData,
        _In_ size_t ddsDataSize,
        _Outptr_opt_ ID3D12Resource** texture,
        _Outptr_ void** grfxMemory,
        _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr, 
        _In_ bool forceSRGB = false,
        _Out_opt_ bool* isCubeMap = nullptr);

    HRESULT __cdecl CreateDDSTextureFromFile( 
        _In_ ID3D12Device* d3dDevice,
        _In_z_ const wchar_t* szFileName,
        _Outptr_opt_ ID3D12Resource** texture,
        _Outptr_ void** grfxMemory,
        _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr,
        _In_ bool forceSRGB = false,
        _Out_opt_ bool* isCubeMap = nullptr);

    void FreeDDSTextureMemory(_In_opt_ void* grfxMemory);
}
