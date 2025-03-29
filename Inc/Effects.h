//--------------------------------------------------------------------------------------
// File: Effects.h
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#pragma once

#ifndef __DIRECTXTK_EFFECTS_H__
#define __DIRECTXTK_EFFECTS_H__

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

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include <DirectXMath.h>

#include "RenderTargetState.h"
#include "EffectPipelineStateDescription.h"

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

#if defined(DIRECTX_TOOLKIT_IMPORT) && defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251 4275)
#endif


namespace DirectX
{
    class DescriptorHeap;
    class ResourceUploadBatch;

    inline namespace DX12
    {
        //------------------------------------------------------------------------------
        // Abstract interface representing any effect which can be applied onto a D3D device context.
        class DIRECTX_TOOLKIT_API IEffect
        {
        public:
            virtual ~IEffect() = default;

            IEffect(const IEffect&) = delete;
            IEffect& operator=(const IEffect&) = delete;

            virtual void __cdecl Apply(_In_ ID3D12GraphicsCommandList* commandList) = 0;

        protected:
            IEffect() = default;
            IEffect(IEffect&&) = default;
            IEffect& operator=(IEffect&&) = default;
        };


        // Abstract interface for effects with world, view, and projection matrices.
        class DIRECTX_TOOLKIT_API IEffectMatrices
        {
        public:
            virtual ~IEffectMatrices() = default;

            IEffectMatrices(const IEffectMatrices&) = delete;
            IEffectMatrices& operator=(const IEffectMatrices&) = delete;

            virtual void XM_CALLCONV SetWorld(FXMMATRIX value) = 0;
            virtual void XM_CALLCONV SetView(FXMMATRIX value) = 0;
            virtual void XM_CALLCONV SetProjection(FXMMATRIX value) = 0;
            virtual void XM_CALLCONV SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection);

        protected:
            IEffectMatrices() = default;
            IEffectMatrices(IEffectMatrices&&) = default;
            IEffectMatrices& operator=(IEffectMatrices&&) = default;
        };


        // Abstract interface for effects which support directional lighting.
        class DIRECTX_TOOLKIT_API IEffectLights
        {
        public:
            virtual ~IEffectLights() = default;

            IEffectLights(const IEffectLights&) = delete;
            IEffectLights& operator=(const IEffectLights&) = delete;

            virtual void XM_CALLCONV SetAmbientLightColor(FXMVECTOR value) = 0;

            virtual void __cdecl SetLightEnabled(int whichLight, bool value) = 0;
            virtual void XM_CALLCONV SetLightDirection(int whichLight, FXMVECTOR value) = 0;
            virtual void XM_CALLCONV SetLightDiffuseColor(int whichLight, FXMVECTOR value) = 0;
            virtual void XM_CALLCONV SetLightSpecularColor(int whichLight, FXMVECTOR value) = 0;

            virtual void __cdecl EnableDefaultLighting() = 0;

            static constexpr int MaxDirectionalLights = 3;

        protected:
            IEffectLights() = default;
            IEffectLights(IEffectLights&&) = default;
            IEffectLights& operator=(IEffectLights&&) = default;
        };


        // Abstract interface for effects which support fog.
        class DIRECTX_TOOLKIT_API IEffectFog
        {
        public:
            virtual ~IEffectFog() = default;

            IEffectFog(const IEffectFog&) = delete;
            IEffectFog& operator=(const IEffectFog&) = delete;

            virtual void __cdecl SetFogStart(float value) = 0;
            virtual void __cdecl SetFogEnd(float value) = 0;
            virtual void XM_CALLCONV SetFogColor(FXMVECTOR value) = 0;

        protected:
            IEffectFog() = default;
            IEffectFog(IEffectFog&&) = default;
            IEffectFog& operator=(IEffectFog&&) = default;
        };


        // Abstract interface for effects which support skinning
        class DIRECTX_TOOLKIT_API IEffectSkinning
        {
        public:
            virtual ~IEffectSkinning() = default;

            IEffectSkinning(const IEffectSkinning&) = delete;
            IEffectSkinning& operator=(const IEffectSkinning&) = delete;

            virtual void __cdecl SetBoneTransforms(_In_reads_(count) XMMATRIX const* value, size_t count) = 0;
            virtual void __cdecl ResetBoneTransforms() = 0;

            static constexpr int MaxBones = 72;

        protected:
            IEffectSkinning() = default;
            IEffectSkinning(IEffectSkinning&&) = default;
            IEffectSkinning& operator=(IEffectSkinning&&) = default;
        };


        //------------------------------------------------------------------------------
        namespace EffectFlags
        {
            constexpr uint32_t None = 0x00;
            constexpr uint32_t Fog = 0x01;
            constexpr uint32_t Lighting = 0x02;

            constexpr uint32_t PerPixelLighting = 0x04 | Lighting;
            // per pixel lighting implies lighting enabled

