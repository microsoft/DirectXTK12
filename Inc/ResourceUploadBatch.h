//--------------------------------------------------------------------------------------
// File: ResourceUploadBatch.h
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

#pragma warning(push)
#pragma warning(disable : 4619 5038)
#include <future>
#pragma warning(pop)

#include <memory>


namespace DirectX
{
    // Has a command list of it's own so it can upload at any time.
    class ResourceUploadBatch
    {
    public:
        explicit ResourceUploadBatch(_In_ ID3D12Device* device);
        ResourceUploadBatch(ResourceUploadBatch&& moveFrom);
        ResourceUploadBatch& operator= (ResourceUploadBatch&& moveFrom);

        ResourceUploadBatch(ResourceUploadBatch const&) = delete;
        ResourceUploadBatch& operator= (ResourceUploadBatch const&) = delete;

        virtual ~ResourceUploadBatch();

        // Call this before your multiple calls to Upload.
        void __cdecl Begin();

        // Asynchronously uploads a resource. The memory in subRes is copied.
        // The resource must be in the COPY_DEST state.
        void __cdecl Upload(
            _In_ ID3D12Resource* resource,
            _In_ uint32_t subresourceIndexStart,
            _In_reads_( numSubresources ) D3D12_SUBRESOURCE_DATA* subRes,
            _In_ uint32_t numSubresources);

        // Asynchronously generate mips from a resource.
        // Resource must be in the PIXEL_SHADER_RESOURCE state
        void __cdecl GenerateMips(_In_ ID3D12Resource* resource);

        // Transition a resource once you're done with it
        void __cdecl Transition(
            _In_ ID3D12Resource* resource,
            _In_ D3D12_RESOURCE_STATES stateBefore,
            _In_ D3D12_RESOURCE_STATES stateAfter);

        // Submits all the uploads to the driver.
        // No more uploads can happen after this call until Begin is called again.
        // This returns a handle to an event that can be waited on.
        std::future<void> __cdecl End(
            _In_ ID3D12CommandQueue* commandQueue );

    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;
    };
}

