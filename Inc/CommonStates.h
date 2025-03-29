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

#include <cstdint>
#include <memory>

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
    inline namespace DX12
    {
        class CommonStates
        {
        public:
            DIRECTX_TOOLKIT_API explicit CommonStates(_In_ ID3D12Device* device);

            DIRECTX_TOOLKIT_API CommonStates(CommonStates&&) noexcept;
            DIRECTX_TOOLKIT_API CommonStates& operator = (CommonStates&&) noexcept;

            CommonStates(const CommonStates&) = delete;
            CommonStates& operator = (const CommonStates&) = delete;

            DIRECTX_TOOLKIT_API virtual ~CommonStates();

            // Blend states.
            DIRECTX_TOOLKIT_API static const D3D12_BLEND_DESC Opaque;
            DIRECTX_TOOLKIT_API static const D3D12_BLEND_DESC AlphaBlend;
            DIRECTX_TOOLKIT_API static const D3D12_BLEND_DESC Additive;
            DIRECTX_TOOLKIT_API static const D3D12_BLEND_DESC NonPremultiplied;

            // Depth stencil states.
            DIRECTX_TOOLKIT_API static const D3D12_DEPTH_STENCIL_DESC DepthNone;
            DIRECTX_TOOLKIT_API static const D3D12_DEPTH_STENCIL_DESC DepthDefault;
            DIRECTX_TOOLKIT_API static const D3D12_DEPTH_STENCIL_DESC DepthRead;
            DIRECTX_TOOLKIT_API static const D3D12_DEPTH_STENCIL_DESC DepthReverseZ;
            DIRECTX_TOOLKIT_API static const D3D12_DEPTH_STENCIL_DESC DepthReadReverseZ;

            // Rasterizer states.
            DIRECTX_TOOLKIT_API static const D3D12_RASTERIZER_DESC CullNone;
            DIRECTX_TOOLKIT_API static const D3D12_RASTERIZER_DESC CullClockwise;
            DIRECTX_TOOLKIT_API static const D3D12_RASTERIZER_DESC CullCounterClockwise;
            DIRECTX_TOOLKIT_API static const D3D12_RASTERIZER_DESC Wireframe;

            // Static sampler states.
            DIRECTX_TOOLKIT_API static const D3D12_STATIC_SAMPLER_DESC StaticPointWrap(unsigned int shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility = D3D12_SHADER_VISIBILITY_ALL, unsigned int registerSpace = 0) noexcept;
            DIRECTX_TOOLKIT_API static const D3D12_STATIC_SAMPLER_DESC StaticPointClamp(unsigned int shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility = D3D12_SHADER_VISIBILITY_ALL, unsigned int registerSpace = 0) noexcept;
            DIRECTX_TOOLKIT_API static const D3D12_STATIC_SAMPLER_DESC StaticLinearWrap(unsigned int shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility = D3D12_SHADER_VISIBILITY_ALL, unsigned int registerSpace = 0) noexcept;
            DIRECTX_TOOLKIT_API static const D3D12_STATIC_SAMPLER_DESC StaticLinearClamp(unsigned int shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility = D3D12_SHADER_VISIBILITY_ALL, unsigned int registerSpace = 0) noexcept;
            DIRECTX_TOOLKIT_API static const D3D12_STATIC_SAMPLER_DESC StaticAnisotropicWrap(unsigned int shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility = D3D12_SHADER_VISIBILITY_ALL, unsigned int registerSpace = 0) noexcept;
            DIRECTX_TOOLKIT_API static const D3D12_STATIC_SAMPLER_DESC StaticAnisotropicClamp(unsigned int shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility = D3D12_SHADER_VISIBILITY_ALL, unsigned int registerSpace = 0) noexcept;

            // Sampler states.
            DIRECTX_TOOLKIT_API D3D12_GPU_DESCRIPTOR_HANDLE PointWrap() const;
            DIRECTX_TOOLKIT_API D3D12_GPU_DESCRIPTOR_HANDLE PointClamp() const;
            DIRECTX_TOOLKIT_API D3D12_GPU_DESCRIPTOR_HANDLE LinearWrap() const;
            DIRECTX_TOOLKIT_API D3D12_GPU_DESCRIPTOR_HANDLE LinearClamp() const;
            DIRECTX_TOOLKIT_API D3D12_GPU_DESCRIPTOR_HANDLE AnisotropicWrap() const;
            DIRECTX_TOOLKIT_API D3D12_GPU_DESCRIPTOR_HANDLE AnisotropicClamp() const;

            // These index into the heap returned by SamplerDescriptorHeap
            enum class SamplerIndex : uint32_t
            {
                PointWrap,
                PointClamp,
                LinearWrap,
                LinearClamp,
                AnisotropicWrap,
                AnisotropicClamp,
                Count
            };

            DIRECTX_TOOLKIT_API ID3D12DescriptorHeap* Heap() const noexcept;

        private:
            class Impl;

            std::unique_ptr<Impl> pImpl;
        };
    }
}
