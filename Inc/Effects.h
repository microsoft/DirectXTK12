//--------------------------------------------------------------------------------------
// File: Effects.h
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
#include <memory>
#include <string>

#include "RenderTargetState.h"
#include "EffectPipelineStateDescription.h"


namespace DirectX
{
    class DescriptorHeap;
    class ResourceUploadBatch;

    //----------------------------------------------------------------------------------
    // Abstract interface representing any effect which can be applied onto a D3D device context.
    class IEffect
    {
    public:
        virtual ~IEffect() { }

        virtual void __cdecl Apply(_In_ ID3D12GraphicsCommandList* commandList) = 0;
    };


    // Abstract interface for effects with world, view, and projection matrices.
    class IEffectMatrices
    {
    public:
        virtual ~IEffectMatrices() { }

        virtual void XM_CALLCONV SetWorld(FXMMATRIX value) = 0;
        virtual void XM_CALLCONV SetView(FXMMATRIX value) = 0;
        virtual void XM_CALLCONV SetProjection(FXMMATRIX value) = 0;
        virtual void XM_CALLCONV SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection);
    };


    // Abstract interface for effects which support directional lighting.
    class IEffectLights
    {
    public:
        virtual ~IEffectLights() { }

        virtual void XM_CALLCONV SetAmbientLightColor(FXMVECTOR value) = 0;

        virtual void __cdecl SetLightEnabled(int whichLight, bool value) = 0;
        virtual void XM_CALLCONV SetLightDirection(int whichLight, FXMVECTOR value) = 0;
        virtual void XM_CALLCONV SetLightDiffuseColor(int whichLight, FXMVECTOR value) = 0;
        virtual void XM_CALLCONV SetLightSpecularColor(int whichLight, FXMVECTOR value) = 0;

        virtual void __cdecl EnableDefaultLighting() = 0;

        static const int MaxDirectionalLights = 3;
    };


    // Abstract interface for effects which support fog.
    class IEffectFog
    {
    public:
        virtual ~IEffectFog() { }

        virtual void __cdecl SetFogStart(float value) = 0;
        virtual void __cdecl SetFogEnd(float value) = 0;
        virtual void XM_CALLCONV SetFogColor(FXMVECTOR value) = 0;
    };


    // Abstract interface for effects which support skinning
    class IEffectSkinning
    {
    public:
        virtual ~IEffectSkinning() { } 

        virtual void __cdecl SetBoneTransforms(_In_reads_(count) XMMATRIX const* value, size_t count) = 0;
        virtual void __cdecl ResetBoneTransforms() = 0;

        static const int MaxBones = 72;
    };


    //----------------------------------------------------------------------------------
    namespace EffectFlags
    {
        const int None                = 0x00;
        const int Fog                 = 0x01;
        const int Lighting            = 0x02;
        const int PerPixelLighting    = 0x04 | Lighting; // per pixel lighting implies lighting enabled
        const int VertexColor         = 0x08;
        const int Texture             = 0x10;

        const int BiasedVertexNormals = 0x10000; // compressed vertex normals need x2 bias
    }


    //----------------------------------------------------------------------------------
    // Built-in shader supports optional texture mapping, vertex coloring, directional lighting, and fog.
    class BasicEffect : public IEffect, public IEffectMatrices, public IEffectLights, public IEffectFog
    {
    public:
        BasicEffect( _In_ ID3D12Device* device, int effectFlags, const EffectPipelineStateDescription& pipelineDescription );
        BasicEffect(BasicEffect&& moveFrom);
        BasicEffect& operator= (BasicEffect&& moveFrom);

        BasicEffect(BasicEffect const&) = delete;
        BasicEffect& operator= (BasicEffect const&) = delete;

        virtual ~BasicEffect();

        // IEffect methods.
        void __cdecl Apply(_In_ ID3D12GraphicsCommandList* commandList) override;

        // Camera settings.
        void XM_CALLCONV SetWorld(FXMMATRIX value) override;
        void XM_CALLCONV SetView(FXMMATRIX value) override;
        void XM_CALLCONV SetProjection(FXMMATRIX value) override;
        void XM_CALLCONV SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection) override;

        // Material settings.
        void XM_CALLCONV SetDiffuseColor(FXMVECTOR value);
        void XM_CALLCONV SetEmissiveColor(FXMVECTOR value);
        void XM_CALLCONV SetSpecularColor(FXMVECTOR value);
        void __cdecl SetSpecularPower(float value);
        void __cdecl DisableSpecular();
        void __cdecl SetAlpha(float value);
        void XM_CALLCONV SetColorAndAlpha(FXMVECTOR value);
        
        // Light settings.
        void XM_CALLCONV SetAmbientLightColor(FXMVECTOR value) override;

        void __cdecl SetLightEnabled(int whichLight, bool value) override;
        void XM_CALLCONV SetLightDirection(int whichLight, FXMVECTOR value) override;
        void XM_CALLCONV SetLightDiffuseColor(int whichLight, FXMVECTOR value) override;
        void XM_CALLCONV SetLightSpecularColor(int whichLight, FXMVECTOR value) override;

        void __cdecl EnableDefaultLighting() override;

        // Fog settings.
        void __cdecl SetFogStart(float value) override;
        void __cdecl SetFogEnd(float value) override;
        void XM_CALLCONV SetFogColor(FXMVECTOR value) override;

        // Texture setting.
        void __cdecl SetTexture(D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor, D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptor);

    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;
    };


    // Built-in shader supports per-pixel alpha testing.
    class AlphaTestEffect : public IEffect, public IEffectMatrices, public IEffectFog
    {
    public:
        AlphaTestEffect(_In_ ID3D12Device* device, int effectFlags, const EffectPipelineStateDescription& pipelineDescription,
                        D3D12_COMPARISON_FUNC alphaFunction = D3D12_COMPARISON_FUNC_GREATER);
        AlphaTestEffect(AlphaTestEffect&& moveFrom);
        AlphaTestEffect& operator= (AlphaTestEffect&& moveFrom);

        AlphaTestEffect(AlphaTestEffect const&) = delete;
        AlphaTestEffect& operator= (AlphaTestEffect const&) = delete;

        virtual ~AlphaTestEffect();

        // IEffect methods.
        void __cdecl Apply(_In_ ID3D12GraphicsCommandList* commandList) override;

        // Camera settings.
        void XM_CALLCONV SetWorld(FXMMATRIX value) override;
        void XM_CALLCONV SetView(FXMMATRIX value) override;
        void XM_CALLCONV SetProjection(FXMMATRIX value) override;
        void XM_CALLCONV SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection) override;

        // Material settings.
        void XM_CALLCONV SetDiffuseColor(FXMVECTOR value);
        void __cdecl SetAlpha(float value);
        void XM_CALLCONV SetColorAndAlpha(FXMVECTOR value);
        
        // Fog settings.
        void __cdecl SetFogStart(float value) override;
        void __cdecl SetFogEnd(float value) override;
        void XM_CALLCONV SetFogColor(FXMVECTOR value) override;

        // Texture setting.
        void __cdecl SetTexture(D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor, D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptor);

        // Alpha test settings.
        void __cdecl SetReferenceAlpha(int value);

    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;
    };


    // Built-in shader supports two layer multitexturing (eg. for lightmaps or detail textures).
    class DualTextureEffect : public IEffect, public IEffectMatrices, public IEffectFog
    {
    public:
        DualTextureEffect(_In_ ID3D12Device* device, int effectFlags, const EffectPipelineStateDescription& pipelineDescription);
        DualTextureEffect(DualTextureEffect&& moveFrom);
        DualTextureEffect& operator= (DualTextureEffect&& moveFrom);

        DualTextureEffect(DualTextureEffect const&) = delete;
        DualTextureEffect& operator= (DualTextureEffect const&) = delete;

        ~DualTextureEffect();

        // IEffect methods.
        void __cdecl Apply(_In_ ID3D12GraphicsCommandList* commandList) override;

        // Camera settings.
        void XM_CALLCONV SetWorld(FXMMATRIX value) override;
        void XM_CALLCONV SetView(FXMMATRIX value) override;
        void XM_CALLCONV SetProjection(FXMMATRIX value) override;
        void XM_CALLCONV SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection) override;

        // Material settings.
        void XM_CALLCONV SetDiffuseColor(FXMVECTOR value);
        void __cdecl SetAlpha(float value);
        void XM_CALLCONV SetColorAndAlpha(FXMVECTOR value);
        
        // Fog settings.
        void __cdecl SetFogStart(float value) override;
        void __cdecl SetFogEnd(float value) override;
        void XM_CALLCONV SetFogColor(FXMVECTOR value) override;

        // Texture settings.
        void __cdecl SetTexture(D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor, D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptor);
        void __cdecl SetTexture2(D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor, D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptor);
        
    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;
    };


    // Built-in shader supports cubic environment mapping.
    class EnvironmentMapEffect : public IEffect, public IEffectMatrices, public IEffectLights, public IEffectFog
    {
    public:
        EnvironmentMapEffect(_In_ ID3D12Device* device, int effectFlags, const EffectPipelineStateDescription& pipelineDescription, bool fresnelEnabled = true, bool specularEnabled = false);
        EnvironmentMapEffect(EnvironmentMapEffect&& moveFrom);
        EnvironmentMapEffect& operator= (EnvironmentMapEffect&& moveFrom);

        EnvironmentMapEffect(EnvironmentMapEffect const&) = delete;
        EnvironmentMapEffect& operator= (EnvironmentMapEffect const&) = delete;

        virtual ~EnvironmentMapEffect();

        // IEffect methods.
        void __cdecl Apply(_In_ ID3D12GraphicsCommandList* commandList) override;

        // Camera settings.
        void XM_CALLCONV SetWorld(FXMMATRIX value) override;
        void XM_CALLCONV SetView(FXMMATRIX value) override;
        void XM_CALLCONV SetProjection(FXMMATRIX value) override;
        void XM_CALLCONV SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection) override;

        // Material settings.
        void XM_CALLCONV SetDiffuseColor(FXMVECTOR value);
        void XM_CALLCONV SetEmissiveColor(FXMVECTOR value);
        void __cdecl SetAlpha(float value);
        void XM_CALLCONV SetColorAndAlpha(FXMVECTOR value);
        
        // Light settings.
        void XM_CALLCONV SetAmbientLightColor(FXMVECTOR value) override;

        void __cdecl SetLightEnabled(int whichLight, bool value) override;
        void XM_CALLCONV SetLightDirection(int whichLight, FXMVECTOR value) override;
        void XM_CALLCONV SetLightDiffuseColor(int whichLight, FXMVECTOR value) override;

        void __cdecl EnableDefaultLighting() override;

        // Fog settings.
        void __cdecl SetFogStart(float value) override;
        void __cdecl SetFogEnd(float value) override;
        void XM_CALLCONV SetFogColor(FXMVECTOR value) override;

        // Texture setting.
        void __cdecl SetTexture(D3D12_GPU_DESCRIPTOR_HANDLE texture, D3D12_GPU_DESCRIPTOR_HANDLE sampler);

        // Environment map settings.
        void __cdecl SetEnvironmentMap(D3D12_GPU_DESCRIPTOR_HANDLE texture, D3D12_GPU_DESCRIPTOR_HANDLE sampler);
        void __cdecl SetEnvironmentMapAmount(float value);
        void XM_CALLCONV SetEnvironmentMapSpecular(FXMVECTOR value);
        void __cdecl SetFresnelFactor(float value);
        
    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;

        // Unsupported interface methods.
        void XM_CALLCONV SetLightSpecularColor(int whichLight, FXMVECTOR value) override;
    };


    // Built-in shader supports skinned animation.
    class SkinnedEffect : public IEffect, public IEffectMatrices, public IEffectLights, public IEffectFog, public IEffectSkinning
    {
    public:
        SkinnedEffect(_In_ ID3D12Device* device, int effectFlags, const EffectPipelineStateDescription& pipelineDescription, int weightsPerVertex = 4);
        SkinnedEffect(SkinnedEffect&& moveFrom);
        SkinnedEffect& operator= (SkinnedEffect&& moveFrom);

        SkinnedEffect(SkinnedEffect const&) = delete;
        SkinnedEffect& operator= (SkinnedEffect const&) = delete;

        virtual ~SkinnedEffect();

        // IEffect methods.
        void __cdecl Apply(_In_ ID3D12GraphicsCommandList* commandList) override;

        // Camera settings.
        void XM_CALLCONV SetWorld(FXMMATRIX value) override;
        void XM_CALLCONV SetView(FXMMATRIX value) override;
        void XM_CALLCONV SetProjection(FXMMATRIX value) override;
        void XM_CALLCONV SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection) override;

        // Material settings.
        void XM_CALLCONV SetDiffuseColor(FXMVECTOR value);
        void XM_CALLCONV SetEmissiveColor(FXMVECTOR value);
        void XM_CALLCONV SetSpecularColor(FXMVECTOR value);
        void __cdecl SetSpecularPower(float value);
        void __cdecl DisableSpecular();
        void __cdecl SetAlpha(float value);
        void XM_CALLCONV SetColorAndAlpha(FXMVECTOR value);
        
        // Light settings.
        void XM_CALLCONV SetAmbientLightColor(FXMVECTOR value) override;

        void __cdecl SetLightEnabled(int whichLight, bool value) override;
        void XM_CALLCONV SetLightDirection(int whichLight, FXMVECTOR value) override;
        void XM_CALLCONV SetLightDiffuseColor(int whichLight, FXMVECTOR value) override;
        void XM_CALLCONV SetLightSpecularColor(int whichLight, FXMVECTOR value) override;

        void __cdecl EnableDefaultLighting() override;

        // Fog settings.
        void __cdecl SetFogStart(float value) override;
        void __cdecl SetFogEnd(float value) override;
        void XM_CALLCONV SetFogColor(FXMVECTOR value) override;

        // Texture setting.
        void __cdecl SetTexture(D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor, D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptor);
        
        // Animation settings.
        void __cdecl SetBoneTransforms(_In_reads_(count) XMMATRIX const* value, size_t count) override;
        void __cdecl ResetBoneTransforms() override;

    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;
    };


    //----------------------------------------------------------------------------------
    // Built-in shader extends BasicEffect with normal map and optional specular map
    class NormalMapEffect : public IEffect, public IEffectMatrices, public IEffectLights, public IEffectFog
    {
    public:
        NormalMapEffect(_In_ ID3D12Device* device, int effectFlags, const EffectPipelineStateDescription& pipelineDescription, bool specularMap = true);
        NormalMapEffect(NormalMapEffect&& moveFrom);
        NormalMapEffect& operator= (NormalMapEffect&& moveFrom);

        NormalMapEffect(NormalMapEffect const&) = delete;
        NormalMapEffect& operator= (NormalMapEffect const&) = delete;

        virtual ~NormalMapEffect();

        // IEffect methods.
        void __cdecl Apply(_In_ ID3D12GraphicsCommandList* commandList) override;

        // Camera settings.
        void XM_CALLCONV SetWorld(FXMMATRIX value) override;
        void XM_CALLCONV SetView(FXMMATRIX value) override;
        void XM_CALLCONV SetProjection(FXMMATRIX value) override;
        void XM_CALLCONV SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection) override;

        // Material settings.
        void XM_CALLCONV SetDiffuseColor(FXMVECTOR value);
        void XM_CALLCONV SetEmissiveColor(FXMVECTOR value);
        void XM_CALLCONV SetSpecularColor(FXMVECTOR value);
        void __cdecl SetSpecularPower(float value);
        void __cdecl DisableSpecular();
        void __cdecl SetAlpha(float value);
        void XM_CALLCONV SetColorAndAlpha(FXMVECTOR value);

        // Light settings.
        void XM_CALLCONV SetAmbientLightColor(FXMVECTOR value) override;

        void __cdecl SetLightEnabled(int whichLight, bool value) override;
        void XM_CALLCONV SetLightDirection(int whichLight, FXMVECTOR value) override;
        void XM_CALLCONV SetLightDiffuseColor(int whichLight, FXMVECTOR value) override;
        void XM_CALLCONV SetLightSpecularColor(int whichLight, FXMVECTOR value) override;

        void __cdecl EnableDefaultLighting() override;

        // Fog settings.
        void __cdecl SetFogStart(float value) override;
        void __cdecl SetFogEnd(float value) override;
        void XM_CALLCONV SetFogColor(FXMVECTOR value) override;

        // Texture setting - albedo, normal and specular intensity
        void __cdecl SetTexture(D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor, D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptor);
        void __cdecl SetNormalTexture(D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor);
        void __cdecl SetSpecularTexture(D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor);

    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;
    };


    //----------------------------------------------------------------------------------
    // Built-in shader for Physically-Based Rendering (Roughness/Metalness) with Image-based lighting
    class PBREffect : public IEffect, public IEffectMatrices, public IEffectLights
    {
    public:
        explicit PBREffect(_In_ ID3D12Device* device, int effectFlags, const EffectPipelineStateDescription& pipelineDescription, bool emissive = false, bool generateVelocity = false);
        PBREffect(PBREffect&& moveFrom);
        PBREffect& operator= (PBREffect&& moveFrom);

        PBREffect(PBREffect const&) = delete;
        PBREffect& operator= (PBREffect const&) = delete;

        virtual ~PBREffect();

        // IEffect methods.
        void __cdecl Apply(_In_ ID3D12GraphicsCommandList* commandList) override;

        // Camera settings.
        void XM_CALLCONV SetWorld(FXMMATRIX value) override;
        void XM_CALLCONV SetView(FXMMATRIX value) override;
        void XM_CALLCONV SetProjection(FXMMATRIX value) override;
        void XM_CALLCONV SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection) override;

        // Light settings.
        void __cdecl SetLightEnabled(int whichLight, bool value) override;
        void XM_CALLCONV SetLightDirection(int whichLight, FXMVECTOR value) override;
        void XM_CALLCONV SetLightDiffuseColor(int whichLight, FXMVECTOR value) override;

        void __cdecl EnableDefaultLighting() override;

        // PBR Settings.
        void __cdecl SetAlpha(float value);
        void XM_CALLCONV SetConstantAlbedo(FXMVECTOR value);
        void __cdecl SetConstantMetallic(float value);
        void __cdecl SetConstantRoughness(float value);

        // Texture settings.
        void __cdecl SetSurfaceTextures(
            D3D12_GPU_DESCRIPTOR_HANDLE albedo,
            D3D12_GPU_DESCRIPTOR_HANDLE normal,
            D3D12_GPU_DESCRIPTOR_HANDLE roughnessMetallicAmbientOcclusion,
            D3D12_GPU_DESCRIPTOR_HANDLE sampler);

        void __cdecl SetIBLTextures(
            D3D12_GPU_DESCRIPTOR_HANDLE radiance,
            int numRadianceMips,
            D3D12_GPU_DESCRIPTOR_HANDLE irradiance,
            D3D12_GPU_DESCRIPTOR_HANDLE sampler);

        void __cdecl SetEmissiveTexture(D3D12_GPU_DESCRIPTOR_HANDLE emissive);

        // Render target size, required for velocity buffer output.
        void __cdecl SetRenderTargetSizeInPixels(int width, int height);

    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;

        // Unsupported interface methods.
        void XM_CALLCONV SetAmbientLightColor(FXMVECTOR value) override;
        void XM_CALLCONV SetLightSpecularColor(int whichLight, FXMVECTOR value) override;
    };


    //----------------------------------------------------------------------------------
    // Built-in shader for debug visualization of normals, tangents, etc.
    class DebugEffect : public IEffect, public IEffectMatrices
    {
    public:
        enum Mode
        {
            Mode_Default = 0,   // Hemispherical ambient lighting
            Mode_Normals,       // RGB normals
            Mode_Tangents,      // RGB tangents
            Mode_BiTangents,    // RGB bi-tangents
        };

        explicit DebugEffect(_In_ ID3D12Device* device, int effectFlags, const EffectPipelineStateDescription& pipelineDescription, Mode debugMode = Mode_Default);
        DebugEffect(DebugEffect&& moveFrom);
        DebugEffect& operator= (DebugEffect&& moveFrom);

        DebugEffect(DebugEffect const&) = delete;
        DebugEffect& operator= (DebugEffect const&) = delete;

        virtual ~DebugEffect();

        // IEffect methods.
        void __cdecl Apply(_In_ ID3D12GraphicsCommandList* commandList) override;

        // Camera settings.
        void XM_CALLCONV SetWorld(FXMMATRIX value) override;
        void XM_CALLCONV SetView(FXMMATRIX value) override;
        void XM_CALLCONV SetProjection(FXMMATRIX value) override;
        void XM_CALLCONV SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection) override;

        // Debug Settings.
        void XM_CALLCONV SetHemisphericalAmbientColor(FXMVECTOR upper, FXMVECTOR lower);
        void __cdecl SetAlpha(float value);

    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;
    };


    //----------------------------------------------------------------------------------
    // Abstract interface to factory texture resources
    class IEffectTextureFactory
    {
    public:
        virtual ~IEffectTextureFactory() {}

        virtual void __cdecl CreateTexture(_In_z_ const wchar_t* name, int descriptorIndex) = 0;
    };


    // Factory for sharing texture resources
    class EffectTextureFactory : public IEffectTextureFactory
    {
    public:
        EffectTextureFactory(
            _In_ ID3D12Device* device,
            _Inout_ ResourceUploadBatch& resourceUploadBatch,
            _In_ ID3D12DescriptorHeap* descriptorHeap);

        EffectTextureFactory(
            _In_ ID3D12Device* device,
            _Inout_ ResourceUploadBatch& resourceUploadBatch,
            _In_ size_t numDescriptors,
            _In_ D3D12_DESCRIPTOR_HEAP_FLAGS descriptorHeapFlags);

        EffectTextureFactory(EffectTextureFactory&& moveFrom);
        EffectTextureFactory& operator= (EffectTextureFactory&& moveFrom);

        EffectTextureFactory(EffectTextureFactory const&) = delete;
        EffectTextureFactory& operator= (EffectTextureFactory const&) = delete;

        virtual ~EffectTextureFactory();

        virtual void __cdecl CreateTexture(_In_z_ const wchar_t* name, int descriptorIndex) override;

        ID3D12DescriptorHeap* __cdecl Heap() const;

        // Shorthand accessors for the descriptor heap
        D3D12_CPU_DESCRIPTOR_HANDLE __cdecl GetCpuDescriptorHandle(size_t index) const;
        D3D12_GPU_DESCRIPTOR_HANDLE __cdecl GetGpuDescriptorHandle(size_t index) const;

        // How many textures are there in this factory?
        size_t __cdecl ResourceCount() const;

        // Get a resource in a specific slot
        void __cdecl GetResource(size_t slot, _Out_ ID3D12Resource** resource, _Out_ bool* isCubeMap);

        // Settings.
        void __cdecl ReleaseCache();

        void __cdecl SetSharing( bool enabled );

        void __cdecl EnableForceSRGB( bool forceSRGB ); 
        void __cdecl EnableAutoGenMips( bool generateMips );

        void __cdecl SetDirectory(_In_opt_z_ const wchar_t* path);

    private:
        // Private implementation
        class Impl;

        std::unique_ptr<Impl> pImpl;
    };


    //----------------------------------------------------------------------------------
    // Abstract interface to factory for sharing effects
    class IEffectFactory
    {
    public:
        virtual ~IEffectFactory() {}

        struct EffectInfo
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
            int                 samplerIndex;
            int                 samplerIndex2;

            EffectInfo()
                : perVertexColor(false)
                , enableSkinning(false)
                , enableDualTexture(false)
                , enableNormalMaps(false)
                , specularPower(0)
                , alphaValue(0)
                , ambientColor(0, 0, 0)
                , diffuseColor(0, 0, 0)
                , specularColor(0, 0, 0)
                , emissiveColor(0, 0, 0)
                , diffuseTextureIndex(-1)
                , specularTextureIndex(-1)
                , normalTextureIndex(-1)
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
    };


    // Factory for sharing effects
    class EffectFactory : public IEffectFactory
    {
    public:
        EffectFactory(_In_ ID3D12Device* device);
        EffectFactory(
            _In_ ID3D12DescriptorHeap* textureDescriptors, 
            _In_ ID3D12DescriptorHeap* samplerDescriptors);

        EffectFactory(EffectFactory&& moveFrom);
        EffectFactory& operator= (EffectFactory&& moveFrom);

        EffectFactory(EffectFactory const&) = delete;
        EffectFactory& operator= (EffectFactory const&) = delete;

        virtual ~EffectFactory();

        // IEffectFactory methods.
        virtual std::shared_ptr<IEffect> __cdecl CreateEffect(
            const EffectInfo& info, 
            const EffectPipelineStateDescription& opaquePipelineState,
            const EffectPipelineStateDescription& alphaPipelineState,
            const D3D12_INPUT_LAYOUT_DESC& inputLayout,
            int textureDescriptorOffset = 0,
            int samplerDescriptorOffset = 0) override;
        
        // Settings.
        void __cdecl ReleaseCache();

        void __cdecl SetSharing( bool enabled );

        void __cdecl EnablePerPixelLighting(bool enabled);

        void __cdecl EnableNormalMapEffect(bool enabled);

        void __cdecl EnableFogging(bool enabled);

    private:
        // Private implementation.
        class Impl;

        std::shared_ptr<Impl> pImpl;
    };
}
