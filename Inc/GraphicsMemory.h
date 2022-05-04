//--------------------------------------------------------------------------------------
// File: GraphicsMemory.h
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
#include <cstring>
#include <memory>


namespace DirectX
{
    class LinearAllocatorPage;

    // Works a little like a smart pointer. The memory will only be fenced by the GPU once the pointer
    // has been invalidated or the user explicitly marks it for fencing.
    class GraphicsResource
    {
    public:
        GraphicsResource() noexcept;
        GraphicsResource(
            _In_ LinearAllocatorPage* page,
            _In_ D3D12_GPU_VIRTUAL_ADDRESS gpuAddress,
            _In_ ID3D12Resource* resource,
            _In_ void* memory,
            _In_ size_t offset,
            _In_ size_t size) noexcept;

        GraphicsResource(GraphicsResource&& other) noexcept;
        GraphicsResource&& operator= (GraphicsResource&&) noexcept;

        GraphicsResource(const GraphicsResource&) = delete;
        GraphicsResource& operator= (const GraphicsResource&) = delete;

        ~GraphicsResource();

        D3D12_GPU_VIRTUAL_ADDRESS GpuAddress() const noexcept { return mGpuAddress; }
        ID3D12Resource* Resource() const noexcept { return mResource; }
        void* Memory() const noexcept { return mMemory; }
        size_t ResourceOffset() const noexcept { return mBufferOffset; }
        size_t Size() const noexcept { return mSize; }

        explicit operator bool() const noexcept { return mResource != nullptr; }

        // Clear the pointer. Using operator -> will produce bad results.
        void __cdecl Reset() noexcept;
        void __cdecl Reset(GraphicsResource&&) noexcept;

    private:
        LinearAllocatorPage*        mPage;
        D3D12_GPU_VIRTUAL_ADDRESS   mGpuAddress;
        ID3D12Resource*             mResource;
        void*                       mMemory;
        size_t                      mBufferOffset;
        size_t                      mSize;
    };

    class SharedGraphicsResource
    {
    public:
        SharedGraphicsResource() noexcept;

        SharedGraphicsResource(SharedGraphicsResource&&) noexcept;
        SharedGraphicsResource&& operator= (SharedGraphicsResource&&) noexcept;

        SharedGraphicsResource(GraphicsResource&&);
        SharedGraphicsResource&& operator= (GraphicsResource&&);

        SharedGraphicsResource(const SharedGraphicsResource&) noexcept;
        SharedGraphicsResource& operator= (const SharedGraphicsResource&) noexcept;

        SharedGraphicsResource(const GraphicsResource&) = delete;
        SharedGraphicsResource& operator= (const GraphicsResource&) = delete;

        ~SharedGraphicsResource();

        D3D12_GPU_VIRTUAL_ADDRESS GpuAddress() const noexcept { return mSharedResource->GpuAddress(); }
        ID3D12Resource* Resource() const noexcept { return mSharedResource->Resource(); }
        void* Memory() const noexcept { return mSharedResource->Memory(); }
        size_t ResourceOffset() const noexcept { return mSharedResource->ResourceOffset(); }
        size_t Size() const noexcept { return mSharedResource->Size(); }

        explicit operator bool() const noexcept { return mSharedResource != nullptr; }

        bool operator == (const SharedGraphicsResource& other) const noexcept { return mSharedResource.get() == other.mSharedResource.get(); }
        bool operator != (const SharedGraphicsResource& other) const noexcept { return mSharedResource.get() != other.mSharedResource.get(); }

        // Clear the pointer. Using operator -> will produce bad results.
        void __cdecl Reset() noexcept;
        void __cdecl Reset(GraphicsResource&&);
        void __cdecl Reset(SharedGraphicsResource&&) noexcept;
        void __cdecl Reset(const SharedGraphicsResource& resource) noexcept;

    private:
        std::shared_ptr<GraphicsResource> mSharedResource;
    };

    //----------------------------------------------------------------------------------
    struct GraphicsMemoryStatistics
    {
        size_t committedMemory;     // Bytes of memory currently committed/in-flight
        size_t totalMemory;         // Total bytes of memory used by the allocators
        size_t totalPages;          // Total page count
        size_t peakCommitedMemory;  // Peak commited memory value since last reset
        size_t peakTotalMemory;     // Peak total bytes
        size_t peakTotalPages;      // Peak total page count
    };

    //----------------------------------------------------------------------------------
    class GraphicsMemory
    {
    public:
        explicit GraphicsMemory(_In_ ID3D12Device* device);

        GraphicsMemory(GraphicsMemory&&) noexcept;
        GraphicsMemory& operator= (GraphicsMemory&&) noexcept;

        GraphicsMemory(GraphicsMemory const&) = delete;
        GraphicsMemory& operator=(GraphicsMemory const&) = delete;

        virtual ~GraphicsMemory();

        // Make sure to keep the GraphicsResource handle alive as long as you need to access
        // the memory on the CPU. For example, do not simply cache GpuAddress() and discard
        // the GraphicsResource object, or your memory may be overwritten later.
        GraphicsResource __cdecl Allocate(size_t size, size_t alignment = 16);

        // Special overload of Allocate that aligns to D3D12 constant buffer alignment requirements
        template<typename T> GraphicsResource AllocateConstant()
        {
            constexpr size_t alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
            constexpr size_t alignedSize = (sizeof(T) + alignment - 1) & ~(alignment - 1);
            return Allocate(alignedSize, alignment);
        }
        template<typename T> GraphicsResource AllocateConstant(const T& setData)
        {
            GraphicsResource alloc = AllocateConstant<T>();
            memcpy(alloc.Memory(), &setData, sizeof(T));
            return alloc;
        }

        // Submits all the pending one-shot memory to the GPU.
        // The memory will be recycled once the GPU is done with it.
        void __cdecl Commit(_In_ ID3D12CommandQueue* commandQueue);

        // This frees up any unused memory.
        // If you want to make sure all memory is reclaimed, idle the GPU before calling this.
        // It is not recommended that you call this unless absolutely necessary (e.g. your
        // memory budget changes at run-time, or perhaps you're changing levels in your game.)
        void __cdecl GarbageCollect();

        // Memory statistics
        GraphicsMemoryStatistics __cdecl GetStatistics();
        void __cdecl ResetStatistics();

        // Singleton
        // Should only use nullptr for single GPU scenarios; mGPU requires a specific device
        static GraphicsMemory& __cdecl Get(_In_opt_ ID3D12Device* device = nullptr);

    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;
    };
}
