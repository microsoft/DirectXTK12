//--------------------------------------------------------------------------------------
// File: DirectXHelpers.cpp
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

#include "pch.h"
#include "DirectXHelpers.h"
#include "GraphicsMemory.h"

using namespace DirectX;

_Use_decl_annotations_
void DirectX::CreateShaderResourceView(
    ID3D12Device* d3dDevice,
    ID3D12Resource* tex,
    D3D12_CPU_DESCRIPTOR_HANDLE srvDescriptor,
    bool isCubeMap)
{
    D3D12_RESOURCE_DESC desc = tex->GetDesc();
    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
    SRVDesc.Format = desc.Format;
    SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    switch (desc.Dimension)
    {
    case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
        if (desc.DepthOrArraySize > 1)
        {
            SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
            SRVDesc.Texture1DArray.MipLevels = (!desc.MipLevels) ? -1 : desc.MipLevels;
            SRVDesc.Texture1DArray.ArraySize = static_cast<UINT>(desc.DepthOrArraySize);
        }
        else
        {
            SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
            SRVDesc.Texture1D.MipLevels = (!desc.MipLevels) ? -1 : desc.MipLevels;
        }
        break;

    case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        if (isCubeMap)
        {
            if (desc.DepthOrArraySize > 6)
            {
                SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
                SRVDesc.TextureCubeArray.MipLevels = (!desc.MipLevels) ? -1 : desc.MipLevels;
                SRVDesc.TextureCubeArray.NumCubes = static_cast<UINT>(desc.DepthOrArraySize / 6);
            }
            else
            {
                SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
                SRVDesc.TextureCube.MipLevels = (!desc.MipLevels) ? -1 : desc.MipLevels;
            }
        }
        else if (desc.DepthOrArraySize > 1)
        {
            SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            SRVDesc.Texture2DArray.MipLevels = (!desc.MipLevels) ? -1 : desc.MipLevels;
            SRVDesc.Texture2DArray.ArraySize = static_cast<UINT>(desc.DepthOrArraySize);
        }
        else
        {
            SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            SRVDesc.Texture2D.MipLevels = (!desc.MipLevels) ? -1 : desc.MipLevels;
        }
        break;

    case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
        SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
        SRVDesc.Texture3D.MipLevels = (!desc.MipLevels) ? -1 : desc.MipLevels;
        break;

    case D3D12_RESOURCE_DIMENSION_BUFFER:
        throw std::exception("CreateShaderResourceView cannot be used with DIMENSION_BUFFER.");

    default:
        throw std::exception("CreateShaderResourceView cannot be used with DIMENSION_UNKNOWN.");
    }

    d3dDevice->CreateShaderResourceView(tex, &SRVDesc, srvDescriptor);
}


