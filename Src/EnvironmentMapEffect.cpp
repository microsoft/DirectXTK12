//--------------------------------------------------------------------------------------
// File: EnvironmentMapEffect.cpp
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

#include "pch.h"
#include "EffectCommon.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;


// Constant buffer layout. Must match the shader!
struct EnvironmentMapEffectConstants
{
    XMVECTOR environmentMapSpecular;
    float environmentMapAmount;
    float fresnelFactor;
    float pad[2];

    XMVECTOR diffuseColor;
    XMVECTOR emissiveColor;
    
    XMVECTOR lightDirection[IEffectLights::MaxDirectionalLights];
    XMVECTOR lightDiffuseColor[IEffectLights::MaxDirectionalLights];

    XMVECTOR eyePosition;

    XMVECTOR fogColor;
    XMVECTOR fogVector;

    XMMATRIX world;
    XMVECTOR worldInverseTranspose[3];
    XMMATRIX worldViewProj;
};

static_assert( ( sizeof(EnvironmentMapEffectConstants) % 16 ) == 0, "CB size not padded correctly" );


// Traits type describes our characteristics to the EffectBase template.
struct EnvironmentMapEffectTraits
{
    typedef EnvironmentMapEffectConstants ConstantBufferType;

    static const int VertexShaderCount = 4;
    static const int PixelShaderCount = 4;
    static const int ShaderPermutationCount = 16;
};


// Internal EnvironmentMapEffect implementation class.
class EnvironmentMapEffect::Impl : public EffectBase<EnvironmentMapEffectTraits>
{
public:
    Impl(_In_ ID3D12Device* device,
         int effectFlags,
         const EffectPipelineStateDescription& pipelineDescription,
         bool fresnelEnabled,
         bool specularEnabled);

    enum RootParameterIndex
    {
        TextureSRV,
        CubemapSRV,
        ConstantBuffer,
        RootParameterCount
    };

    EffectLights lights;

    D3D12_GPU_DESCRIPTOR_HANDLE texture;
    D3D12_GPU_DESCRIPTOR_HANDLE environmentMap;

    int GetCurrentShaderPermutation(bool fresnelEnabled, bool specularEnabled) const;

    void Apply(_In_ ID3D12GraphicsCommandList* commandList);
};


// Include the precompiled shader code.
namespace
{
#if defined(_XBOX_ONE) && defined(_TITLE)
    #include "Shaders/Compiled/XboxOneEnvironmentMapEffect_VSEnvMap.inc"
    #include "Shaders/Compiled/XboxOneEnvironmentMapEffect_VSEnvMapFresnel.inc"
    #include "Shaders/Compiled/XboxOneEnvironmentMapEffect_VSEnvMapOneLight.inc"
    #include "Shaders/Compiled/XboxOneEnvironmentMapEffect_VSEnvMapOneLightFresnel.inc"

    #include "Shaders/Compiled/XboxOneEnvironmentMapEffect_PSEnvMap.inc"
    #include "Shaders/Compiled/XboxOneEnvironmentMapEffect_PSEnvMapNoFog.inc"
    #include "Shaders/Compiled/XboxOneEnvironmentMapEffect_PSEnvMapSpecular.inc"
    #include "Shaders/Compiled/XboxOneEnvironmentMapEffect_PSEnvMapSpecularNoFog.inc"
#else
    #include "Shaders/Compiled/EnvironmentMapEffect_VSEnvMap.inc"
    #include "Shaders/Compiled/EnvironmentMapEffect_VSEnvMapFresnel.inc"
    #include "Shaders/Compiled/EnvironmentMapEffect_VSEnvMapOneLight.inc"
    #include "Shaders/Compiled/EnvironmentMapEffect_VSEnvMapOneLightFresnel.inc"

    #include "Shaders/Compiled/EnvironmentMapEffect_PSEnvMap.inc"
    #include "Shaders/Compiled/EnvironmentMapEffect_PSEnvMapNoFog.inc"
    #include "Shaders/Compiled/EnvironmentMapEffect_PSEnvMapSpecular.inc"
    #include "Shaders/Compiled/EnvironmentMapEffect_PSEnvMapSpecularNoFog.inc"
#endif
}


