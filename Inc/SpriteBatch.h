//--------------------------------------------------------------------------------------
// File: SpriteBatch.h
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
#ifdef USING_DIRECTX_HEADERS
#include <directx/d3d12.h>
#include <dxguids/dxguids.h>
#else
#include <d3d12.h>
#endif
#include <dxgi1_4.h>
#endif

#include <cstdint>
#include <functional>
#include <memory>

#include <DirectXMath.h>
#include <DirectXColors.h>

#include "RenderTargetState.h"

#ifndef DIRECTX_TOOLKIT_API
#ifdef DIRECTX_TOOLKIT_EXPORT
#define DIRECTX_TOOLKIT_API __declspec(dllexport)
#elif defined(DIRECTX_TOOLKIT_IMPORT)
#define DIRECTX_TOOLKIT_API __declspec(dllimport)
#else
#define DIRECTX_TOOLKIT_API
#endif
#endif


namespace DirectX
{
    class ResourceUploadBatch;

    inline namespace DX12
    {
        enum SpriteSortMode : uint32_t
        {
            SpriteSortMode_Deferred,
            SpriteSortMode_Immediate,
            SpriteSortMode_Texture,
            SpriteSortMode_BackToFront,
            SpriteSortMode_FrontToBack,
        };

        enum SpriteEffects : uint32_t
        {
            SpriteEffects_None = 0,
            SpriteEffects_FlipHorizontally = 1,
            SpriteEffects_FlipVertically = 2,
            SpriteEffects_FlipBoth = SpriteEffects_FlipHorizontally | SpriteEffects_FlipVertically,
        };

        class DIRECTX_TOOLKIT_API SpriteBatchPipelineStateDescription
        {
        public:
            explicit SpriteBatchPipelineStateDescription(
                const RenderTargetState& renderTarget,
                _In_opt_ const D3D12_BLEND_DESC* blend = nullptr,
                _In_opt_ const D3D12_DEPTH_STENCIL_DESC* depthStencil = nullptr,
                _In_opt_ const D3D12_RASTERIZER_DESC* rasterizer = nullptr,
                _In_opt_ const D3D12_GPU_DESCRIPTOR_HANDLE* isamplerDescriptor = nullptr) noexcept
                :
                blendDesc(blend ? *blend : s_DefaultBlendDesc),
                depthStencilDesc(depthStencil ? *depthStencil : s_DefaultDepthStencilDesc),
                rasterizerDesc(rasterizer ? *rasterizer : s_DefaultRasterizerDesc),
                renderTargetState(renderTarget),
                samplerDescriptor{},
                customRootSignature(nullptr),
                customVertexShader{},
                customPixelShader{}
            {
                if (isamplerDescriptor)
                    this->samplerDescriptor = *isamplerDescriptor;
            }

            D3D12_BLEND_DESC            blendDesc;
            D3D12_DEPTH_STENCIL_DESC    depthStencilDesc;
            D3D12_RASTERIZER_DESC       rasterizerDesc;
            RenderTargetState           renderTargetState;
            D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptor;
            ID3D12RootSignature*        customRootSignature;
            D3D12_SHADER_BYTECODE       customVertexShader;
            D3D12_SHADER_BYTECODE       customPixelShader;

        private:
            static const D3D12_BLEND_DESC           s_DefaultBlendDesc;
            static const D3D12_RASTERIZER_DESC      s_DefaultRasterizerDesc;
            static const D3D12_DEPTH_STENCIL_DESC   s_DefaultDepthStencilDesc;
        };

        class SpriteBatch
        {
        public:
            DIRECTX_TOOLKIT_API SpriteBatch(
                _In_ ID3D12Device* device, ResourceUploadBatch& upload,
                const SpriteBatchPipelineStateDescription& psoDesc,
                _In_opt_ const D3D12_VIEWPORT* viewport = nullptr);

            DIRECTX_TOOLKIT_API SpriteBatch(SpriteBatch&&) noexcept;
            DIRECTX_TOOLKIT_API SpriteBatch& operator= (SpriteBatch&&) noexcept;

            SpriteBatch(SpriteBatch const&) = delete;
            SpriteBatch& operator= (SpriteBatch const&) = delete;

            DIRECTX_TOOLKIT_API virtual ~SpriteBatch();

            // Begin/End a batch of sprite drawing operations.
            DIRECTX_TOOLKIT_API void XM_CALLCONV Begin(
                _In_ ID3D12GraphicsCommandList* commandList,
                SpriteSortMode sortMode = SpriteSortMode_Deferred,
                FXMMATRIX transformMatrix = MatrixIdentity);
            DIRECTX_TOOLKIT_API void XM_CALLCONV Begin(
                _In_ ID3D12GraphicsCommandList* commandList,
                D3D12_GPU_DESCRIPTOR_HANDLE sampler,
                SpriteSortMode sortMode = SpriteSortMode_Deferred,
                FXMMATRIX transformMatrix = MatrixIdentity);
            DIRECTX_TOOLKIT_API void __cdecl End();

            // Draw overloads specifying position, origin and scale as XMFLOAT2.
            DIRECTX_TOOLKIT_API void XM_CALLCONV Draw(
                D3D12_GPU_DESCRIPTOR_HANDLE textureSRV, XMUINT2 const& textureSize,
                XMFLOAT2 const& position,
                FXMVECTOR color = Colors::White);
            DIRECTX_TOOLKIT_API void XM_CALLCONV Draw(
                D3D12_GPU_DESCRIPTOR_HANDLE textureSRV, XMUINT2 const& textureSize,
                XMFLOAT2 const& position, _In_opt_ RECT const* sourceRectangle,
                FXMVECTOR color = Colors::White, float rotation = 0, XMFLOAT2 const& origin = Float2Zero, float scale = 1,
                SpriteEffects effects = SpriteEffects_None, float layerDepth = 0);
            DIRECTX_TOOLKIT_API void XM_CALLCONV Draw(
                D3D12_GPU_DESCRIPTOR_HANDLE textureSRV, XMUINT2 const& textureSize,
                XMFLOAT2 const& position, _In_opt_ RECT const* sourceRectangle,
                FXMVECTOR color, float rotation, XMFLOAT2 const& origin, XMFLOAT2 const& scale,
                SpriteEffects effects = SpriteEffects_None, float layerDepth = 0);

            // Draw overloads specifying position, origin and scale via the first two components of an XMVECTOR.
            DIRECTX_TOOLKIT_API void XM_CALLCONV Draw(
                D3D12_GPU_DESCRIPTOR_HANDLE textureSRV, XMUINT2 const& textureSize,
                FXMVECTOR position,
                FXMVECTOR color = Colors::White);
            DIRECTX_TOOLKIT_API void XM_CALLCONV Draw(
                D3D12_GPU_DESCRIPTOR_HANDLE textureSRV, XMUINT2 const& textureSize,
                FXMVECTOR position, _In_opt_ RECT const* sourceRectangle,
                FXMVECTOR color = Colors::White, float rotation = 0, FXMVECTOR origin = g_XMZero, float scale = 1,
                SpriteEffects effects = SpriteEffects_None, float layerDepth = 0);
            DIRECTX_TOOLKIT_API void XM_CALLCONV Draw(
                D3D12_GPU_DESCRIPTOR_HANDLE textureSRV, XMUINT2 const& textureSize,
                FXMVECTOR position, _In_opt_ RECT const* sourceRectangle,
                FXMVECTOR color, float rotation, FXMVECTOR origin, GXMVECTOR scale,
                SpriteEffects effects = SpriteEffects_None, float layerDepth = 0);

            // Draw overloads specifying position as a RECT.
            DIRECTX_TOOLKIT_API void XM_CALLCONV Draw(
                D3D12_GPU_DESCRIPTOR_HANDLE textureSRV, XMUINT2 const& textureSize,
                RECT const& destinationRectangle,
                FXMVECTOR color = Colors::White);
            DIRECTX_TOOLKIT_API void XM_CALLCONV Draw(D3D12_GPU_DESCRIPTOR_HANDLE
                 textureSRV, XMUINT2 const& textureSize,
                 RECT const& destinationRectangle, _In_opt_ RECT const* sourceRectangle,
                 FXMVECTOR color = Colors::White, float rotation = 0, XMFLOAT2 const& origin = Float2Zero,
                 SpriteEffects effects = SpriteEffects_None, float layerDepth = 0);

            // Rotation mode to be applied to the sprite transformation
        #if defined(__dxgi1_2_h__) || defined(__d3d11_x_h__) || defined(__d3d12_x_h__) || defined(__XBOX_D3D12_X__)
            DIRECTX_TOOLKIT_API void __cdecl SetRotation(DXGI_MODE_ROTATION mode);
            DIRECTX_TOOLKIT_API DXGI_MODE_ROTATION __cdecl GetRotation() const noexcept;
        #endif

            // Set viewport for sprite transformation
            DIRECTX_TOOLKIT_API void __cdecl SetViewport(const D3D12_VIEWPORT& viewPort);

        private:
            // Private implementation.
            struct Impl;

            std::unique_ptr<Impl> pImpl;

            DIRECTX_TOOLKIT_API static const XMMATRIX MatrixIdentity;
            DIRECTX_TOOLKIT_API static const XMFLOAT2 Float2Zero;
        };
    }
}
