//--------------------------------------------------------------------------------------
// File: SpriteBatch.h
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

#include <DirectXMath.h>
#include <DirectXColors.h>
#include <functional>
#include <memory>

#include "RenderTargetState.h"

namespace DirectX
{
    class ResourceUploadBatch;
    
    enum SpriteSortMode
    {
        SpriteSortMode_Deferred,
        SpriteSortMode_Immediate,
        SpriteSortMode_Texture,
        SpriteSortMode_BackToFront,
        SpriteSortMode_FrontToBack,
    };
    
    enum SpriteEffects
    {
        SpriteEffects_None = 0,
        SpriteEffects_FlipHorizontally = 1,
        SpriteEffects_FlipVertically = 2,
        SpriteEffects_FlipBoth = SpriteEffects_FlipHorizontally | SpriteEffects_FlipVertically,
    };

    class SpriteBatchShaderDescription
    {
    public:
        SpriteBatchShaderDescription(
            _In_ ID3D12RootSignature* rootSignature,
            _In_ const D3D12_SHADER_BYTECODE* vertexShader,
            _In_ const D3D12_SHADER_BYTECODE* pixelShader)
            : rootSignature(rootSignature),
            vertexShaderByteCode(vertexShader),
            pixelShaderByteCode(pixelShader)
        {
        }

        ID3D12RootSignature* rootSignature;
        const D3D12_SHADER_BYTECODE* vertexShaderByteCode;
        const D3D12_SHADER_BYTECODE* pixelShaderByteCode;
    };

    class SpriteBatchPipelineStateDescription
    {
    public:
        explicit SpriteBatchPipelineStateDescription(
            _In_ const RenderTargetState* renderTarget,
            _In_opt_ const D3D12_BLEND_DESC* blend = nullptr,
            _In_opt_ const D3D12_DEPTH_STENCIL_DESC* depthStencil = nullptr,
            _In_opt_ const D3D12_RASTERIZER_DESC* rasterizer = nullptr,
            _In_opt_ const SpriteBatchShaderDescription* shaders = nullptr)
            :
            blendDesc(blend),
            depthStencilDesc(depthStencil),
            rasterizerDesc(rasterizer),
            renderTargetState(renderTarget),
            shaders(shaders)
        {
            // constructor
        }

        const D3D12_BLEND_DESC* blendDesc;
        const D3D12_DEPTH_STENCIL_DESC* depthStencilDesc;
        const D3D12_RASTERIZER_DESC* rasterizerDesc;
        const RenderTargetState* renderTargetState;
        const SpriteBatchShaderDescription* shaders;
    };
    
    class SpriteBatch
    {
    public:
        SpriteBatch(_In_ ID3D12Device* device, _In_ ResourceUploadBatch& upload, _In_ const SpriteBatchPipelineStateDescription* psoDesc, _In_opt_ const D3D12_VIEWPORT* viewport = nullptr);
        SpriteBatch(SpriteBatch&& moveFrom);
        SpriteBatch& operator= (SpriteBatch&& moveFrom);

        SpriteBatch(SpriteBatch const&) = delete;
        SpriteBatch& operator= (SpriteBatch const&) = delete;

        virtual ~SpriteBatch();

        // Begin/End a batch of sprite drawing operations.
        void XM_CALLCONV Begin(
            _In_ ID3D12GraphicsCommandList* commandList,
            _In_opt_ SpriteSortMode sortMode = SpriteSortMode_Deferred,
            _In_opt_ FXMMATRIX transformMatrix = MatrixIdentity);
        void __cdecl End();

        // Draw overloads specifying position, origin and scale as XMFLOAT2.
        void XM_CALLCONV Draw(_In_ D3D12_GPU_DESCRIPTOR_HANDLE textureSRV, _In_ XMUINT2 textureSize, XMFLOAT2 const& position, FXMVECTOR color = Colors::White);
        void XM_CALLCONV Draw(_In_ D3D12_GPU_DESCRIPTOR_HANDLE textureSRV, _In_ XMUINT2 textureSize, XMFLOAT2 const& position, _In_opt_ RECT const* sourceRectangle, FXMVECTOR color = Colors::White, float rotation = 0, XMFLOAT2 const& origin = Float2Zero, float scale = 1, SpriteEffects effects = SpriteEffects_None, float layerDepth = 0);
        void XM_CALLCONV Draw(_In_ D3D12_GPU_DESCRIPTOR_HANDLE textureSRV, _In_ XMUINT2 textureSize, XMFLOAT2 const& position, _In_opt_ RECT const* sourceRectangle, FXMVECTOR color, float rotation, XMFLOAT2 const& origin, XMFLOAT2 const& scale, SpriteEffects effects = SpriteEffects_None, float layerDepth = 0);

        // Draw overloads specifying position, origin and scale via the first two components of an XMVECTOR.
        void XM_CALLCONV Draw(_In_ D3D12_GPU_DESCRIPTOR_HANDLE textureSRV, _In_ XMUINT2 textureSize, FXMVECTOR position, FXMVECTOR color = Colors::White);
        void XM_CALLCONV Draw(_In_ D3D12_GPU_DESCRIPTOR_HANDLE textureSRV, _In_ XMUINT2 textureSize, FXMVECTOR position, _In_opt_ RECT const* sourceRectangle, FXMVECTOR color = Colors::White, float rotation = 0, FXMVECTOR origin = g_XMZero, float scale = 1, SpriteEffects effects = SpriteEffects_None, float layerDepth = 0);
        void XM_CALLCONV Draw(_In_ D3D12_GPU_DESCRIPTOR_HANDLE textureSRV, _In_ XMUINT2 textureSize, FXMVECTOR position, _In_opt_ RECT const* sourceRectangle, FXMVECTOR color, float rotation, FXMVECTOR origin, GXMVECTOR scale, SpriteEffects effects = SpriteEffects_None, float layerDepth = 0);

        // Draw overloads specifying position as a RECT.
        void XM_CALLCONV Draw(_In_ D3D12_GPU_DESCRIPTOR_HANDLE textureSRV, _In_ XMUINT2 textureSize, RECT const& destinationRectangle, FXMVECTOR color = Colors::White);
        void XM_CALLCONV Draw(_In_ D3D12_GPU_DESCRIPTOR_HANDLE textureSRV, _In_ XMUINT2 textureSize, RECT const& destinationRectangle, _In_opt_ RECT const* sourceRectangle, FXMVECTOR color = Colors::White, float rotation = 0, XMFLOAT2 const& origin = Float2Zero, SpriteEffects effects = SpriteEffects_None, float layerDepth = 0);

        // Rotation mode to be applied to the sprite transformation
        void __cdecl SetRotation( DXGI_MODE_ROTATION mode );
        DXGI_MODE_ROTATION __cdecl GetRotation() const;

        // Set viewport for sprite transformation
        void __cdecl SetViewport( const D3D12_VIEWPORT& viewPort );

    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;

        static const XMMATRIX MatrixIdentity;
        static const XMFLOAT2 Float2Zero;
    };
}