const D3D12_SHADER_BYTECODE EffectBase<EnvironmentMapEffectTraits>::VertexShaderBytecode[] =
{
    { EnvironmentMapEffect_VSEnvMap,                sizeof(EnvironmentMapEffect_VSEnvMap)                },
    { EnvironmentMapEffect_VSEnvMapFresnel,         sizeof(EnvironmentMapEffect_VSEnvMapFresnel)         },
    { EnvironmentMapEffect_VSEnvMapOneLight,        sizeof(EnvironmentMapEffect_VSEnvMapOneLight)        },
    { EnvironmentMapEffect_VSEnvMapOneLightFresnel, sizeof(EnvironmentMapEffect_VSEnvMapOneLightFresnel) },
};


const int EffectBase<EnvironmentMapEffectTraits>::VertexShaderIndices[] =
{
    0,      // basic
    0,      // basic, no fog
    1,      // fresnel
    1,      // fresnel, no fog
    0,      // specular
    0,      // specular, no fog
    1,      // fresnel + specular
    1,      // fresnel + specular, no fog

    2,      // one light
    2,      // one light, no fog
    3,      // one light, fresnel
    3,      // one light, fresnel, no fog
    2,      // one light, specular
    2,      // one light, specular, no fog
    3,      // one light, fresnel + specular
    3,      // one light, fresnel + specular, no fog

};


const D3D12_SHADER_BYTECODE EffectBase<EnvironmentMapEffectTraits>::PixelShaderBytecode[] =
{
    { EnvironmentMapEffect_PSEnvMap,              sizeof(EnvironmentMapEffect_PSEnvMap)              },
    { EnvironmentMapEffect_PSEnvMapNoFog,         sizeof(EnvironmentMapEffect_PSEnvMapNoFog)         },
    { EnvironmentMapEffect_PSEnvMapSpecular,      sizeof(EnvironmentMapEffect_PSEnvMapSpecular)      },
    { EnvironmentMapEffect_PSEnvMapSpecularNoFog, sizeof(EnvironmentMapEffect_PSEnvMapSpecularNoFog) },
};


const int EffectBase<EnvironmentMapEffectTraits>::PixelShaderIndices[] =
{
    0,      // basic
    1,      // basic, no fog
    0,      // fresnel
    1,      // fresnel, no fog
    2,      // specular
    3,      // specular, no fog
    2,      // fresnel + specular
    3,      // fresnel + specular, no fog

    0,      // one light
    1,      // one light, no fog
    0,      // one light, fresnel
    1,      // one light, fresnel, no fog
    2,      // one light, specular
    3,      // one light, specular, no fog
    2,      // one light, fresnel + specular
    3,      // one light, fresnel + specular, no fog
};


// Global pool of per-device EnvironmentMapEffect resources.
SharedResourcePool<ID3D12Device*, EffectBase<EnvironmentMapEffectTraits>::DeviceResources> EffectBase<EnvironmentMapEffectTraits>::deviceResourcesPool;


