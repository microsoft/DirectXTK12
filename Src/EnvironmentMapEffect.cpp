//--------------------------------------------------------------------------------------
// File: EnvironmentMapEffect.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "EffectCommon.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace
{
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

    static_assert((sizeof(EnvironmentMapEffectConstants) % 16) == 0, "CB size not padded correctly");


    // Traits type describes our characteristics to the EffectBase template.
    struct EnvironmentMapEffectTraits
    {
        using ConstantBufferType = EnvironmentMapEffectConstants;

        static constexpr int VertexShaderCount = 6;
        static constexpr int PixelShaderCount = 16;
        static constexpr int ShaderPermutationCount = 40;
        static constexpr int RootSignatureCount = 1;
    };
}

// Internal EnvironmentMapEffect implementation class.
class EnvironmentMapEffect::Impl : public EffectBase<EnvironmentMapEffectTraits>
{
public:
    Impl(_In_ ID3D12Device* device,
        uint32_t effectFlags,
        const EffectPipelineStateDescription& pipelineDescription,
        EnvironmentMapEffect::Mapping mapping);

    enum RootParameterIndex
    {
        TextureSRV,
        TextureSampler,
        CubemapSRV,
        CubemapSampler,
        ConstantBuffer,
        RootParameterCount
    };

    EffectLights lights;

    D3D12_GPU_DESCRIPTOR_HANDLE texture;
    D3D12_GPU_DESCRIPTOR_HANDLE textureSampler;
    D3D12_GPU_DESCRIPTOR_HANDLE environmentMap;
    D3D12_GPU_DESCRIPTOR_HANDLE environmentMapSampler;

    int GetPipelineStatePermutation(EnvironmentMapEffect::Mapping mapping, uint32_t effectFlags) const noexcept;

    void Apply(_In_ ID3D12GraphicsCommandList* commandList);
};


