//--------------------------------------------------------------------------------------
// File: DirectXHelpers.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "DirectXHelpers.h"
#include "GraphicsMemory.h"
#include "PlatformHelpers.h"

using namespace DirectX;

_Use_decl_annotations_
void DirectX::CreateShaderResourceView(
    ID3D12Device* device,
    ID3D12Resource* tex,
    D3D12_CPU_DESCRIPTOR_HANDLE srvDescriptor,
    bool isCubeMap)
{
#if defined(_MSC_VER) || !defined(_WIN32)
    const auto desc = tex->GetDesc();
#else
    D3D12_RESOURCE_DESC tmpDesc;
    const auto& desc = *tex->GetDesc(&tmpDesc);
#endif

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = desc.Format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    switch (desc.Dimension)
    {
    case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
        if (desc.DepthOrArraySize > 1)
        {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
            srvDesc.Texture1DArray.MipLevels = (!desc.MipLevels) ? UINT(-1) : desc.MipLevels;
            srvDesc.Texture1DArray.ArraySize = static_cast<UINT>(desc.DepthOrArraySize);
        }
        else
        {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
            srvDesc.Texture1D.MipLevels = (!desc.MipLevels) ? UINT(-1) : desc.MipLevels;
        }
        break;

    case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        if (isCubeMap)
        {
            if (desc.DepthOrArraySize > 6)
            {
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
                srvDesc.TextureCubeArray.MipLevels = (!desc.MipLevels) ? UINT(-1) : desc.MipLevels;
                srvDesc.TextureCubeArray.NumCubes = static_cast<UINT>(desc.DepthOrArraySize / 6);
            }
            else
            {
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
                srvDesc.TextureCube.MipLevels = (!desc.MipLevels) ? UINT(-1) : desc.MipLevels;
            }
        }
        else if (desc.DepthOrArraySize > 1)
        {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            srvDesc.Texture2DArray.MipLevels = (!desc.MipLevels) ? UINT(-1) : desc.MipLevels;
            srvDesc.Texture2DArray.ArraySize = static_cast<UINT>(desc.DepthOrArraySize);
        }
        else
        {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = (!desc.MipLevels) ? UINT(-1) : desc.MipLevels;
        }
        break;

    case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
        srvDesc.Texture3D.MipLevels = (!desc.MipLevels) ? UINT(-1) : desc.MipLevels;
        break;

    case D3D12_RESOURCE_DIMENSION_BUFFER:
        DebugTrace("ERROR: CreateShaderResourceView cannot be used with DIMENSION_BUFFER.\n");
        throw std::invalid_argument("buffer resources not supported");

    case D3D12_RESOURCE_DIMENSION_UNKNOWN:
    default:
        DebugTrace("ERROR: CreateShaderResourceView cannot be used with DIMENSION_UNKNOWN (%d).\n", desc.Dimension);
        throw std::invalid_argument("unknown resource dimension");
    }

    device->CreateShaderResourceView(tex, &srvDesc, srvDescriptor);
}

_Use_decl_annotations_
void DirectX::CreateUnorderedAccessView(
    ID3D12Device* device,
    ID3D12Resource* tex,
    D3D12_CPU_DESCRIPTOR_HANDLE uavDescriptor,
    UINT mipLevel)
{
    const auto desc = tex->GetDesc();

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = desc.Format;

    switch (desc.Dimension)
    {
    case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
        if (desc.DepthOrArraySize > 1)
        {
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
            uavDesc.Texture1DArray.MipSlice = mipLevel;
            uavDesc.Texture1DArray.FirstArraySlice = 0;
            uavDesc.Texture1DArray.ArraySize = desc.DepthOrArraySize;
        }
        else
        {
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
            uavDesc.Texture1D.MipSlice = mipLevel;
        }
        break;

    case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        if (desc.DepthOrArraySize > 1)
        {
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
            uavDesc.Texture2DArray.MipSlice = mipLevel;
            uavDesc.Texture2DArray.ArraySize = desc.DepthOrArraySize;
        }
        else
        {
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            uavDesc.Texture2D.MipSlice = mipLevel;
        }
        break;

    case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
        uavDesc.Texture3D.MipSlice = mipLevel;
        uavDesc.Texture3D.WSize = desc.DepthOrArraySize;
        break;

    case D3D12_RESOURCE_DIMENSION_BUFFER:
        DebugTrace("ERROR: CreateUnorderedResourceView cannot be used with DIMENSION_BUFFER.\n\tUse CreateBufferUnorderedAccessView.");
        throw std::exception("buffer resources not supported");

    case D3D12_RESOURCE_DIMENSION_UNKNOWN:
    default:
        DebugTrace("ERROR: CreateUnorderedResourceView cannot be used with DIMENSION_UNKNOWN (%d).\n", desc.Dimension);
        throw std::exception("unknown resource dimension");

    }
    device->CreateUnorderedAccessView(tex, nullptr, &uavDesc, uavDescriptor);
}