            constexpr uint32_t VertexColor = 0x08;
            constexpr uint32_t Texture = 0x10;
            constexpr uint32_t Instancing = 0x20;

            constexpr uint32_t Specular = 0x100;
            // enable optional specular/specularMap feature

            constexpr uint32_t Emissive = 0x200;
            // enable optional emissive/emissiveMap feature

            constexpr uint32_t Fresnel = 0x400;
            // enable optional Fresnel feature

            constexpr uint32_t Velocity = 0x800;
            // enable optional velocity feature

            constexpr uint32_t BiasedVertexNormals = 0x10000;
            // compressed vertex normals need x2 bias
        }


        //------------------------------------------------------------------------------
        // Built-in shader supports optional texture mapping, vertex coloring, directional lighting, and fog.
        class BasicEffect : public IEffect, public IEffectMatrices, public IEffectLights, public IEffectFog
        {
        public:
            DIRECTX_TOOLKIT_API BasicEffect(
                _In_ ID3D12Device* device,
                uint32_t effectFlags,
                const EffectPipelineStateDescription& pipelineDescription);

            DIRECTX_TOOLKIT_API BasicEffect(BasicEffect&&) noexcept;
            DIRECTX_TOOLKIT_API BasicEffect& operator= (BasicEffect&&) noexcept;

            BasicEffect(BasicEffect const&) = delete;
            BasicEffect& operator= (BasicEffect const&) = delete;

            DIRECTX_TOOLKIT_API ~BasicEffect() override;

            // IEffect methods.
            DIRECTX_TOOLKIT_API void __cdecl Apply(_In_ ID3D12GraphicsCommandList* commandList) override;

            // Camera settings.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetWorld(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetView(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetProjection(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection) override;

            // Material settings.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetDiffuseColor(FXMVECTOR value);
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetEmissiveColor(FXMVECTOR value);
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetSpecularColor(FXMVECTOR value);
            DIRECTX_TOOLKIT_API void __cdecl SetSpecularPower(float value);
            DIRECTX_TOOLKIT_API void __cdecl DisableSpecular();
            DIRECTX_TOOLKIT_API void __cdecl SetAlpha(float value);
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetColorAndAlpha(FXMVECTOR value);

            // Light settings.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetAmbientLightColor(FXMVECTOR value) override;