#pragma region Shaders
// Include the precompiled shader code.
namespace
{
#ifdef _GAMING_XBOX_SCARLETT
#include "XboxGamingScarlettEnvironmentMapEffect_VSEnvMap.inc"
#include "XboxGamingScarlettEnvironmentMapEffect_VSEnvMapFresnel.inc"
#include "XboxGamingScarlettEnvironmentMapEffect_VSEnvMapPixelLighting.inc"

#include "XboxGamingScarlettEnvironmentMapEffect_VSEnvMapBn.inc"
#include "XboxGamingScarlettEnvironmentMapEffect_VSEnvMapFresnelBn.inc"
#include "XboxGamingScarlettEnvironmentMapEffect_VSEnvMapPixelLightingBn.inc"

#include "XboxGamingScarlettEnvironmentMapEffect_PSEnvMap.inc"
#include "XboxGamingScarlettEnvironmentMapEffect_PSEnvMapNoFog.inc"
#include "XboxGamingScarlettEnvironmentMapEffect_PSEnvMapSpecular.inc"
#include "XboxGamingScarlettEnvironmentMapEffect_PSEnvMapSpecularNoFog.inc"
#include "XboxGamingScarlettEnvironmentMapEffect_PSEnvMapPixelLighting.inc"
#include "XboxGamingScarlettEnvironmentMapEffect_PSEnvMapPixelLightingNoFog.inc"
#include "XboxGamingScarlettEnvironmentMapEffect_PSEnvMapPixelLightingFresnel.inc"
#include "XboxGamingScarlettEnvironmentMapEffect_PSEnvMapPixelLightingFresnelNoFog.inc"

#include "XboxGamingScarlettEnvironmentMapEffect_PSEnvMapSpherePixelLighting.inc"
#include "XboxGamingScarlettEnvironmentMapEffect_PSEnvMapSpherePixelLightingNoFog.inc"
#include "XboxGamingScarlettEnvironmentMapEffect_PSEnvMapSpherePixelLightingFresnel.inc"
#include "XboxGamingScarlettEnvironmentMapEffect_PSEnvMapSpherePixelLightingFresnelNoFog.inc"

#include "XboxGamingScarlettEnvironmentMapEffect_PSEnvMapDualParabolaPixelLighting.inc"
#include "XboxGamingScarlettEnvironmentMapEffect_PSEnvMapDualParabolaPixelLightingNoFog.inc"
#include "XboxGamingScarlettEnvironmentMapEffect_PSEnvMapDualParabolaPixelLightingFresnel.inc"
#include "XboxGamingScarlettEnvironmentMapEffect_PSEnvMapDualParabolaPixelLightingFresnelNoFog.inc"
#elif defined(_GAMING_XBOX)
#include "XboxGamingXboxOneEnvironmentMapEffect_VSEnvMap.inc"
#include "XboxGamingXboxOneEnvironmentMapEffect_VSEnvMapFresnel.inc"
#include "XboxGamingXboxOneEnvironmentMapEffect_VSEnvMapPixelLighting.inc"

#include "XboxGamingXboxOneEnvironmentMapEffect_VSEnvMapBn.inc"
#include "XboxGamingXboxOneEnvironmentMapEffect_VSEnvMapFresnelBn.inc"
#include "XboxGamingXboxOneEnvironmentMapEffect_VSEnvMapPixelLightingBn.inc"

#include "XboxGamingXboxOneEnvironmentMapEffect_PSEnvMap.inc"
#include "XboxGamingXboxOneEnvironmentMapEffect_PSEnvMapNoFog.inc"
#include "XboxGamingXboxOneEnvironmentMapEffect_PSEnvMapSpecular.inc"
#include "XboxGamingXboxOneEnvironmentMapEffect_PSEnvMapSpecularNoFog.inc"
#include "XboxGamingXboxOneEnvironmentMapEffect_PSEnvMapPixelLighting.inc"
#include "XboxGamingXboxOneEnvironmentMapEffect_PSEnvMapPixelLightingNoFog.inc"
#include "XboxGamingXboxOneEnvironmentMapEffect_PSEnvMapPixelLightingFresnel.inc"
#include "XboxGamingXboxOneEnvironmentMapEffect_PSEnvMapPixelLightingFresnelNoFog.inc"

#include "XboxGamingXboxOneEnvironmentMapEffect_PSEnvMapSpherePixelLighting.inc"
#include "XboxGamingXboxOneEnvironmentMapEffect_PSEnvMapSpherePixelLightingNoFog.inc"
#include "XboxGamingXboxOneEnvironmentMapEffect_PSEnvMapSpherePixelLightingFresnel.inc"
#include "XboxGamingXboxOneEnvironmentMapEffect_PSEnvMapSpherePixelLightingFresnelNoFog.inc"

#include "XboxGamingXboxOneEnvironmentMapEffect_PSEnvMapDualParabolaPixelLighting.inc"
#include "XboxGamingXboxOneEnvironmentMapEffect_PSEnvMapDualParabolaPixelLightingNoFog.inc"
#include "XboxGamingXboxOneEnvironmentMapEffect_PSEnvMapDualParabolaPixelLightingFresnel.inc"
#include "XboxGamingXboxOneEnvironmentMapEffect_PSEnvMapDualParabolaPixelLightingFresnelNoFog.inc"
#elif defined(_XBOX_ONE) && defined(_TITLE)
#include "XboxOneEnvironmentMapEffect_VSEnvMap.inc"
#include "XboxOneEnvironmentMapEffect_VSEnvMapFresnel.inc"
#include "XboxOneEnvironmentMapEffect_VSEnvMapPixelLighting.inc"

#include "XboxOneEnvironmentMapEffect_VSEnvMapBn.inc"
#include "XboxOneEnvironmentMapEffect_VSEnvMapFresnelBn.inc"
#include "XboxOneEnvironmentMapEffect_VSEnvMapPixelLightingBn.inc"

#include "XboxOneEnvironmentMapEffect_PSEnvMap.inc"
#include "XboxOneEnvironmentMapEffect_PSEnvMapNoFog.inc"
#include "XboxOneEnvironmentMapEffect_PSEnvMapSpecular.inc"
#include "XboxOneEnvironmentMapEffect_PSEnvMapSpecularNoFog.inc"
#include "XboxOneEnvironmentMapEffect_PSEnvMapPixelLighting.inc"
#include "XboxOneEnvironmentMapEffect_PSEnvMapPixelLightingNoFog.inc"
#include "XboxOneEnvironmentMapEffect_PSEnvMapPixelLightingFresnel.inc"
#include "XboxOneEnvironmentMapEffect_PSEnvMapPixelLightingFresnelNoFog.inc"

#include "XboxOneEnvironmentMapEffect_PSEnvMapSpherePixelLighting.inc"
#include "XboxOneEnvironmentMapEffect_PSEnvMapSpherePixelLightingNoFog.inc"
#include "XboxOneEnvironmentMapEffect_PSEnvMapSpherePixelLightingFresnel.inc"
#include "XboxOneEnvironmentMapEffect_PSEnvMapSpherePixelLightingFresnelNoFog.inc"

#include "XboxOneEnvironmentMapEffect_PSEnvMapDualParabolaPixelLighting.inc"
#include "XboxOneEnvironmentMapEffect_PSEnvMapDualParabolaPixelLightingNoFog.inc"
#include "XboxOneEnvironmentMapEffect_PSEnvMapDualParabolaPixelLightingFresnel.inc"
#include "XboxOneEnvironmentMapEffect_PSEnvMapDualParabolaPixelLightingFresnelNoFog.inc"
#else
#include "EnvironmentMapEffect_VSEnvMap.inc"
#include "EnvironmentMapEffect_VSEnvMapFresnel.inc"
#include "EnvironmentMapEffect_VSEnvMapPixelLighting.inc"

#include "EnvironmentMapEffect_VSEnvMapBn.inc"
#include "EnvironmentMapEffect_VSEnvMapFresnelBn.inc"
#include "EnvironmentMapEffect_VSEnvMapPixelLightingBn.inc"

#include "EnvironmentMapEffect_PSEnvMap.inc"
#include "EnvironmentMapEffect_PSEnvMapNoFog.inc"
#include "EnvironmentMapEffect_PSEnvMapSpecular.inc"
#include "EnvironmentMapEffect_PSEnvMapSpecularNoFog.inc"
#include "EnvironmentMapEffect_PSEnvMapPixelLighting.inc"
#include "EnvironmentMapEffect_PSEnvMapPixelLightingNoFog.inc"
#include "EnvironmentMapEffect_PSEnvMapPixelLightingFresnel.inc"
#include "EnvironmentMapEffect_PSEnvMapPixelLightingFresnelNoFog.inc"

#include "EnvironmentMapEffect_PSEnvMapSpherePixelLighting.inc"
#include "EnvironmentMapEffect_PSEnvMapSpherePixelLightingNoFog.inc"
#include "EnvironmentMapEffect_PSEnvMapSpherePixelLightingFresnel.inc"
#include "EnvironmentMapEffect_PSEnvMapSpherePixelLightingFresnelNoFog.inc"

#include "EnvironmentMapEffect_PSEnvMapDualParabolaPixelLighting.inc"
#include "EnvironmentMapEffect_PSEnvMapDualParabolaPixelLightingNoFog.inc"
#include "EnvironmentMapEffect_PSEnvMapDualParabolaPixelLightingFresnel.inc"
#include "EnvironmentMapEffect_PSEnvMapDualParabolaPixelLightingFresnelNoFog.inc"
#endif
}