// Constructor.
EnvironmentMapEffect::Impl::Impl(    
    _In_ ID3D12Device* device, 
    int effectFlags, 
    const EffectPipelineStateDescription& pipelineDescription, 
    bool fresnelEnabled, 
    bool specularEnabled)
  : EffectBase(device)
{
    static_assert( _countof(EffectBase<EnvironmentMapEffectTraits>::VertexShaderIndices) == EnvironmentMapEffectTraits::ShaderPermutationCount, "array/max mismatch" );
    static_assert( _countof(EffectBase<EnvironmentMapEffectTraits>::VertexShaderBytecode) == EnvironmentMapEffectTraits::VertexShaderCount, "array/max mismatch" );
    static_assert( _countof(EffectBase<EnvironmentMapEffectTraits>::PixelShaderBytecode) == EnvironmentMapEffectTraits::PixelShaderCount, "array/max mismatch" );
    static_assert( _countof(EffectBase<EnvironmentMapEffectTraits>::PixelShaderIndices) == EnvironmentMapEffectTraits::ShaderPermutationCount, "array/max mismatch" );

    // Create root signature
    {
        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | // Only the input assembler stage needs access to the constant buffer.
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;

        CD3DX12_STATIC_SAMPLER_DESC samplers[2];
        samplers[0] = CD3DX12_STATIC_SAMPLER_DESC(0);
        samplers[1] = CD3DX12_STATIC_SAMPLER_DESC(1);

        CD3DX12_ROOT_PARAMETER rootParameters[RootParameterIndex::RootParameterCount];
        rootParameters[RootParameterIndex::ConstantBuffer].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);

        // Texture 1
        CD3DX12_DESCRIPTOR_RANGE descriptorRange1;
        descriptorRange1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
        rootParameters[RootParameterIndex::TextureSRV].InitAsDescriptorTable(1, &descriptorRange1);

        // Texture 2
        CD3DX12_DESCRIPTOR_RANGE descriptorRange2;
        descriptorRange2.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
        rootParameters[RootParameterIndex::CubemapSRV].InitAsDescriptorTable(1, &descriptorRange2);

        // Create the root signature
        CD3DX12_ROOT_SIGNATURE_DESC rsigDesc;
        rsigDesc.Init(_countof(rootParameters), rootParameters, _countof(samplers), samplers, rootSignatureFlags);

        ThrowIfFailed(CreateRootSignature(device, &rsigDesc, mRootSignature.ReleaseAndGetAddressOf()));
    }

    fog.enabled = (effectFlags & EffectFlags::Fog) != 0;

    constants.environmentMapAmount = 1;
    constants.fresnelFactor = 1;

    XMVECTOR unwantedOutput[MaxDirectionalLights];

    lights.InitializeConstants(unwantedOutput[0], constants.lightDirection, constants.lightDiffuseColor, unwantedOutput);

    {   // Create pipeline state
        int sp = GetCurrentShaderPermutation(fresnelEnabled, specularEnabled);
        int vi = EffectBase<EnvironmentMapEffectTraits>::VertexShaderIndices[sp];
        int pi = EffectBase<EnvironmentMapEffectTraits>::PixelShaderIndices[sp];

        EffectBase::CreatePipelineState(
            mRootSignature.Get(),
            pipelineDescription.inputLayout,
            &EffectBase<EnvironmentMapEffectTraits>::VertexShaderBytecode[vi],
            &EffectBase<EnvironmentMapEffectTraits>::PixelShaderBytecode[pi],
            pipelineDescription.blendDesc,
            pipelineDescription.depthStencilDesc,
            pipelineDescription.rasterizerDesc,
            pipelineDescription.renderTargetState,
            pipelineDescription.primitiveTopology,
            pipelineDescription.stripCutValue);
    }
}


int EnvironmentMapEffect::Impl::GetCurrentShaderPermutation(bool fresnelEnabled, bool specularEnabled) const
{
    int permutation = 0;

    // Use optimized shaders if fog is disabled.
    if (!fog.enabled)
    {
        permutation += 1;
    }

    // Support fresnel or specular?
    if (fresnelEnabled)
    {
        permutation += 2;
    }

    if (specularEnabled)
    {
        permutation += 4;
    }

    // Use the only-bother-with-the-first-light shader optimization?
    if (!lights.lightEnabled[1] && !lights.lightEnabled[2])
    {
        permutation += 8;
    }

    return permutation;
}


// Sets our state onto the D3D device.
void EnvironmentMapEffect::Impl::Apply(_In_ ID3D12GraphicsCommandList* commandList)
{
    // Compute derived parameter values.
    matrices.SetConstants(dirtyFlags, constants.worldViewProj);

    fog.SetConstants(dirtyFlags, matrices.worldView, constants.fogVector);
            
    lights.SetConstants(dirtyFlags, matrices, constants.world, constants.worldInverseTranspose, constants.eyePosition, constants.diffuseColor, constants.emissiveColor, true);

    UpdateConstants();

    // Set the resources and state
    commandList->SetGraphicsRootSignature(mRootSignature.Get());
    commandList->SetGraphicsRootDescriptorTable(RootParameterIndex::TextureSRV, texture);
    commandList->SetGraphicsRootDescriptorTable(RootParameterIndex::CubemapSRV, environmentMap);
    commandList->SetGraphicsRootConstantBufferView(RootParameterIndex::ConstantBuffer, GetConstantBufferGpuAddress());
    commandList->SetPipelineState(EffectBase::mPipelineState.Get());
}


// Public constructor.
EnvironmentMapEffect::EnvironmentMapEffect(
    _In_ ID3D12Device* device, 
    int effectFlags, 
    const EffectPipelineStateDescription& pipelineDescription, 
    bool fresnelEnabled, 
    bool specularEnabled)
  : pImpl(new Impl(device, effectFlags, pipelineDescription, fresnelEnabled, specularEnabled))
{
}


