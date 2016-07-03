//--------------------------------------------------------------------------------------
// File: CommonStates.h
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


namespace DirectX
{
    class CommonStates
    {
    public:
        CommonStates() = delete;

        // Blend states.
        static const D3D12_BLEND_DESC Opaque;
        static const D3D12_BLEND_DESC AlphaBlend;
        static const D3D12_BLEND_DESC Additive;
        static const D3D12_BLEND_DESC NonPremultiplied;
        
        // Depth stencil states.
        static const D3D12_DEPTH_STENCIL_DESC DepthNone;
        static const D3D12_DEPTH_STENCIL_DESC DepthDefault;
        static const D3D12_DEPTH_STENCIL_DESC DepthRead;

        // Rasterizer states.
        static const D3D12_RASTERIZER_DESC CullNone;
        static const D3D12_RASTERIZER_DESC CullClockwise;
        static const D3D12_RASTERIZER_DESC CullCounterClockwise;
        static const D3D12_RASTERIZER_DESC Wireframe;

        // Sampler states.
        static const D3D12_SAMPLER_DESC PointWrap;
        static const D3D12_SAMPLER_DESC PointClamp;
        static const D3D12_SAMPLER_DESC LinearWrap;
        static const D3D12_SAMPLER_DESC LinearClamp;
        static const D3D12_SAMPLER_DESC AnisotropicWrap;
        static const D3D12_SAMPLER_DESC AnisotropicClamp;

        // Static sampler states.
        static const D3D12_STATIC_SAMPLER_DESC StaticPointWrap(unsigned int shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility = D3D12_SHADER_VISIBILITY_ALL, unsigned int registerSpace = 0);
        static const D3D12_STATIC_SAMPLER_DESC StaticPointClamp(unsigned int shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility = D3D12_SHADER_VISIBILITY_ALL, unsigned int registerSpace = 0);
        static const D3D12_STATIC_SAMPLER_DESC StaticLinearWrap(unsigned int shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility = D3D12_SHADER_VISIBILITY_ALL, unsigned int registerSpace = 0);
        static const D3D12_STATIC_SAMPLER_DESC StaticLinearClamp(unsigned int shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility = D3D12_SHADER_VISIBILITY_ALL, unsigned int registerSpace = 0);
        static const D3D12_STATIC_SAMPLER_DESC StaticAnisotropicWrap(unsigned int shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility = D3D12_SHADER_VISIBILITY_ALL, unsigned int registerSpace = 0);
        static const D3D12_STATIC_SAMPLER_DESC StaticAnisotropicClamp(unsigned int shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility = D3D12_SHADER_VISIBILITY_ALL, unsigned int registerSpace = 0);
    };
}
