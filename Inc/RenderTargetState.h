//--------------------------------------------------------------------------------------
// File: RenderTargetState.h
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
#else
#include <d3d12.h>
#include <dxgi1_4.h>
#endif

#include <cstdint>


namespace DirectX
{
    // Encapsulates all render target state when creating pipeline state objects
    class RenderTargetState
    {
    public:
        RenderTargetState() noexcept
            : sampleMask(~0U)
            , numRenderTargets(0)
            , rtvFormats{}
            , dsvFormat(DXGI_FORMAT_UNKNOWN)
            , sampleDesc{}
            , nodeMask(0)
        {
        }

        RenderTargetState(const RenderTargetState&) = default;
        RenderTargetState& operator=(const RenderTargetState&) = default;

        RenderTargetState(RenderTargetState&&) = default;
        RenderTargetState& operator=(RenderTargetState&&) = default;

        // Single render target convenience constructor
        RenderTargetState(
            _In_ DXGI_FORMAT rtFormat,
            _In_ DXGI_FORMAT dsFormat) noexcept
            : sampleMask(UINT_MAX)
            , numRenderTargets(1)
            , rtvFormats{}
            , dsvFormat(dsFormat)
            , sampleDesc{}
            , nodeMask(0)
        {
            sampleDesc.Count = 1;
            rtvFormats[0] = rtFormat;
        }

        // Convenience constructors converting from DXGI_SWAPCHAIN_DESC
#if defined(__dxgi_h__) || defined(__d3d11_x_h__) || defined(__d3d12_x_h__) || defined(__XBOX_D3D12_X__)
        RenderTargetState(
            _In_ const DXGI_SWAP_CHAIN_DESC* desc,
            _In_ DXGI_FORMAT dsFormat) noexcept
            : sampleMask(UINT_MAX)
            , numRenderTargets(1)
            , rtvFormats{}
            , dsvFormat(dsFormat)
            , sampleDesc{}
            , nodeMask(0)
        {
            rtvFormats[0] = desc->BufferDesc.Format;
            sampleDesc = desc->SampleDesc;
        }
#endif

#if defined(__dxgi1_2_h__) || defined(__d3d11_x_h__) || defined(__d3d12_x_h__) || defined(__XBOX_D3D12_X__)
        RenderTargetState(
            _In_ const DXGI_SWAP_CHAIN_DESC1* desc,
            _In_ DXGI_FORMAT dsFormat) noexcept
            : sampleMask(UINT_MAX)
            , numRenderTargets(1)
            , rtvFormats{}
            , dsvFormat(dsFormat)
            , sampleDesc{}
            , nodeMask(0)
        {
            rtvFormats[0] = desc->Format;
            sampleDesc = desc->SampleDesc;
        }
#endif

        uint32_t            sampleMask;
        uint32_t            numRenderTargets;
        DXGI_FORMAT         rtvFormats[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
        DXGI_FORMAT         dsvFormat;
        DXGI_SAMPLE_DESC    sampleDesc;
        uint32_t            nodeMask;
    };
}