            DIRECTX_TOOLKIT_API void __cdecl SetLightEnabled(int whichLight, bool value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetLightDirection(int whichLight, FXMVECTOR value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetLightDiffuseColor(int whichLight, FXMVECTOR value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetLightSpecularColor(int whichLight, FXMVECTOR value) override;

            DIRECTX_TOOLKIT_API void __cdecl EnableDefaultLighting() override;

            // Fog settings.
            DIRECTX_TOOLKIT_API void __cdecl SetFogStart(float value) override;
            DIRECTX_TOOLKIT_API void __cdecl SetFogEnd(float value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetFogColor(FXMVECTOR value) override;

            // Texture setting.
            DIRECTX_TOOLKIT_API void __cdecl SetTexture(D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor, D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptor);

        private:
            // Private implementation.
            class Impl;

            std::unique_ptr<Impl> pImpl;
        };


        // Built-in shader supports per-pixel alpha testing.
        class AlphaTestEffect : public IEffect, public IEffectMatrices, public IEffectFog
        {
        public:
            DIRECTX_TOOLKIT_API AlphaTestEffect(
                _In_ ID3D12Device* device,
                uint32_t effectFlags,
                const EffectPipelineStateDescription& pipelineDescription,
                D3D12_COMPARISON_FUNC alphaFunction = D3D12_COMPARISON_FUNC_GREATER);

            DIRECTX_TOOLKIT_API AlphaTestEffect(AlphaTestEffect&&) noexcept;
            DIRECTX_TOOLKIT_API AlphaTestEffect& operator= (AlphaTestEffect&&) noexcept;

            AlphaTestEffect(AlphaTestEffect const&) = delete;
            AlphaTestEffect& operator= (AlphaTestEffect const&) = delete;

            DIRECTX_TOOLKIT_API ~AlphaTestEffect() override;

            // IEffect methods.
            DIRECTX_TOOLKIT_API void __cdecl Apply(_In_ ID3D12GraphicsCommandList* commandList) override;

            // Camera settings.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetWorld(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetView(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetProjection(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection) override;

            // Material settings.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetDiffuseColor(FXMVECTOR value);
            DIRECTX_TOOLKIT_API void __cdecl SetAlpha(float value);
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetColorAndAlpha(FXMVECTOR value);

            // Fog settings.
            DIRECTX_TOOLKIT_API void __cdecl SetFogStart(float value) override;
            DIRECTX_TOOLKIT_API void __cdecl SetFogEnd(float value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetFogColor(FXMVECTOR value) override;

            // Texture setting.
            DIRECTX_TOOLKIT_API void __cdecl SetTexture(D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor, D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptor);

            // Alpha test settings.
            DIRECTX_TOOLKIT_API void __cdecl SetReferenceAlpha(int value);

        private:
            // Private implementation.
            class Impl;

            std::unique_ptr<Impl> pImpl;
        };


        // Built-in shader supports two layer multitexturing (eg. for lightmaps or detail textures).
        class DualTextureEffect : public IEffect, public IEffectMatrices, public IEffectFog
        {
        public:
            DIRECTX_TOOLKIT_API DualTextureEffect(
                _In_ ID3D12Device* device,
                uint32_t effectFlags,
                const EffectPipelineStateDescription& pipelineDescription);

            DIRECTX_TOOLKIT_API DualTextureEffect(DualTextureEffect&&) noexcept;
            DIRECTX_TOOLKIT_API DualTextureEffect& operator= (DualTextureEffect&&) noexcept;

            DualTextureEffect(DualTextureEffect const&) = delete;
            DualTextureEffect& operator= (DualTextureEffect const&) = delete;

            DIRECTX_TOOLKIT_API ~DualTextureEffect() override;

            // IEffect methods.
            DIRECTX_TOOLKIT_API void __cdecl Apply(_In_ ID3D12GraphicsCommandList* commandList) override;

            // Camera settings.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetWorld(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetView(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetProjection(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection) override;

            // Material settings.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetDiffuseColor(FXMVECTOR value);
            DIRECTX_TOOLKIT_API void __cdecl SetAlpha(float value);
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetColorAndAlpha(FXMVECTOR value);

            // Fog settings.
            DIRECTX_TOOLKIT_API void __cdecl SetFogStart(float value) override;
            DIRECTX_TOOLKIT_API void __cdecl SetFogEnd(float value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetFogColor(FXMVECTOR value) override;

            // Texture settings.
            DIRECTX_TOOLKIT_API void __cdecl SetTexture(D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor, D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptor);
            DIRECTX_TOOLKIT_API void __cdecl SetTexture2(D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor, D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptor);

        private:
            // Private implementation.
            class Impl;

            std::unique_ptr<Impl> pImpl;
        };


        // Built-in shader supports cubic environment mapping.
        class EnvironmentMapEffect : public IEffect, public IEffectMatrices, public IEffectLights, public IEffectFog
        {
        public:
            enum Mapping : uint32_t
            {
                Mapping_Cube = 0,       // Cubic environment map
                Mapping_Sphere,         // Spherical environment map
                Mapping_DualParabola,   // Dual-parabola environment map (requires Feature Level 10.0)
            };

            DIRECTX_TOOLKIT_API EnvironmentMapEffect(
                _In_ ID3D12Device* device,
                uint32_t effectFlags,
                const EffectPipelineStateDescription& pipelineDescription,
                Mapping mapping = Mapping_Cube);

            DIRECTX_TOOLKIT_API EnvironmentMapEffect(EnvironmentMapEffect&&) noexcept;
            DIRECTX_TOOLKIT_API EnvironmentMapEffect& operator= (EnvironmentMapEffect&&) noexcept;

            EnvironmentMapEffect(EnvironmentMapEffect const&) = delete;
            EnvironmentMapEffect& operator= (EnvironmentMapEffect const&) = delete;

            DIRECTX_TOOLKIT_API ~EnvironmentMapEffect() override;

            // IEffect methods.
            DIRECTX_TOOLKIT_API void __cdecl Apply(_In_ ID3D12GraphicsCommandList* commandList) override;

            // Camera settings.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetWorld(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetView(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetProjection(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection) override;

            // Material settings.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetDiffuseColor(FXMVECTOR value);
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetEmissiveColor(FXMVECTOR value);
            DIRECTX_TOOLKIT_API void __cdecl SetAlpha(float value);
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetColorAndAlpha(FXMVECTOR value);

            // Light settings.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetAmbientLightColor(FXMVECTOR value) override;

            DIRECTX_TOOLKIT_API void __cdecl SetLightEnabled(int whichLight, bool value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetLightDirection(int whichLight, FXMVECTOR value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetLightDiffuseColor(int whichLight, FXMVECTOR value) override;

            DIRECTX_TOOLKIT_API void __cdecl EnableDefaultLighting() override;

            // Fog settings.
            DIRECTX_TOOLKIT_API void __cdecl SetFogStart(float value) override;
            DIRECTX_TOOLKIT_API void __cdecl SetFogEnd(float value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetFogColor(FXMVECTOR value) override;

            // Texture setting.
            DIRECTX_TOOLKIT_API void __cdecl SetTexture(D3D12_GPU_DESCRIPTOR_HANDLE texture, D3D12_GPU_DESCRIPTOR_HANDLE sampler);

            // Environment map settings.
            DIRECTX_TOOLKIT_API void __cdecl SetEnvironmentMap(D3D12_GPU_DESCRIPTOR_HANDLE texture, D3D12_GPU_DESCRIPTOR_HANDLE sampler);
            DIRECTX_TOOLKIT_API void __cdecl SetEnvironmentMapAmount(float value);
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetEnvironmentMapSpecular(FXMVECTOR value);
            DIRECTX_TOOLKIT_API void __cdecl SetFresnelFactor(float value);

        private:
            // Private implementation.
            class Impl;

            std::unique_ptr<Impl> pImpl;

            // Unsupported interface methods.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetLightSpecularColor(int whichLight, FXMVECTOR value) override;
        };


        // Built-in shader supports skinned animation.
        class SkinnedEffect : public IEffect, public IEffectMatrices, public IEffectLights, public IEffectFog, public IEffectSkinning
        {
        public:
            DIRECTX_TOOLKIT_API SkinnedEffect(
                _In_ ID3D12Device* device,
                uint32_t effectFlags,
                const EffectPipelineStateDescription& pipelineDescription);

            DIRECTX_TOOLKIT_API SkinnedEffect(SkinnedEffect&&) noexcept;
            DIRECTX_TOOLKIT_API SkinnedEffect& operator= (SkinnedEffect&&) noexcept;

            SkinnedEffect(SkinnedEffect const&) = delete;
            SkinnedEffect& operator= (SkinnedEffect const&) = delete;

            DIRECTX_TOOLKIT_API ~SkinnedEffect() override;

            // IEffect methods.
            DIRECTX_TOOLKIT_API void __cdecl Apply(_In_ ID3D12GraphicsCommandList* commandList) override;

            // Camera settings.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetWorld(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetView(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetProjection(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection) override;

            // Material settings.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetDiffuseColor(FXMVECTOR value);
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetEmissiveColor(FXMVECTOR value);
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetSpecularColor(FXMVECTOR value);
            DIRECTX_TOOLKIT_API void __cdecl SetSpecularPower(float value);
            DIRECTX_TOOLKIT_API void __cdecl DisableSpecular();
            DIRECTX_TOOLKIT_API void __cdecl SetAlpha(float value);
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetColorAndAlpha(FXMVECTOR value);

            // Light settings.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetAmbientLightColor(FXMVECTOR value) override;

            DIRECTX_TOOLKIT_API void __cdecl SetLightEnabled(int whichLight, bool value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetLightDirection(int whichLight, FXMVECTOR value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetLightDiffuseColor(int whichLight, FXMVECTOR value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetLightSpecularColor(int whichLight, FXMVECTOR value) override;

            DIRECTX_TOOLKIT_API void __cdecl EnableDefaultLighting() override;

            // Fog settings.
            DIRECTX_TOOLKIT_API void __cdecl SetFogStart(float value) override;
            DIRECTX_TOOLKIT_API void __cdecl SetFogEnd(float value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetFogColor(FXMVECTOR value) override;

            // Texture setting.
            DIRECTX_TOOLKIT_API void __cdecl SetTexture(D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor, D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptor);

            // Animation settings.
            DIRECTX_TOOLKIT_API void __cdecl SetBoneTransforms(_In_reads_(count) XMMATRIX const* value, size_t count) override;
            DIRECTX_TOOLKIT_API void __cdecl ResetBoneTransforms() override;

        private:
            // Private implementation.
            class Impl;

            std::unique_ptr<Impl> pImpl;
        };


        //------------------------------------------------------------------------------
        // Built-in shader extends BasicEffect with normal map and optional specular map
        class NormalMapEffect : public IEffect, public IEffectMatrices, public IEffectLights, public IEffectFog
        {
        public:
            DIRECTX_TOOLKIT_API inline NormalMapEffect(
                _In_ ID3D12Device* device,
                uint32_t effectFlags,
                const EffectPipelineStateDescription& pipelineDescription) :
                NormalMapEffect(device, effectFlags, pipelineDescription, false)
            {
            }

            DIRECTX_TOOLKIT_API NormalMapEffect(NormalMapEffect&&) noexcept;
            DIRECTX_TOOLKIT_API NormalMapEffect& operator= (NormalMapEffect&&) noexcept;

            NormalMapEffect(NormalMapEffect const&) = delete;
            NormalMapEffect& operator= (NormalMapEffect const&) = delete;

            DIRECTX_TOOLKIT_API ~NormalMapEffect() override;

            // IEffect methods.
            DIRECTX_TOOLKIT_API void __cdecl Apply(_In_ ID3D12GraphicsCommandList* commandList) override;

            // Camera settings.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetWorld(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetView(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetProjection(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection) override;

            // Material settings.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetDiffuseColor(FXMVECTOR value);
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetEmissiveColor(FXMVECTOR value);
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetSpecularColor(FXMVECTOR value);
            DIRECTX_TOOLKIT_API void __cdecl SetSpecularPower(float value);
            DIRECTX_TOOLKIT_API void __cdecl DisableSpecular();
            DIRECTX_TOOLKIT_API void __cdecl SetAlpha(float value);
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetColorAndAlpha(FXMVECTOR value);

            // Light settings.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetAmbientLightColor(FXMVECTOR value) override;

            DIRECTX_TOOLKIT_API void __cdecl SetLightEnabled(int whichLight, bool value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetLightDirection(int whichLight, FXMVECTOR value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetLightDiffuseColor(int whichLight, FXMVECTOR value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetLightSpecularColor(int whichLight, FXMVECTOR value) override;

            DIRECTX_TOOLKIT_API void __cdecl EnableDefaultLighting() override;

            // Fog settings.
            DIRECTX_TOOLKIT_API void __cdecl SetFogStart(float value) override;
            DIRECTX_TOOLKIT_API void __cdecl SetFogEnd(float value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetFogColor(FXMVECTOR value) override;

            // Texture setting - albedo, normal and specular intensity
            DIRECTX_TOOLKIT_API void __cdecl SetTexture(D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor, D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptor);
            DIRECTX_TOOLKIT_API void __cdecl SetNormalTexture(D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor);
            DIRECTX_TOOLKIT_API void __cdecl SetSpecularTexture(D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor);

        protected:
            // Private implementation.
            class Impl;

            std::unique_ptr<Impl> pImpl;

            DIRECTX_TOOLKIT_API NormalMapEffect(
                _In_ ID3D12Device* device,
                uint32_t effectFlags,
                const EffectPipelineStateDescription& pipelineDescription,
                bool skinningEnabled);
        };

        class DIRECTX_TOOLKIT_API SkinnedNormalMapEffect : public NormalMapEffect, public IEffectSkinning
        {
        public:
            SkinnedNormalMapEffect(
                _In_ ID3D12Device* device,
                uint32_t effectFlags,
                const EffectPipelineStateDescription& pipelineDescription) :
                NormalMapEffect(device, effectFlags, pipelineDescription, true)
            {
            }

            SkinnedNormalMapEffect(SkinnedNormalMapEffect&&) = default;
            SkinnedNormalMapEffect& operator= (SkinnedNormalMapEffect&&) = default;

            SkinnedNormalMapEffect(SkinnedNormalMapEffect const&) = delete;
            SkinnedNormalMapEffect& operator= (SkinnedNormalMapEffect const&) = delete;

            ~SkinnedNormalMapEffect() override;

            // Animation settings.
            void __cdecl SetBoneTransforms(_In_reads_(count) XMMATRIX const* value, size_t count) override;
            void __cdecl ResetBoneTransforms() override;
        };


        //------------------------------------------------------------------------------
        // Built-in shader for Physically-Based Rendering (Roughness/Metalness) with Image-based lighting
        class PBREffect : public IEffect, public IEffectMatrices, public IEffectLights
        {
        public:
            DIRECTX_TOOLKIT_API inline PBREffect(
                _In_ ID3D12Device* device,
                uint32_t effectFlags,
                const EffectPipelineStateDescription& pipelineDescription) :
                PBREffect(device, effectFlags, pipelineDescription, false)
            {
            }

            DIRECTX_TOOLKIT_API PBREffect(PBREffect&&) noexcept;
            DIRECTX_TOOLKIT_API PBREffect& operator= (PBREffect&&) noexcept;

            PBREffect(PBREffect const&) = delete;
            PBREffect& operator= (PBREffect const&) = delete;

            DIRECTX_TOOLKIT_API ~PBREffect() override;

            // IEffect methods.
            DIRECTX_TOOLKIT_API void __cdecl Apply(_In_ ID3D12GraphicsCommandList* commandList) override;

            // Camera settings.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetWorld(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetView(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetProjection(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection) override;

            // Light settings.
            DIRECTX_TOOLKIT_API void __cdecl SetLightEnabled(int whichLight, bool value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetLightDirection(int whichLight, FXMVECTOR value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetLightDiffuseColor(int whichLight, FXMVECTOR value) override;

            DIRECTX_TOOLKIT_API void __cdecl EnableDefaultLighting() override;

            // PBR Settings.
            DIRECTX_TOOLKIT_API void __cdecl SetAlpha(float value);
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetConstantAlbedo(FXMVECTOR value);
            DIRECTX_TOOLKIT_API void __cdecl SetConstantMetallic(float value);
            DIRECTX_TOOLKIT_API void __cdecl SetConstantRoughness(float value);

            // Texture settings.
            DIRECTX_TOOLKIT_API void __cdecl SetAlbedoTexture(D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor, D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptor);
            DIRECTX_TOOLKIT_API void __cdecl SetNormalTexture(D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor);
            DIRECTX_TOOLKIT_API void __cdecl SetRMATexture(D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor);

            DIRECTX_TOOLKIT_API void __cdecl SetEmissiveTexture(D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor);

            DIRECTX_TOOLKIT_API void __cdecl SetSurfaceTextures(
                D3D12_GPU_DESCRIPTOR_HANDLE albedo,
                D3D12_GPU_DESCRIPTOR_HANDLE normal,
                D3D12_GPU_DESCRIPTOR_HANDLE roughnessMetallicAmbientOcclusion,
                D3D12_GPU_DESCRIPTOR_HANDLE sampler);

            DIRECTX_TOOLKIT_API void __cdecl SetIBLTextures(
                D3D12_GPU_DESCRIPTOR_HANDLE radiance,
                int numRadianceMips,
                D3D12_GPU_DESCRIPTOR_HANDLE irradiance,
                D3D12_GPU_DESCRIPTOR_HANDLE sampler);

            // Render target size, required for velocity buffer output.
            DIRECTX_TOOLKIT_API void __cdecl SetRenderTargetSizeInPixels(int width, int height);

        protected:
            // Private implementation.
            class Impl;

            std::unique_ptr<Impl> pImpl;

            DIRECTX_TOOLKIT_API PBREffect(
                _In_ ID3D12Device* device,
                uint32_t effectFlags,
                const EffectPipelineStateDescription& pipelineDescription,
                bool skinningEnabled);

            // Unsupported interface methods.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetAmbientLightColor(FXMVECTOR value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetLightSpecularColor(int whichLight, FXMVECTOR value) override;
        };

        class DIRECTX_TOOLKIT_API SkinnedPBREffect : public PBREffect, public IEffectSkinning
        {
        public:
            SkinnedPBREffect(
                _In_ ID3D12Device* device,
                uint32_t effectFlags,
                const EffectPipelineStateDescription& pipelineDescription) :
                PBREffect(device, effectFlags, pipelineDescription, true)
            {
            }

            SkinnedPBREffect(SkinnedPBREffect&&) = default;
            SkinnedPBREffect& operator= (SkinnedPBREffect&&) = default;

            SkinnedPBREffect(SkinnedPBREffect const&) = delete;
            SkinnedPBREffect& operator= (SkinnedPBREffect const&) = delete;

            ~SkinnedPBREffect() override;

            // Animation settings.
            void __cdecl SetBoneTransforms(_In_reads_(count) XMMATRIX const* value, size_t count) override;
            void __cdecl ResetBoneTransforms() override;
        };


        //------------------------------------------------------------------------------
        // Built-in shader for debug visualization of normals, tangents, etc.
        class DebugEffect : public IEffect, public IEffectMatrices
        {
        public:
            enum Mode : uint32_t
            {
                Mode_Default = 0,   // Hemispherical ambient lighting
                Mode_Normals,       // RGB normals
                Mode_Tangents,      // RGB tangents
                Mode_BiTangents,    // RGB bi-tangents
            };

            DIRECTX_TOOLKIT_API DebugEffect(
                _In_ ID3D12Device* device,
                uint32_t effectFlags,
                const EffectPipelineStateDescription& pipelineDescription,
                Mode debugMode = Mode_Default);

            DIRECTX_TOOLKIT_API DebugEffect(DebugEffect&&) noexcept;
            DIRECTX_TOOLKIT_API DebugEffect& operator= (DebugEffect&&) noexcept;

            DebugEffect(DebugEffect const&) = delete;
            DebugEffect& operator= (DebugEffect const&) = delete;

            DIRECTX_TOOLKIT_API ~DebugEffect() override;

            // IEffect methods.
            DIRECTX_TOOLKIT_API void __cdecl Apply(_In_ ID3D12GraphicsCommandList* commandList) override;

            // Camera settings.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetWorld(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetView(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetProjection(FXMMATRIX value) override;
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection) override;

            // Debug Settings.
            DIRECTX_TOOLKIT_API void XM_CALLCONV SetHemisphericalAmbientColor(FXMVECTOR upper, FXMVECTOR lower);
            DIRECTX_TOOLKIT_API void __cdecl SetAlpha(float value);

        private:
            // Private implementation.
            class Impl;

            std::unique_ptr<Impl> pImpl;
        };


        //------------------------------------------------------------------------------
        // Abstract interface to factory texture resources
        class DIRECTX_TOOLKIT_API IEffectTextureFactory
        {
        public:
            virtual ~IEffectTextureFactory() = default;

            IEffectTextureFactory(const IEffectTextureFactory&) = delete;
            IEffectTextureFactory& operator=(const IEffectTextureFactory&) = delete;

            virtual size_t __cdecl CreateTexture(_In_z_ const wchar_t* name, int descriptorIndex) = 0;

        protected:
            IEffectTextureFactory() = default;
            IEffectTextureFactory(IEffectTextureFactory&&) = default;
            IEffectTextureFactory& operator=(IEffectTextureFactory&&) = default;
        };


        // Factory for sharing texture resources
        class EffectTextureFactory : public IEffectTextureFactory
        {
        public:
            DIRECTX_TOOLKIT_API EffectTextureFactory(
                _In_ ID3D12Device* device,
                ResourceUploadBatch& resourceUploadBatch,
                _In_ ID3D12DescriptorHeap* descriptorHeap) noexcept(false);

            DIRECTX_TOOLKIT_API EffectTextureFactory(
                _In_ ID3D12Device* device,
                ResourceUploadBatch& resourceUploadBatch,
                _In_ size_t numDescriptors,
                _In_ D3D12_DESCRIPTOR_HEAP_FLAGS descriptorHeapFlags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) noexcept(false);

            DIRECTX_TOOLKIT_API EffectTextureFactory(EffectTextureFactory&&) noexcept;
            DIRECTX_TOOLKIT_API EffectTextureFactory& operator= (EffectTextureFactory&&) noexcept;

            EffectTextureFactory(EffectTextureFactory const&) = delete;
            EffectTextureFactory& operator= (EffectTextureFactory const&) = delete;

            DIRECTX_TOOLKIT_API ~EffectTextureFactory() override;

            DIRECTX_TOOLKIT_API size_t __cdecl CreateTexture(_In_z_ const wchar_t* name, int descriptorIndex) override;

            DIRECTX_TOOLKIT_API ID3D12DescriptorHeap* __cdecl Heap() const noexcept;

            // Shorthand accessors for the descriptor heap
            DIRECTX_TOOLKIT_API D3D12_CPU_DESCRIPTOR_HANDLE __cdecl GetCpuDescriptorHandle(size_t index) const;
            DIRECTX_TOOLKIT_API D3D12_GPU_DESCRIPTOR_HANDLE __cdecl GetGpuDescriptorHandle(size_t index) const;

            // How many textures are there in this factory?
            DIRECTX_TOOLKIT_API size_t __cdecl ResourceCount() const noexcept;

            // Get a resource in a specific slot (note: increases reference count on resource)
            DIRECTX_TOOLKIT_API void __cdecl GetResource(size_t slot, _Out_ ID3D12Resource** resource, _Out_opt_ bool* isCubeMap = nullptr);

            // Settings.
            DIRECTX_TOOLKIT_API void __cdecl ReleaseCache();

            DIRECTX_TOOLKIT_API void __cdecl SetSharing(bool enabled) noexcept;

            DIRECTX_TOOLKIT_API void __cdecl EnableForceSRGB(bool forceSRGB) noexcept;
            DIRECTX_TOOLKIT_API void __cdecl EnableAutoGenMips(bool generateMips) noexcept;

            DIRECTX_TOOLKIT_API void __cdecl SetDirectory(_In_opt_z_ const wchar_t* path) noexcept;

        private:
            // Private implementation
            class Impl;

            std::unique_ptr<Impl> pImpl;
        };


        //------------------------------------------------------------------------------
        // Abstract interface to factory for sharing effects
        class DIRECTX_TOOLKIT_API IEffectFactory
        {
        public:
            virtual ~IEffectFactory() = default;

            IEffectFactory(const IEffectFactory&) = delete;
            IEffectFactory& operator=(const IEffectFactory&) = delete;

            struct DIRECTX_TOOLKIT_API EffectInfo
            {
                std::wstring        name;
                bool                perVertexColor;
                bool                enableSkinning;
                bool                enableDualTexture;
                bool                enableNormalMaps;
                bool                biasedVertexNormals;
                float               specularPower;
                float               alphaValue;
                XMFLOAT3            ambientColor;
                XMFLOAT3            diffuseColor;
                XMFLOAT3            specularColor;
                XMFLOAT3            emissiveColor;
                int                 diffuseTextureIndex;
                int                 specularTextureIndex;
                int                 normalTextureIndex;
                int                 emissiveTextureIndex;
                int                 samplerIndex;
                int                 samplerIndex2;

                EffectInfo() noexcept
                    : perVertexColor(false)
                    , enableSkinning(false)
                    , enableDualTexture(false)
                    , enableNormalMaps(false)
                    , biasedVertexNormals(false)
                    , specularPower(0)
                    , alphaValue(0)
                    , ambientColor(0, 0, 0)
                    , diffuseColor(0, 0, 0)
                    , specularColor(0, 0, 0)
                    , emissiveColor(0, 0, 0)
                    , diffuseTextureIndex(-1)
                    , specularTextureIndex(-1)
                    , normalTextureIndex(-1)
                    , emissiveTextureIndex(-1)
                    , samplerIndex(-1)
                    , samplerIndex2(-1)
                {
                }
            };

            virtual std::shared_ptr<IEffect> __cdecl CreateEffect(
                const EffectInfo& info,
                const EffectPipelineStateDescription& opaquePipelineState,
                const EffectPipelineStateDescription& alphaPipelineState,
                const D3D12_INPUT_LAYOUT_DESC& inputLayout,
                int textureDescriptorOffset = 0,
                int samplerDescriptorOffset = 0) = 0;

        protected:
            IEffectFactory() = default;
            IEffectFactory(IEffectFactory&&) = default;
            IEffectFactory& operator=(IEffectFactory&&) = default;
        };


        // Factory for sharing effects
        class EffectFactory : public IEffectFactory
        {
        public:
            DIRECTX_TOOLKIT_API EffectFactory(_In_ ID3D12Device* device);
            DIRECTX_TOOLKIT_API EffectFactory(
                _In_ ID3D12DescriptorHeap* textureDescriptors,
                _In_ ID3D12DescriptorHeap* samplerDescriptors);

            DIRECTX_TOOLKIT_API EffectFactory(EffectFactory&&) noexcept;
            DIRECTX_TOOLKIT_API EffectFactory& operator= (EffectFactory&&) noexcept;

            EffectFactory(EffectFactory const&) = delete;
            EffectFactory& operator= (EffectFactory const&) = delete;

            DIRECTX_TOOLKIT_API ~EffectFactory() override;

            // IEffectFactory methods.
            DIRECTX_TOOLKIT_API virtual std::shared_ptr<IEffect> __cdecl CreateEffect(
                const EffectInfo& info,
                const EffectPipelineStateDescription& opaquePipelineState,
                const EffectPipelineStateDescription& alphaPipelineState,
                const D3D12_INPUT_LAYOUT_DESC& inputLayout,
                int textureDescriptorOffset = 0,
                int samplerDescriptorOffset = 0) override;

            // Settings.
            DIRECTX_TOOLKIT_API void __cdecl ReleaseCache();

            DIRECTX_TOOLKIT_API void __cdecl SetSharing(bool enabled) noexcept;

            DIRECTX_TOOLKIT_API void __cdecl EnableLighting(bool enabled) noexcept;

            DIRECTX_TOOLKIT_API void __cdecl EnablePerPixelLighting(bool enabled) noexcept;

            DIRECTX_TOOLKIT_API void __cdecl EnableNormalMapEffect(bool enabled) noexcept;

            DIRECTX_TOOLKIT_API void __cdecl EnableFogging(bool enabled) noexcept;

            DIRECTX_TOOLKIT_API void __cdecl EnableInstancing(bool enabled) noexcept;

        private:
            // Private implementation.
            class Impl;

            std::shared_ptr<Impl> pImpl;
        };


        // Factory for Physically Based Rendering (PBR)
        class PBREffectFactory : public IEffectFactory
        {
        public:
            DIRECTX_TOOLKIT_API PBREffectFactory(_In_ ID3D12Device* device) noexcept(false);
            DIRECTX_TOOLKIT_API PBREffectFactory(
                _In_ ID3D12DescriptorHeap* textureDescriptors,
                _In_ ID3D12DescriptorHeap* samplerDescriptors) noexcept(false);

            DIRECTX_TOOLKIT_API PBREffectFactory(PBREffectFactory&&) noexcept;
            DIRECTX_TOOLKIT_API PBREffectFactory& operator= (PBREffectFactory&&) noexcept;

            PBREffectFactory(PBREffectFactory const&) = delete;
            PBREffectFactory& operator= (PBREffectFactory const&) = delete;

            DIRECTX_TOOLKIT_API ~PBREffectFactory() override;

            // IEffectFactory methods.
            DIRECTX_TOOLKIT_API virtual std::shared_ptr<IEffect> __cdecl CreateEffect(
                const EffectInfo& info,
                const EffectPipelineStateDescription& opaquePipelineState,
                const EffectPipelineStateDescription& alphaPipelineState,
                const D3D12_INPUT_LAYOUT_DESC& inputLayout,
                int textureDescriptorOffset = 0,
                int samplerDescriptorOffset = 0) override;

            // Settings.
            DIRECTX_TOOLKIT_API void __cdecl ReleaseCache();

            DIRECTX_TOOLKIT_API void __cdecl SetSharing(bool enabled) noexcept;

            DIRECTX_TOOLKIT_API void __cdecl EnableInstancing(bool enabled) noexcept;

        private:
            // Private implementation.
            class Impl;

            std::shared_ptr<Impl> pImpl;
        };
    }
}

#if defined(DIRECTX_TOOLKIT_IMPORT) && defined(_MSC_VER)
#pragma warning(pop)
#endif

#endif // __DIRECTXTK_EFFECTS_H__