template<>
const D3D12_SHADER_BYTECODE EffectBase<EnvironmentMapEffectTraits>::VertexShaderBytecode[] =
{
    { EnvironmentMapEffect_VSEnvMap,                sizeof(EnvironmentMapEffect_VSEnvMap)                },
    { EnvironmentMapEffect_VSEnvMapFresnel,         sizeof(EnvironmentMapEffect_VSEnvMapFresnel)         },
    { EnvironmentMapEffect_VSEnvMapPixelLighting,   sizeof(EnvironmentMapEffect_VSEnvMapPixelLighting)   },

    { EnvironmentMapEffect_VSEnvMapBn,              sizeof(EnvironmentMapEffect_VSEnvMapBn)              },
    { EnvironmentMapEffect_VSEnvMapFresnelBn,       sizeof(EnvironmentMapEffect_VSEnvMapFresnelBn)       },
    { EnvironmentMapEffect_VSEnvMapPixelLightingBn, sizeof(EnvironmentMapEffect_VSEnvMapPixelLightingBn) },
};


template<>
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

    2,      // pixel lighting
    2,      // pixel lighting, no fog
    2,      // pixel lighting, fresnel
    2,      // pixel lighting, fresnel, no fog

    3,      // basic (biased vertex normals)
    3,      // basic (biased vertex normals), no fog
    4,      // fresnel (biased vertex normals)
    4,      // fresnel (biased vertex normals), no fog
    3,      // specular (biased vertex normals)
    3,      // specular (biased vertex normals), no fog
    4,      // fresnel + specular (biased vertex normals)
    4,      // fresnel + specular (biased vertex normals), no fog

    5,      // pixel lighting (biased vertex normals)
    5,      // pixel lighting (biased vertex normals), no fog
    5,      // pixel lighting (biased vertex normals), fresnel
    5,      // pixel lighting (biased vertex normals), fresnel, no fog

    2,      // spheremap pixel lighting
    2,      // spheremap pixel lighting, no fog
    2,      // spheremap pixel lighting, fresnel
    2,      // spheremap pixel lighting, fresnel, no fog

    5,      // spheremap pixel lighting (biased vertex normals)
    5,      // spheremap pixel lighting (biased vertex normals), no fog
    5,      // spheremap pixel lighting (biased vertex normals), fresnel
    5,      // spheremap pixel lighting (biased vertex normals), fresnel, no fog

    2,      // dual-parabola pixel lighting
    2,      // dual-parabola pixel lighting, no fog
    2,      // dual-parabola pixel lighting, fresnel
    2,      // dual-parabola pixel lighting, fresnel, no fog

    5,      // dual-parabola pixel lighting (biased vertex normals)
    5,      // dual-parabola pixel lighting (biased vertex normals), no fog
    5,      // dual-parabola pixel lighting (biased vertex normals), fresnel
    5,      // dual-parabola pixel lighting (biased vertex normals), fresnel, no fog
};


