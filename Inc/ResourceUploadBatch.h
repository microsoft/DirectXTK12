//--------------------------------------------------------------------------------------
// File: ResourceUploadBatch.h
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
#include <directx/dxgiformat.h>
#include <dxguids/dxguids.h>
#else
#include <d3d12.h>
#include <dxgiformat.h>
#endif

#include <cstdint>
#include <future>
#include <memory>

#include "GraphicsMemory.h"

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
    // Has a command list of it's own so it can upload at any time.
    class ResourceUploadBatch
    {
    public:
        DIRECTX_TOOLKIT_API explicit ResourceUploadBatch(_In_ ID3D12Device* device) noexcept(false);

        DIRECTX_TOOLKIT_API ResourceUploadBatch(ResourceUploadBatch&&) noexcept;
        DIRECTX_TOOLKIT_API ResourceUploadBatch& operator= (ResourceUploadBatch&&) noexcept;

        ResourceUploadBatch(ResourceUploadBatch const&) = delete;
        ResourceUploadBatch& operator= (ResourceUploadBatch const&) = delete;

        DIRECTX_TOOLKIT_API virtual ~ResourceUploadBatch();

        // Call this before your multiple calls to Upload.
        DIRECTX_TOOLKIT_API void __cdecl Begin(D3D12_COMMAND_LIST_TYPE commandType = D3D12_COMMAND_LIST_TYPE_DIRECT);

        // Asynchronously uploads a resource. The memory in subRes is copied.
        // The resource must be in the COPY_DEST state.
        DIRECTX_TOOLKIT_API void __cdecl Upload(
            _In_ ID3D12Resource* resource,
            uint32_t subresourceIndexStart,
            _In_reads_(numSubresources) const D3D12_SUBRESOURCE_DATA* subRes,
            uint32_t numSubresources);

        DIRECTX_TOOLKIT_API void __cdecl Upload(
            _In_ ID3D12Resource* resource,
            const SharedGraphicsResource& buffer);

        // Asynchronously generate mips from a resource.
        // Resource must be in the PIXEL_SHADER_RESOURCE state
        DIRECTX_TOOLKIT_API void __cdecl GenerateMips(_In_ ID3D12Resource* resource);

        // Transition a resource once you're done with it
        DIRECTX_TOOLKIT_API void __cdecl Transition(
            _In_ ID3D12Resource* resource,
            D3D12_RESOURCE_STATES stateBefore,
            D3D12_RESOURCE_STATES stateAfter);

        // Submits all the uploads to the driver.
        // No more uploads can happen after this call until Begin is called again.
        // This returns a handle to an event that can be waited on.
        DIRECTX_TOOLKIT_API std::future<void> __cdecl End(_In_ ID3D12CommandQueue* commandQueue);

        // Validates if the given DXGI format is supported for autogen mipmaps
        DIRECTX_TOOLKIT_API bool __cdecl IsSupportedForGenerateMips(DXGI_FORMAT format) noexcept;

    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;
    };
}
