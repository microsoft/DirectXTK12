//--------------------------------------------------------------------------------------
// File: CommonStates.h
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

#include <memory>


namespace DirectX
{
    inline namespace DX12
    {
        class CommonStates
        {
        public:
            explicit CommonStates(_In_ ID3D12Device* device);

            CommonStates(CommonStates&&) noexcept;
            CommonStates& operator = (CommonStates&&) noexcept;

            CommonStates(const CommonStates&) = delete;
            CommonStates& operator = (const CommonStates&) = delete;

            virtual ~CommonStates();

            // Blend states.
            static const D3D12_BLEND_DESC Opaque;
            static const D3D12_BLEND_DESC AlphaBlend;
            static const D3D12_BLEND_DESC Additive;
            static const D3D12_BLEND_DESC NonPremultiplied;

            // Depth stencil states.
            static const D3D12_DEPTH_STENCIL_DESC DepthNone;
            static const D3D12_DEPTH_STENCIL_DESC DepthDefault;
            static const D3D12_DEPTH_STENCIL_DESC DepthRead;
            static const D3D12_DEPTH_STENCIL_DESC DepthReverseZ;
            static const D3D12_DEPTH_STENCIL_DESC DepthReadReverseZ;

            // Rasterizer states.
            static const D3D12_RASTERIZER_DESC CullNone;
            static const D3D12_RASTERIZER_DESC CullClockwise;
            static const D3D12_RASTERIZER_DESC CullCounterClockwise;
            static const D3D12_RASTERIZER_DESC Wireframe;

            // Static sampler states.
            static const D3D12_STATIC_SAMPLER_DESC StaticPointWrap(unsigned int shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility = D3D12_SHADER_VISIBILITY_ALL, unsigned int registerSpace = 0) noexcept;
            static const D3D12_STATIC_SAMPLER_DESC StaticPointClamp(unsigned int shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility = D3D12_SHADER_VISIBILITY_ALL, unsigned int registerSpace = 0) noexcept;
            static const D3D12_STATIC_SAMPLER_DESC StaticLinearWrap(unsigned int shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility = D3D12_SHADER_VISIBILITY_ALL, unsigned int registerSpace = 0) noexcept;
            static const D3D12_STATIC_SAMPLER_DESC StaticLinearClamp(unsigned int shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility = D3D12_SHADER_VISIBILITY_ALL, unsigned int registerSpace = 0) noexcept;
            static const D3D12_STATIC_SAMPLER_DESC StaticAnisotropicWrap(unsigned int shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility = D3D12_SHADER_VISIBILITY_ALL, unsigned int registerSpace = 0) noexcept;
            static const D3D12_STATIC_SAMPLER_DESC StaticAnisotropicClamp(unsigned int shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility = D3D12_SHADER_VISIBILITY_ALL, unsigned int registerSpace = 0) noexcept;

            // Sampler states.
            D3D12_GPU_DESCRIPTOR_HANDLE PointWrap() const;
            D3D12_GPU_DESCRIPTOR_HANDLE PointClamp() const;
            D3D12_GPU_DESCRIPTOR_HANDLE LinearWrap() const;
            D3D12_GPU_DESCRIPTOR_HANDLE LinearClamp() const;
            D3D12_GPU_DESCRIPTOR_HANDLE AnisotropicWrap() const;
            D3D12_GPU_DESCRIPTOR_HANDLE AnisotropicClamp() const;

            // These index into the heap returned by SamplerDescriptorHeap
            enum class SamplerIndex
            {
                PointWrap,
                PointClamp,
                LinearWrap,
                LinearClamp,
                AnisotropicWrap,
                AnisotropicClamp,
                Count
            };

            ID3D12DescriptorHeap* Heap() const noexcept;

        private:
            class Impl;

            std::unique_ptr<Impl> pImpl;
        };
    }
}
