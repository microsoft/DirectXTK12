//--------------------------------------------------------------------------------------
// File: EffectPipelineStateDescription.h
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

#include <stdint.h>

#include "RenderTargetState.h"


namespace DirectX
{
    // Pipeline state information for creating effects.
    struct EffectPipelineStateDescription
    {
        EffectPipelineStateDescription(
            _In_opt_ const D3D12_INPUT_LAYOUT_DESC* inputLayout,
            const D3D12_BLEND_DESC& blend,
            const D3D12_DEPTH_STENCIL_DESC& depthStencil,
            const D3D12_RASTERIZER_DESC& rasterizer,
            const RenderTargetState& renderTarget,
            D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
            D3D12_INDEX_BUFFER_STRIP_CUT_VALUE stripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED)
            :
            inputLayout{},
            blendDesc(blend),
            depthStencilDesc(depthStencil),
            rasterizerDesc(rasterizer),
            renderTargetState(renderTarget),
            primitiveTopology(primitiveTopology),
            stripCutValue(stripCutValue)
        {
            if (inputLayout)
                this->inputLayout = *inputLayout;
        }

        void CreatePipelineState(
            _In_ ID3D12Device* device,
            _In_ ID3D12RootSignature* rootSignature,
            const D3D12_SHADER_BYTECODE& vertexShader,
            const D3D12_SHADER_BYTECODE& pixelShader,
            _Outptr_ ID3D12PipelineState** pPipelineState) const;

        uint32_t ComputeHash() const;

        D3D12_INPUT_LAYOUT_DESC             inputLayout;
        D3D12_BLEND_DESC                    blendDesc;
        D3D12_DEPTH_STENCIL_DESC            depthStencilDesc;
        D3D12_RASTERIZER_DESC               rasterizerDesc;
        RenderTargetState                   renderTargetState;
        D3D12_PRIMITIVE_TOPOLOGY_TYPE       primitiveTopology;
        D3D12_INDEX_BUFFER_STRIP_CUT_VALUE  stripCutValue;
    };
}