template<>
const D3D12_SHADER_BYTECODE EffectBase<EnvironmentMapEffectTraits>::PixelShaderBytecode[] =
{
    { EnvironmentMapEffect_PSEnvMap,                          sizeof(EnvironmentMapEffect_PSEnvMap)                          },
    { EnvironmentMapEffect_PSEnvMapNoFog,                     sizeof(EnvironmentMapEffect_PSEnvMapNoFog)                     },
    { EnvironmentMapEffect_PSEnvMapSpecular,                  sizeof(EnvironmentMapEffect_PSEnvMapSpecular)                  },
    { EnvironmentMapEffect_PSEnvMapSpecularNoFog,             sizeof(EnvironmentMapEffect_PSEnvMapSpecularNoFog)             },
    { EnvironmentMapEffect_PSEnvMapPixelLighting,             sizeof(EnvironmentMapEffect_PSEnvMapPixelLighting)             },
    { EnvironmentMapEffect_PSEnvMapPixelLightingNoFog,        sizeof(EnvironmentMapEffect_PSEnvMapPixelLightingNoFog)        },
    { EnvironmentMapEffect_PSEnvMapPixelLightingFresnel,      sizeof(EnvironmentMapEffect_PSEnvMapPixelLightingFresnel)      },
    { EnvironmentMapEffect_PSEnvMapPixelLightingFresnelNoFog, sizeof(EnvironmentMapEffect_PSEnvMapPixelLightingFresnelNoFog) },

    { EnvironmentMapEffect_PSEnvMapSpherePixelLighting,             sizeof(EnvironmentMapEffect_PSEnvMapSpherePixelLighting) },
    { EnvironmentMapEffect_PSEnvMapSpherePixelLightingNoFog,        sizeof(EnvironmentMapEffect_PSEnvMapSpherePixelLightingNoFog) },
    { EnvironmentMapEffect_PSEnvMapSpherePixelLightingFresnel,      sizeof(EnvironmentMapEffect_PSEnvMapSpherePixelLightingFresnel) },
    { EnvironmentMapEffect_PSEnvMapSpherePixelLightingFresnelNoFog, sizeof(EnvironmentMapEffect_PSEnvMapSpherePixelLightingFresnelNoFog) },

    { EnvironmentMapEffect_PSEnvMapDualParabolaPixelLighting,             sizeof(EnvironmentMapEffect_PSEnvMapDualParabolaPixelLighting) },
    { EnvironmentMapEffect_PSEnvMapDualParabolaPixelLightingNoFog,        sizeof(EnvironmentMapEffect_PSEnvMapDualParabolaPixelLightingNoFog) },
    { EnvironmentMapEffect_PSEnvMapDualParabolaPixelLightingFresnel,      sizeof(EnvironmentMapEffect_PSEnvMapDualParabolaPixelLightingFresnel) },
    { EnvironmentMapEffect_PSEnvMapDualParabolaPixelLightingFresnelNoFog, sizeof(EnvironmentMapEffect_PSEnvMapDualParabolaPixelLightingFresnelNoFog) },
};