// Move constructor.
EnvironmentMapEffect::EnvironmentMapEffect(EnvironmentMapEffect&& moveFrom)
  : pImpl(std::move(moveFrom.pImpl))
{
}


// Move assignment.
EnvironmentMapEffect& EnvironmentMapEffect::operator= (EnvironmentMapEffect&& moveFrom)
{
    pImpl = std::move(moveFrom.pImpl);
    return *this;
}


// Public destructor.
EnvironmentMapEffect::~EnvironmentMapEffect()
{
}


void EnvironmentMapEffect::Apply(
    _In_ ID3D12GraphicsCommandList* cmdList)
{
    pImpl->Apply(cmdList);
}


void XM_CALLCONV EnvironmentMapEffect::SetWorld(FXMMATRIX value)
{
    pImpl->matrices.world = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj | EffectDirtyFlags::WorldInverseTranspose | EffectDirtyFlags::FogVector;
}


void XM_CALLCONV EnvironmentMapEffect::SetView(FXMMATRIX value)
{
    pImpl->matrices.view = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj | EffectDirtyFlags::EyePosition | EffectDirtyFlags::FogVector;
}


void XM_CALLCONV EnvironmentMapEffect::SetProjection(FXMMATRIX value)
{
    pImpl->matrices.projection = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj;
}


void XM_CALLCONV EnvironmentMapEffect::SetDiffuseColor(FXMVECTOR value)
{
    pImpl->lights.diffuseColor = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::MaterialColor;
}


void XM_CALLCONV EnvironmentMapEffect::SetEmissiveColor(FXMVECTOR value)
{
    pImpl->lights.emissiveColor = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::MaterialColor;
}


void EnvironmentMapEffect::SetAlpha(float value)
{
    pImpl->lights.alpha = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::MaterialColor;
}


void XM_CALLCONV EnvironmentMapEffect::SetAmbientLightColor(FXMVECTOR value)
{
    pImpl->lights.ambientLightColor = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::MaterialColor;
}


void EnvironmentMapEffect::SetLightEnabled(int whichLight, bool value)
{
    XMVECTOR unwantedOutput[MaxDirectionalLights] = {};

    pImpl->dirtyFlags |= pImpl->lights.SetLightEnabled(whichLight, value, pImpl->constants.lightDiffuseColor, unwantedOutput);
}


void XM_CALLCONV EnvironmentMapEffect::SetLightDirection(int whichLight, FXMVECTOR value)
{
    EffectLights::ValidateLightIndex(whichLight);

    pImpl->constants.lightDirection[whichLight] = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void XM_CALLCONV EnvironmentMapEffect::SetLightDiffuseColor(int whichLight, FXMVECTOR value)
{
    pImpl->dirtyFlags |= pImpl->lights.SetLightDiffuseColor(whichLight, value, pImpl->constants.lightDiffuseColor);
}


void XM_CALLCONV EnvironmentMapEffect::SetLightSpecularColor(int, FXMVECTOR)
{
    // Unsupported interface method.
}


void EnvironmentMapEffect::EnableDefaultLighting()
{
    EffectLights::EnableDefaultLighting(this);
}


void EnvironmentMapEffect::SetFogStart(float value)
{
    pImpl->fog.start = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::FogVector;
}


void EnvironmentMapEffect::SetFogEnd(float value)
{
    pImpl->fog.end = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::FogVector;
}


void XM_CALLCONV EnvironmentMapEffect::SetFogColor(FXMVECTOR value)
{
    pImpl->constants.fogColor = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void EnvironmentMapEffect::SetTexture(_In_opt_ D3D12_GPU_DESCRIPTOR_HANDLE value)
{
    pImpl->texture = value;
}


void EnvironmentMapEffect::SetEnvironmentMap(_In_opt_ D3D12_GPU_DESCRIPTOR_HANDLE value)
{
    pImpl->environmentMap = value;
}


void EnvironmentMapEffect::SetEnvironmentMapAmount(float value)
{
    pImpl->constants.environmentMapAmount = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void XM_CALLCONV EnvironmentMapEffect::SetEnvironmentMapSpecular(FXMVECTOR value)
{
    pImpl->constants.environmentMapSpecular = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void EnvironmentMapEffect::SetFresnelFactor(float value)
{
    pImpl->constants.fresnelFactor = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}
