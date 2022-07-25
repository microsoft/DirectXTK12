//--------------------------------------------------------------------------------------
// File: EffectPipelineStateDescription.h
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
#include <cstring>

#include "RenderTargetState.h"


namespace DirectX
{
    // Pipeline state information for creating effects.
    struct EffectPipelineStateDescription
    {
        EffectPipelineStateDescription(
            _In_opt_ const D3D12_INPUT_LAYOUT_DESC* iinputLayout,
            const D3D12_BLEND_DESC& blend,
            const D3D12_DEPTH_STENCIL_DESC& depthStencil,
            const D3D12_RASTERIZER_DESC& rasterizer,
            const RenderTargetState& renderTarget,
            D3D12_PRIMITIVE_TOPOLOGY_TYPE iprimitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
            D3D12_INDEX_BUFFER_STRIP_CUT_VALUE istripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED) noexcept
            :
            inputLayout{},
            blendDesc(blend),
            depthStencilDesc(depthStencil),
            rasterizerDesc(rasterizer),
            renderTargetState(renderTarget),
            primitiveTopology(iprimitiveTopology),
            stripCutValue(istripCutValue)
        {
            if (iinputLayout)
                this->inputLayout = *iinputLayout;
        }

        EffectPipelineStateDescription(const EffectPipelineStateDescription&) = default;
        EffectPipelineStateDescription& operator=(const EffectPipelineStateDescription&) = default;

        EffectPipelineStateDescription(EffectPipelineStateDescription&&) = default;
        EffectPipelineStateDescription& operator=(EffectPipelineStateDescription&&) = default;

        void CreatePipelineState(
            _In_ ID3D12Device* device,
            _In_ ID3D12RootSignature* rootSignature,
            const D3D12_SHADER_BYTECODE& vertexShader,
            const D3D12_SHADER_BYTECODE& pixelShader,
            _Outptr_ ID3D12PipelineState** pPipelineState) const;

    #if defined(_MSC_VER) || !defined(_WIN32)
        D3D12_GRAPHICS_PIPELINE_STATE_DESC GetDesc() const noexcept
        {
            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
            psoDesc.BlendState = blendDesc;
            psoDesc.SampleMask = renderTargetState.sampleMask;
            psoDesc.RasterizerState = rasterizerDesc;
            psoDesc.DepthStencilState = depthStencilDesc;
            psoDesc.InputLayout = inputLayout;
            psoDesc.IBStripCutValue = stripCutValue;
            psoDesc.PrimitiveTopologyType = primitiveTopology;
            psoDesc.NumRenderTargets = renderTargetState.numRenderTargets;
            memcpy(psoDesc.RTVFormats, renderTargetState.rtvFormats, sizeof(DXGI_FORMAT) * D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT);
            psoDesc.DSVFormat = renderTargetState.dsvFormat;
            psoDesc.SampleDesc = renderTargetState.sampleDesc;
            psoDesc.NodeMask = renderTargetState.nodeMask;
            return psoDesc;
        }
    #else
        D3D12_GRAPHICS_PIPELINE_STATE_DESC* GetDesc(_Out_ D3D12_GRAPHICS_PIPELINE_STATE_DESC* psoDesc) const noexcept
        {
            if (!psoDesc)
                return nullptr;

            *psoDesc = {};
            psoDesc->BlendState = blendDesc;
            psoDesc->SampleMask = renderTargetState.sampleMask;
            psoDesc->RasterizerState = rasterizerDesc;
            psoDesc->DepthStencilState = depthStencilDesc;
            psoDesc->InputLayout = inputLayout;
            psoDesc->IBStripCutValue = stripCutValue;
            psoDesc->PrimitiveTopologyType = primitiveTopology;
            psoDesc->NumRenderTargets = renderTargetState.numRenderTargets;
            memcpy(psoDesc->RTVFormats, renderTargetState.rtvFormats, sizeof(DXGI_FORMAT) * D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT);
            psoDesc->DSVFormat = renderTargetState.dsvFormat;
            psoDesc->SampleDesc = renderTargetState.sampleDesc;
            psoDesc->NodeMask = renderTargetState.nodeMask;
            return psoDesc;
        }
    #endif

        uint32_t ComputeHash() const noexcept;

        D3D12_INPUT_LAYOUT_DESC             inputLayout;
        D3D12_BLEND_DESC                    blendDesc;
        D3D12_DEPTH_STENCIL_DESC            depthStencilDesc;
        D3D12_RASTERIZER_DESC               rasterizerDesc;
        RenderTargetState                   renderTargetState;
        D3D12_PRIMITIVE_TOPOLOGY_TYPE       primitiveTopology;
        D3D12_INDEX_BUFFER_STRIP_CUT_VALUE  stripCutValue;
    };
}
