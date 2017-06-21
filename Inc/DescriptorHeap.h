//--------------------------------------------------------------------------------------
// File: DescriptorHeap.h
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
#include <d3dx12_x.h>
#else
#include <d3d12.h>
#include "d3dx12.h"
#endif

#include <stdexcept>
#include <assert.h>
#include <stdint.h>

#include <wrl/client.h>


namespace DirectX
{
    // A contiguous linear random-access descriptor heap
    class DescriptorHeap
    {
    public:
        DescriptorHeap(
            _In_ ID3D12DescriptorHeap* pExistingHeap);
        DescriptorHeap(
            _In_ ID3D12Device* device,
            _In_ const D3D12_DESCRIPTOR_HEAP_DESC* pDesc);
        DescriptorHeap(
            _In_ ID3D12Device* device,
            D3D12_DESCRIPTOR_HEAP_TYPE type,
            D3D12_DESCRIPTOR_HEAP_FLAGS flags,
            size_t count);

        DescriptorHeap(DescriptorHeap&&) = default;
        DescriptorHeap& operator=(DescriptorHeap&&) = default;

        DescriptorHeap(const DescriptorHeap&) = delete;
        DescriptorHeap& operator=(const DescriptorHeap&) = delete;

        D3D12_GPU_DESCRIPTOR_HANDLE __cdecl WriteDescriptors(
            _In_ ID3D12Device* device,
            uint32_t offsetIntoHeap,
            uint32_t totalDescriptorCount,
            _In_reads_(descriptorRangeCount) const D3D12_CPU_DESCRIPTOR_HANDLE* pDescriptorRangeStarts,
            _In_reads_(descriptorRangeCount) const uint32_t* pDescriptorRangeSizes,
            uint32_t descriptorRangeCount);

        D3D12_GPU_DESCRIPTOR_HANDLE __cdecl WriteDescriptors(
            _In_ ID3D12Device* device,
            uint32_t offsetIntoHeap,
            _In_reads_(descriptorRangeCount) const D3D12_CPU_DESCRIPTOR_HANDLE* pDescriptorRangeStarts,
            _In_reads_(descriptorRangeCount) const uint32_t* pDescriptorRangeSizes,
            uint32_t descriptorRangeCount);

        D3D12_GPU_DESCRIPTOR_HANDLE __cdecl WriteDescriptors(
            _In_ ID3D12Device* device,
            uint32_t offsetIntoHeap,
            _In_reads_(descriptorCount) const D3D12_CPU_DESCRIPTOR_HANDLE* pDescriptors,
            uint32_t descriptorCount);

        D3D12_GPU_DESCRIPTOR_HANDLE GetFirstGpuHandle() const
        {
            assert(m_desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
            assert(m_pHeap != 0);
            return m_hGPU;
        }

        D3D12_CPU_DESCRIPTOR_HANDLE GetFirstCpuHandle() const
        {
            assert(m_pHeap != 0);
            return m_hCPU;
        }

        D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(_In_ size_t index) const
        {
            assert(m_pHeap != 0);
            if (index >= m_desc.NumDescriptors)
            {
                throw std::out_of_range("D3DX12_GPU_DESCRIPTOR_HANDLE");
            }
            assert(m_desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
            return CD3DX12_GPU_DESCRIPTOR_HANDLE(m_hGPU, static_cast<INT>(index), m_increment);
        }

        D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(_In_ size_t index) const
        {
            assert(m_pHeap != 0);
            if (index >= m_desc.NumDescriptors)
            {
                throw std::out_of_range("D3DX12_CPU_DESCRIPTOR_HANDLE");
            }
            return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_hCPU, static_cast<INT>(index), m_increment);
        }

        size_t Count() const { return m_desc.NumDescriptors; }
        unsigned int Flags() const { return m_desc.Flags; }
        D3D12_DESCRIPTOR_HEAP_TYPE Type() const { return m_desc.Type; }
        size_t Increment() const { return m_increment; }
        ID3D12DescriptorHeap* Heap() const { return m_pHeap.Get(); }

        static void __cdecl DefaultDesc(
            _In_ D3D12_DESCRIPTOR_HEAP_TYPE type,
            _Out_ D3D12_DESCRIPTOR_HEAP_DESC* pDesc);

    private:
        void __cdecl Create(_In_ ID3D12Device* pDevice, _In_ const D3D12_DESCRIPTOR_HEAP_DESC* pDesc);

        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>    m_pHeap;
        D3D12_DESCRIPTOR_HEAP_DESC                      m_desc;
        CD3DX12_CPU_DESCRIPTOR_HANDLE                   m_hCPU;
        CD3DX12_GPU_DESCRIPTOR_HANDLE                   m_hGPU;
        uint32_t                                        m_increment;
    };


    // Helper class for dynamically allocating descriptor indices.
    // The pile is statically sized and will throw an exception if it becomes full.
    class DescriptorPile : public DescriptorHeap
    {
    public:
        using IndexType = size_t;
        static const IndexType INVALID_INDEX = size_t(-1);

        DescriptorPile(
            _In_ ID3D12DescriptorHeap* pExistingHeap,
            size_t reserve = 0)
            : DescriptorHeap(pExistingHeap),
            m_top(reserve)
        {
            if (reserve > 0 && m_top >= Count())
            {
                throw std::out_of_range("Reserve descriptor range is too large");
            }
        }

        DescriptorPile(
            _In_ ID3D12Device* device,
            _In_ const D3D12_DESCRIPTOR_HEAP_DESC* pDesc,
            size_t reserve = 0)
            : DescriptorHeap(device, pDesc),
            m_top(reserve)
        {
            if (reserve > 0 && m_top >= Count())
            {
                throw std::out_of_range("Reserve descriptor range is too large");
            }
        }

        DescriptorPile(
            _In_ ID3D12Device* device,
            D3D12_DESCRIPTOR_HEAP_TYPE type,
            D3D12_DESCRIPTOR_HEAP_FLAGS flags,
            size_t capacity,
            size_t reserve = 0)
            : DescriptorHeap(device, type, flags, capacity),
            m_top(reserve)
        {
            if (reserve > 0 && m_top >= Count())
            {
                throw std::out_of_range("Reserve descriptor range is too large");
            }
        }

        DescriptorPile(const DescriptorPile&) = delete;
        DescriptorPile& operator=(const DescriptorPile&) = delete;

        DescriptorPile(DescriptorPile&&) = default;
        DescriptorPile& operator=(DescriptorPile&&) = default;

        IndexType Allocate()
        {
            IndexType start, end;
            AllocateRange(1, start, end);

            return start;
        }

        void AllocateRange(size_t numDescriptors, _Out_ IndexType& start, _Out_ IndexType& end)
        {
            // make sure we didn't allocate zero
            if (numDescriptors == 0)
            {
                throw std::out_of_range("Can't allocate zero descriptors");
            }

            // get the current top
            start = m_top;

            // increment top with new request
            m_top += numDescriptors;
            end = m_top;

            // make sure we have enough room
            if (m_top >= Count())
            {
                throw std::exception("Can't allocate more descriptors");
            }
        }

    private:
        IndexType m_top;
    };
}
