//--------------------------------------------------------------------------------------
// File: BufferHelpers.h
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

#ifndef DIRECTX_TOOLKIT_API
#ifdef DIRECTX_TOOLKIT_EXPORT
#define DIRECTX_TOOLKIT_API __declspec(dllexport)
#elif defined(DIRECTX_TOOLKIT_IMPORT)
#define DIRECTX_TOOLKIT_API __declspec(dllimport)
#else
#define DIRECTX_TOOLKIT_API
#endif
#endif


namespace DirectX
{
    class ResourceUploadBatch;

    // Helpers for creating initialized Direct3D buffer resources.
    DIRECTX_TOOLKIT_API
    HRESULT __cdecl CreateStaticBuffer(
        _In_ ID3D12Device* device,
        ResourceUploadBatch& resourceUpload,
        _In_reads_bytes_(count* stride) const void* ptr,
        size_t count,
        size_t stride,
        D3D12_RESOURCE_STATES afterState,
        _COM_Outptr_ ID3D12Resource** pBuffer,
        D3D12_RESOURCE_FLAGS resFlags = D3D12_RESOURCE_FLAG_NONE) noexcept;

    template<typename T>
    HRESULT CreateStaticBuffer(
        _In_ ID3D12Device* device,
        ResourceUploadBatch& resourceUpload,
        _In_reads_(count) T const* data,
        size_t count,
        D3D12_RESOURCE_STATES afterState,
        _COM_Outptr_ ID3D12Resource** pBuffer,
        D3D12_RESOURCE_FLAGS resFlags = D3D12_RESOURCE_FLAG_NONE) noexcept
    {
        return CreateStaticBuffer(device, resourceUpload, data, count, sizeof(T), afterState, pBuffer, resFlags);
    }

    template<typename T>
    HRESULT CreateStaticBuffer(
        _In_ ID3D12Device* device,
        ResourceUploadBatch& resourceUpload,
        T const& data,
        D3D12_RESOURCE_STATES afterState,
        _COM_Outptr_ ID3D12Resource** pBuffer,
        D3D12_RESOURCE_FLAGS resFlags = D3D12_RESOURCE_FLAG_NONE) noexcept
    {
        return CreateStaticBuffer(device, resourceUpload, data.data(), data.size(), sizeof(typename T::value_type),
            afterState, pBuffer, resFlags);
    }

    DIRECTX_TOOLKIT_API
    HRESULT __cdecl CreateUAVBuffer(
        _In_ ID3D12Device* device,
        uint64_t bufferSize,
        _COM_Outptr_ ID3D12Resource** pBuffer,
        D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_FLAGS additionalResFlags = D3D12_RESOURCE_FLAG_NONE) noexcept;

    DIRECTX_TOOLKIT_API
    HRESULT __cdecl CreateUploadBuffer(
        _In_ ID3D12Device* device,
        _In_reads_bytes_opt_(count* stride) const void* ptr,
        size_t count,
        size_t stride,
        _COM_Outptr_ ID3D12Resource** pBuffer,
        D3D12_RESOURCE_FLAGS resFlags = D3D12_RESOURCE_FLAG_NONE) noexcept;

    template<typename T>
    HRESULT CreateUploadBuffer(
        _In_ ID3D12Device* device,
        _In_reads_(count) T const* data,
        size_t count,
        _COM_Outptr_ ID3D12Resource** pBuffer,
        D3D12_RESOURCE_FLAGS resFlags = D3D12_RESOURCE_FLAG_NONE) noexcept
    {
        return CreateUploadBuffer(device, data, count, sizeof(T), pBuffer, resFlags);
    }

    template<typename T>
    HRESULT CreateUploadBuffer(
        _In_ ID3D12Device* device,
        T const& data,
        _COM_Outptr_ ID3D12Resource** pBuffer,
        D3D12_RESOURCE_FLAGS resFlags = D3D12_RESOURCE_FLAG_NONE) noexcept
    {
        return CreateUploadBuffer(device, data.data(), data.size(), sizeof(typename T::value_type),
            pBuffer, resFlags);
    }

    // Helpers for creating texture from memory arrays.
    DIRECTX_TOOLKIT_API
    HRESULT __cdecl CreateTextureFromMemory(
        _In_ ID3D12Device* device,
        ResourceUploadBatch& resourceUpload,
        size_t width,
        DXGI_FORMAT format,
        const D3D12_SUBRESOURCE_DATA& initData,
        _COM_Outptr_ ID3D12Resource** texture,
        D3D12_RESOURCE_STATES afterState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_FLAGS resFlags = D3D12_RESOURCE_FLAG_NONE) noexcept;

    DIRECTX_TOOLKIT_API
    HRESULT __cdecl CreateTextureFromMemory(
        _In_ ID3D12Device* device,
        ResourceUploadBatch& resourceUpload,
        size_t width, size_t height,
        DXGI_FORMAT format,
        const D3D12_SUBRESOURCE_DATA& initData,
        _COM_Outptr_ ID3D12Resource** texture,
        bool generateMips = false,
        D3D12_RESOURCE_STATES afterState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_FLAGS resFlags = D3D12_RESOURCE_FLAG_NONE) noexcept;

    DIRECTX_TOOLKIT_API
    HRESULT __cdecl CreateTextureFromMemory(
        _In_ ID3D12Device* device,
        ResourceUploadBatch& resourceUpload,
        size_t width, size_t height, size_t depth,
        DXGI_FORMAT format,
        const D3D12_SUBRESOURCE_DATA& initData,
        _COM_Outptr_ ID3D12Resource** texture,
        D3D12_RESOURCE_STATES afterState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_FLAGS resFlags = D3D12_RESOURCE_FLAG_NONE) noexcept;
}
