//--------------------------------------------------------------------------------------
// File: GenericMultiBuffer.h
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

#include "DirectXHelpers.h"
#include "PlatformHelpers.h"


namespace DirectX
{
    class GenericMultiBuffer
    {
    public:
        GenericMultiBuffer(_In_ ID3D12Device* device, size_t numBytesPerFrame, size_t maxFrames)
            : mDataStride(numBytesPerFrame)
            , mNumFrames(maxFrames)
        {
            CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
            CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(numBytesPerFrame * maxFrames);

            // Create the constant buffer.
            ThrowIfFailed(device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &bufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(mResource.GetAddressOf())));

            ThrowIfFailed(mResource->Map(0, nullptr, reinterpret_cast<void**>( &mMappedData )));
            memset(mMappedData, 0, mDataStride * maxFrames);
        }

        // Get the CPU address for a given frame
        void* GetPointer(_In_ size_t frameIndex) const
        {
            assert(frameIndex < mNumFrames);
            assert(mMappedData != nullptr);
            return mMappedData + ( frameIndex * mDataStride );
        }

        // Get the GPU address for a given frame
        D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress(_In_ size_t frameIndex) const
        {
            assert(frameIndex < mNumFrames);
            D3D12_GPU_VIRTUAL_ADDRESS cbvGpuAddress = mResource->GetGPUVirtualAddress();
            cbvGpuAddress += mDataStride * frameIndex;
            return cbvGpuAddress;
        }

        // Returns the resource pointer
        ID3D12Resource* Resource() const { return mResource.Get(); }

    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> mResource;
        uint8_t* mMappedData;
        size_t mNumFrames;
        size_t mDataStride;
    };
}