template<>
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

    4,      // per pixel lighting
    5,      // per pixel lighting, no fog
    6,      // per pixel lighting, fresnel
    7,      // per pixel lighting, fresnel, no fog

    0,      // basic (biased vertex normals)
    1,      // basic (biased vertex normals), no fog
    0,      // fresnel (biased vertex normals)
    1,      // fresnel (biased vertex normals), no fog
    2,      // specular (biased vertex normals)
    3,      // specular (biased vertex normals), no fog
    2,      // fresnel + specular (biased vertex normals)
    3,      // fresnel + specular (biased vertex normals), no fog

    4,      // per pixel lighting (biased vertex normals)
    5,      // per pixel lighting (biased vertex normals), no fog
    6,      // per pixel lighting (biased vertex normals), fresnel
    7,      // per pixel lighting (biased vertex normals), fresnel, no fog

    8,      // spheremap pixel lighting
    9,      // spheremap pixel lighting, no fog
    10,     // spheremap pixel lighting, fresnel
    11,     // spheremap pixel lighting, fresnel, no fog

    8,      // spheremap pixel lighting (biased vertex normals)
    9,      // spheremap pixel lighting (biased vertex normals), no fog
    10,     // spheremap pixel lighting (biased vertex normals), fresnel
    11,     // spheremap pixel lighting (biased vertex normals), fresnel, no fog

    12,     // dual-parabola pixel lighting
    13,     // dual-parabola pixel lighting, no fog
    14,     // dual-parabola pixel lighting, fresnel
    15,     // dual-parabola pixel lighting, fresnel, no fog

    12,     // dual-parabola pixel lighting (biased vertex normals)
    13,     // dual-parabola pixel lighting (biased vertex normals), no fog
    14,     // dual-parabola pixel lighting (biased vertex normals), fresnel
    15,     // dual-parabola pixel lighting (biased vertex normals), fresnel, no fog
};
#pragma endregion

// Global pool of per-device EnvironmentMapEffect resources.
template<>
SharedResourcePool<ID3D12Device*, EffectBase<EnvironmentMapEffectTraits>::DeviceResources> EffectBase<EnvironmentMapEffectTraits>::deviceResourcesPool = {};