_Use_decl_annotations_
void DirectX::CreateRenderTargetView(
    ID3D12Device* device,
    ID3D12Resource* tex,
    D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor,
    UINT mipLevel)
{
    const auto desc = tex->GetDesc();

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = desc.Format;

    switch (desc.Dimension)
    {
    case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
        if (desc.DepthOrArraySize > 1)
        {
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
            rtvDesc.Texture1DArray.MipSlice = mipLevel;
            rtvDesc.Texture1DArray.FirstArraySlice = 0;
            rtvDesc.Texture1DArray.ArraySize = desc.DepthOrArraySize;
        }
        else
        {
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
            rtvDesc.Texture1D.MipSlice = mipLevel;
        }
        break;

    case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        if (desc.DepthOrArraySize > 1)
        {
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            rtvDesc.Texture2DArray.MipSlice = mipLevel;
            rtvDesc.Texture2DArray.ArraySize = desc.DepthOrArraySize;
        }
        else
        {
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            rtvDesc.Texture2D.MipSlice = mipLevel;
        }
        break;

    case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
        rtvDesc.Texture3D.MipSlice = mipLevel;
        rtvDesc.Texture3D.WSize = desc.DepthOrArraySize;
        break;

    case D3D12_RESOURCE_DIMENSION_BUFFER:
        DebugTrace("ERROR: CreateRenderTargetView cannot be used with DIMENSION_BUFFER.\n");
        throw std::exception("buffer resources not supported");

    case D3D12_RESOURCE_DIMENSION_UNKNOWN:
    default:
        DebugTrace("ERROR: CreateRenderTargetView cannot be used with DIMENSION_UNKNOWN (%d).\n", desc.Dimension);
        throw std::exception("unknown resource dimension");

    }
    device->CreateRenderTargetView(tex, &rtvDesc, rtvDescriptor);
}

_Use_decl_annotations_
void DirectX::CreateBufferShaderResourceView(
    ID3D12Device* device,
    ID3D12Resource* buffer,
    D3D12_CPU_DESCRIPTOR_HANDLE srvDescriptor,
    UINT stride)
{
    const auto desc = buffer->GetDesc();
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = desc.Format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = static_cast<UINT>((stride > 0) ? (desc.Width / stride) : desc.Width);
    srvDesc.Buffer.StructureByteStride = stride;

    device->CreateShaderResourceView(buffer, &srvDesc, srvDescriptor);
}

_Use_decl_annotations_
void DirectX::CreateBufferUnorderedAccessView(
    ID3D12Device* device,
    ID3D12Resource* buffer,
    D3D12_CPU_DESCRIPTOR_HANDLE uavDescriptor,
    UINT stride,
    D3D12_BUFFER_UAV_FLAGS flag,
    UINT counterOffset,
    ID3D12Resource* counterResource)
{
    const auto desc = buffer->GetDesc();

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = desc.Format;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = static_cast<UINT>(stride > 0 ? desc.Width / stride : desc.Width);
    uavDesc.Buffer.StructureByteStride = stride;
    uavDesc.Buffer.CounterOffsetInBytes = counterOffset;
    uavDesc.Buffer.Flags = flag;

    device->CreateUnorderedAccessView(buffer, counterResource, &uavDesc, uavDescriptor);
}