// Constructor.
EnvironmentMapEffect::Impl::Impl(
    _In_ ID3D12Device* device,
    uint32_t effectFlags,
    const EffectPipelineStateDescription& pipelineDescription,
    EnvironmentMapEffect::Mapping mapping)
    : EffectBase(device),
    texture{},
    textureSampler{},
    environmentMap{},
    environmentMapSampler{}
{
    static_assert(static_cast<int>(std::size(EffectBase<EnvironmentMapEffectTraits>::VertexShaderIndices)) == EnvironmentMapEffectTraits::ShaderPermutationCount, "array/max mismatch");
    static_assert(static_cast<int>(std::size(EffectBase<EnvironmentMapEffectTraits>::VertexShaderBytecode)) == EnvironmentMapEffectTraits::VertexShaderCount, "array/max mismatch");
    static_assert(static_cast<int>(std::size(EffectBase<EnvironmentMapEffectTraits>::PixelShaderBytecode)) == EnvironmentMapEffectTraits::PixelShaderCount, "array/max mismatch");
    static_assert(static_cast<int>(std::size(EffectBase<EnvironmentMapEffectTraits>::PixelShaderIndices)) == EnvironmentMapEffectTraits::ShaderPermutationCount, "array/max mismatch");

    // Create root signature.
    {
        ENUM_FLAGS_CONSTEXPR D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
            | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
            | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
            | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
#ifdef _GAMING_XBOX_SCARLETT
            | D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS
            | D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS
#endif
            ;

        CD3DX12_ROOT_PARAMETER rootParameters[RootParameterIndex::RootParameterCount] = {};
        rootParameters[RootParameterIndex::ConstantBuffer].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);

        // Texture 1
        const CD3DX12_DESCRIPTOR_RANGE textureDescriptor(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
        const CD3DX12_DESCRIPTOR_RANGE textureSamplerDescriptor(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
        rootParameters[RootParameterIndex::TextureSRV].InitAsDescriptorTable(1, &textureDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[RootParameterIndex::TextureSampler].InitAsDescriptorTable(1, &textureSamplerDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);

        // Texture 2
        const CD3DX12_DESCRIPTOR_RANGE cubemapDescriptor(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
        const CD3DX12_DESCRIPTOR_RANGE cubemapSamplerDescriptor(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 1);
        rootParameters[RootParameterIndex::CubemapSRV].InitAsDescriptorTable(1, &cubemapDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[RootParameterIndex::CubemapSampler].InitAsDescriptorTable(1, &cubemapSamplerDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);

        // Create the root signature
        CD3DX12_ROOT_SIGNATURE_DESC rsigDesc = {};
        rsigDesc.Init(static_cast<UINT>(std::size(rootParameters)), rootParameters, 0, nullptr, rootSignatureFlags);

        mRootSignature = GetRootSignature(0, rsigDesc);
    }

    assert(mRootSignature != nullptr);

    fog.enabled = (effectFlags & EffectFlags::Fog) != 0;

    if (effectFlags & EffectFlags::VertexColor)
    {
        DebugTrace("ERROR: EnvironmentMapEffect does not implement EffectFlags::VertexColor\n");
        throw std::invalid_argument("VertexColor effect flag is invalid");
    }
    else if (effectFlags & EffectFlags::Instancing)
    {
        DebugTrace("ERROR: EnvironmentMapEffect does not implement EffectFlags::Instancing\n");
        throw std::invalid_argument("Instancing effect flag is invalid");
    }

    constants.environmentMapAmount = 1;
    constants.fresnelFactor = 1;

    XMVECTOR unwantedOutput[MaxDirectionalLights];

    lights.InitializeConstants(unwantedOutput[0], constants.lightDirection, constants.lightDiffuseColor, unwantedOutput);

    // Create pipeline state.
    const int sp = GetPipelineStatePermutation(mapping, effectFlags);
    assert(sp >= 0 && sp < EnvironmentMapEffectTraits::ShaderPermutationCount);
    _Analysis_assume_(sp >= 0 && sp < EnvironmentMapEffectTraits::ShaderPermutationCount);

    const int vi = EffectBase<EnvironmentMapEffectTraits>::VertexShaderIndices[sp];
    assert(vi >= 0 && vi < EnvironmentMapEffectTraits::VertexShaderCount);
    _Analysis_assume_(vi >= 0 && vi < EnvironmentMapEffectTraits::VertexShaderCount);
    const int pi = EffectBase<EnvironmentMapEffectTraits>::PixelShaderIndices[sp];
    assert(pi >= 0 && pi < EnvironmentMapEffectTraits::PixelShaderCount);
    _Analysis_assume_(pi >= 0 && pi < EnvironmentMapEffectTraits::PixelShaderCount);

    pipelineDescription.CreatePipelineState(
        device,
        mRootSignature,
        EffectBase<EnvironmentMapEffectTraits>::VertexShaderBytecode[vi],
        EffectBase<EnvironmentMapEffectTraits>::PixelShaderBytecode[pi],
        mPipelineState.GetAddressOf());

    SetDebugObjectName(mPipelineState.Get(), L"EnvironmentMapEffect");
}


int EnvironmentMapEffect::Impl::GetPipelineStatePermutation(
    EnvironmentMapEffect::Mapping mapping,
    uint32_t effectFlags) const noexcept
{
    const bool biasedVertexNormals = (effectFlags & EffectFlags::BiasedVertexNormals) != 0;

    int permutation = 0;

    // Use optimized shaders if fog is disabled.
    if (!fog.enabled)
    {
        permutation += 1;
    }

    // Support fresnel?
    if (effectFlags & EffectFlags::Fresnel)
    {
        permutation += 2;
    }

    if (mapping == Mapping_Sphere)
    {
        permutation += 24;

        if (biasedVertexNormals)
        {
            permutation += 4;
        }
    }
    else if (mapping == Mapping_DualParabola)
    {
        permutation += 32;

        if (biasedVertexNormals)
        {
            permutation += 4;
        }
    }
    else // Mapping_Cube
    {
        if (effectFlags & EffectFlags::PerPixelLightingBit)
        {
            permutation += 8;
        }
        else
        {
            if (effectFlags & EffectFlags::Specular)
            {
                permutation += 4;
            }
        }

        if (biasedVertexNormals)
        {
            // Compressed normals need to be scaled and biased in the vertex shader.
            permutation += 12;
        }
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
    commandList->SetGraphicsRootSignature(mRootSignature);

    // Set the textures
    if (!texture.ptr || !environmentMap.ptr)
    {
        DebugTrace("ERROR: Missing texture(s) for EnvironmentMapEffect (texture %llu, environmentMap %llu)\n", texture.ptr, environmentMap.ptr);
        throw std::runtime_error("EnvironmentMapEffect");
    }
    if (!textureSampler.ptr || !environmentMapSampler.ptr)
    {
        DebugTrace("ERROR: Missing sampler(s) for EnvironmentMapEffect (sampler %llu, environmentMap %llu)\n", textureSampler.ptr, environmentMapSampler.ptr);
        throw std::runtime_error("EnvironmentMapEffect");
    }

    // **NOTE** If D3D asserts or crashes here, you probably need to call commandList->SetDescriptorHeaps() with the required descriptor heaps.
    commandList->SetGraphicsRootDescriptorTable(RootParameterIndex::TextureSRV, texture);
    commandList->SetGraphicsRootDescriptorTable(RootParameterIndex::TextureSampler, textureSampler);
    commandList->SetGraphicsRootDescriptorTable(RootParameterIndex::CubemapSRV, environmentMap);
    commandList->SetGraphicsRootDescriptorTable(RootParameterIndex::CubemapSampler, environmentMapSampler);

    // Set constants
    commandList->SetGraphicsRootConstantBufferView(RootParameterIndex::ConstantBuffer, GetConstantBufferGpuAddress());

    // Set the pipeline state
    commandList->SetPipelineState(EffectBase::mPipelineState.Get());
}


// Public constructor.
EnvironmentMapEffect::EnvironmentMapEffect(
    _In_ ID3D12Device* device,
    uint32_t effectFlags,
    const EffectPipelineStateDescription& pipelineDescription,
    EnvironmentMapEffect::Mapping mapping)
    : pImpl(std::make_unique<Impl>(device, effectFlags, pipelineDescription, mapping))
{
}


EnvironmentMapEffect::EnvironmentMapEffect(EnvironmentMapEffect&&) noexcept = default;
EnvironmentMapEffect& EnvironmentMapEffect::operator= (EnvironmentMapEffect&&) noexcept = default;
EnvironmentMapEffect::~EnvironmentMapEffect() = default;


// IEffect methods.
void EnvironmentMapEffect::Apply(
    _In_ ID3D12GraphicsCommandList* cmdList)
{
    pImpl->Apply(cmdList);
}


// Camera settings.
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


void XM_CALLCONV EnvironmentMapEffect::SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection)
{
    pImpl->matrices.world = world;
    pImpl->matrices.view = view;
    pImpl->matrices.projection = projection;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj | EffectDirtyFlags::WorldInverseTranspose | EffectDirtyFlags::EyePosition | EffectDirtyFlags::FogVector;
}


// Material settings.
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


void XM_CALLCONV EnvironmentMapEffect::SetColorAndAlpha(FXMVECTOR value)
{
    pImpl->lights.diffuseColor = value;
    pImpl->lights.alpha = XMVectorGetW(value);

    pImpl->dirtyFlags |= EffectDirtyFlags::MaterialColor;
}


// Light settings.
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


// Fog settings.
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


// Texture settings.
void EnvironmentMapEffect::SetTexture(D3D12_GPU_DESCRIPTOR_HANDLE texture, D3D12_GPU_DESCRIPTOR_HANDLE sampler)
{
    pImpl->texture = texture;
    pImpl->textureSampler = sampler;
}


void EnvironmentMapEffect::SetEnvironmentMap(D3D12_GPU_DESCRIPTOR_HANDLE texture, D3D12_GPU_DESCRIPTOR_HANDLE sampler)
{
    pImpl->environmentMap = texture;
    pImpl->environmentMapSampler = sampler;
}


// Additional settings.
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
