//--------------------------------------------------------------------------------------
// File: NormalMapEffect.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "EffectCommon.h"

using namespace DirectX;

namespace
{
    // Constant buffer layout. Must match the shader!
    struct NormalMapEffectConstants
    {
        XMVECTOR diffuseColor;
        XMVECTOR emissiveColor;
        XMVECTOR specularColorAndPower;

        XMVECTOR lightDirection[IEffectLights::MaxDirectionalLights];
        XMVECTOR lightDiffuseColor[IEffectLights::MaxDirectionalLights];
        XMVECTOR lightSpecularColor[IEffectLights::MaxDirectionalLights];

        XMVECTOR eyePosition;

        XMVECTOR fogColor;
        XMVECTOR fogVector;

        XMMATRIX world;
        XMVECTOR worldInverseTranspose[3];
        XMMATRIX worldViewProj;
    };

    static_assert((sizeof(NormalMapEffectConstants) % 16) == 0, "CB size not padded correctly");


    // Traits type describes our characteristics to the EffectBase template.
    struct NormalMapEffectTraits
    {
        using ConstantBufferType = NormalMapEffectConstants;

        static constexpr int VertexShaderCount = 16;
        static constexpr int PixelShaderCount = 4;
        static constexpr int ShaderPermutationCount = 32;
        static constexpr int RootSignatureCount = 2;
    };
}

// Internal NormalMapEffect implementation class.
class NormalMapEffect::Impl : public EffectBase<NormalMapEffectTraits>
{
public:
    Impl(_In_ ID3D12Device* device, uint32_t effectFlags, const EffectPipelineStateDescription& pipelineDescription);

    enum RootParameterIndex
    {
        TextureSRV,
        TextureNormalSRV,
        TextureSampler,
        ConstantBuffer,
        TextureSpecularSRV,
        RootParameterCount
    };

    bool specularMap;

    D3D12_GPU_DESCRIPTOR_HANDLE texture;
    D3D12_GPU_DESCRIPTOR_HANDLE specular;
    D3D12_GPU_DESCRIPTOR_HANDLE normal;
    D3D12_GPU_DESCRIPTOR_HANDLE sampler;

    EffectLights lights;

    int GetPipelineStatePermutation(uint32_t effectFlags) const noexcept;

    void Apply(_In_ ID3D12GraphicsCommandList* commandList);
};


// Include the precompiled shader code.
namespace
{
#ifdef _GAMING_XBOX_SCARLETT
    #include "Shaders/Compiled/XboxGamingScarlettNormalMapEffect_VSNormalPixelLightingTx.inc"
    #include "Shaders/Compiled/XboxGamingScarlettNormalMapEffect_VSNormalPixelLightingTxVc.inc"
    #include "Shaders/Compiled/XboxGamingScarlettNormalMapEffect_VSNormalPixelLightingTxNoSpec.inc"
    #include "Shaders/Compiled/XboxGamingScarlettNormalMapEffect_VSNormalPixelLightingTxVcNoSpec.inc"

    #include "Shaders/Compiled/XboxGamingScarlettNormalMapEffect_VSNormalPixelLightingTxBn.inc"
    #include "Shaders/Compiled/XboxGamingScarlettNormalMapEffect_VSNormalPixelLightingTxVcBn.inc"
    #include "Shaders/Compiled/XboxGamingScarlettNormalMapEffect_VSNormalPixelLightingTxNoSpecBn.inc"
    #include "Shaders/Compiled/XboxGamingScarlettNormalMapEffect_VSNormalPixelLightingTxVcNoSpecBn.inc"

    #include "Shaders/Compiled/XboxGamingScarlettNormalMapEffect_VSNormalPixelLightingTxInst.inc"
    #include "Shaders/Compiled/XboxGamingScarlettNormalMapEffect_VSNormalPixelLightingTxVcInst.inc"
    #include "Shaders/Compiled/XboxGamingScarlettNormalMapEffect_VSNormalPixelLightingTxNoSpecInst.inc"
    #include "Shaders/Compiled/XboxGamingScarlettNormalMapEffect_VSNormalPixelLightingTxVcNoSpecInst.inc"

    #include "Shaders/Compiled/XboxGamingScarlettNormalMapEffect_VSNormalPixelLightingTxBnInst.inc"
    #include "Shaders/Compiled/XboxGamingScarlettNormalMapEffect_VSNormalPixelLightingTxVcBnInst.inc"
    #include "Shaders/Compiled/XboxGamingScarlettNormalMapEffect_VSNormalPixelLightingTxNoSpecBnInst.inc"
    #include "Shaders/Compiled/XboxGamingScarlettNormalMapEffect_VSNormalPixelLightingTxVcNoSpecBnInst.inc"

    #include "Shaders/Compiled/XboxGamingScarlettNormalMapEffect_PSNormalPixelLightingTx.inc"
    #include "Shaders/Compiled/XboxGamingScarlettNormalMapEffect_PSNormalPixelLightingTxNoFog.inc"
    #include "Shaders/Compiled/XboxGamingScarlettNormalMapEffect_PSNormalPixelLightingTxNoSpec.inc"
    #include "Shaders/Compiled/XboxGamingScarlettNormalMapEffect_PSNormalPixelLightingTxNoFogSpec.inc"
#elif defined(_GAMING_XBOX)
    #include "Shaders/Compiled/XboxGamingXboxOneNormalMapEffect_VSNormalPixelLightingTx.inc"
    #include "Shaders/Compiled/XboxGamingXboxOneNormalMapEffect_VSNormalPixelLightingTxVc.inc"
    #include "Shaders/Compiled/XboxGamingXboxOneNormalMapEffect_VSNormalPixelLightingTxNoSpec.inc"
    #include "Shaders/Compiled/XboxGamingXboxOneNormalMapEffect_VSNormalPixelLightingTxVcNoSpec.inc"

    #include "Shaders/Compiled/XboxGamingXboxOneNormalMapEffect_VSNormalPixelLightingTxBn.inc"
    #include "Shaders/Compiled/XboxGamingXboxOneNormalMapEffect_VSNormalPixelLightingTxVcBn.inc"
    #include "Shaders/Compiled/XboxGamingXboxOneNormalMapEffect_VSNormalPixelLightingTxNoSpecBn.inc"
    #include "Shaders/Compiled/XboxGamingXboxOneNormalMapEffect_VSNormalPixelLightingTxVcNoSpecBn.inc"

    #include "Shaders/Compiled/XboxGamingXboxOneNormalMapEffect_VSNormalPixelLightingTxInst.inc"
    #include "Shaders/Compiled/XboxGamingXboxOneNormalMapEffect_VSNormalPixelLightingTxVcInst.inc"
    #include "Shaders/Compiled/XboxGamingXboxOneNormalMapEffect_VSNormalPixelLightingTxNoSpecInst.inc"
    #include "Shaders/Compiled/XboxGamingXboxOneNormalMapEffect_VSNormalPixelLightingTxVcNoSpecInst.inc"

    #include "Shaders/Compiled/XboxGamingXboxOneNormalMapEffect_VSNormalPixelLightingTxBnInst.inc"
    #include "Shaders/Compiled/XboxGamingXboxOneNormalMapEffect_VSNormalPixelLightingTxVcBnInst.inc"
    #include "Shaders/Compiled/XboxGamingXboxOneNormalMapEffect_VSNormalPixelLightingTxNoSpecBnInst.inc"
    #include "Shaders/Compiled/XboxGamingXboxOneNormalMapEffect_VSNormalPixelLightingTxVcNoSpecBnInst.inc"

    #include "Shaders/Compiled/XboxGamingXboxOneNormalMapEffect_PSNormalPixelLightingTx.inc"
    #include "Shaders/Compiled/XboxGamingXboxOneNormalMapEffect_PSNormalPixelLightingTxNoFog.inc"
    #include "Shaders/Compiled/XboxGamingXboxOneNormalMapEffect_PSNormalPixelLightingTxNoSpec.inc"
    #include "Shaders/Compiled/XboxGamingXboxOneNormalMapEffect_PSNormalPixelLightingTxNoFogSpec.inc"
#elif defined(_XBOX_ONE) && defined(_TITLE)
    #include "Shaders/Compiled/XboxOneNormalMapEffect_VSNormalPixelLightingTx.inc"
    #include "Shaders/Compiled/XboxOneNormalMapEffect_VSNormalPixelLightingTxVc.inc"
    #include "Shaders/Compiled/XboxOneNormalMapEffect_VSNormalPixelLightingTxNoSpec.inc"
    #include "Shaders/Compiled/XboxOneNormalMapEffect_VSNormalPixelLightingTxVcNoSpec.inc"

    #include "Shaders/Compiled/XboxOneNormalMapEffect_VSNormalPixelLightingTxBn.inc"
    #include "Shaders/Compiled/XboxOneNormalMapEffect_VSNormalPixelLightingTxVcBn.inc"
    #include "Shaders/Compiled/XboxOneNormalMapEffect_VSNormalPixelLightingTxNoSpecBn.inc"
    #include "Shaders/Compiled/XboxOneNormalMapEffect_VSNormalPixelLightingTxVcNoSpecBn.inc"

    #include "Shaders/Compiled/XboxOneNormalMapEffect_VSNormalPixelLightingTxInst.inc"
    #include "Shaders/Compiled/XboxOneNormalMapEffect_VSNormalPixelLightingTxVcInst.inc"
    #include "Shaders/Compiled/XboxOneNormalMapEffect_VSNormalPixelLightingTxNoSpecInst.inc"
    #include "Shaders/Compiled/XboxOneNormalMapEffect_VSNormalPixelLightingTxVcNoSpecInst.inc"

    #include "Shaders/Compiled/XboxOneNormalMapEffect_VSNormalPixelLightingTxBnInst.inc"
    #include "Shaders/Compiled/XboxOneNormalMapEffect_VSNormalPixelLightingTxVcBnInst.inc"
    #include "Shaders/Compiled/XboxOneNormalMapEffect_VSNormalPixelLightingTxNoSpecBnInst.inc"
    #include "Shaders/Compiled/XboxOneNormalMapEffect_VSNormalPixelLightingTxVcNoSpecBnInst.inc"

    #include "Shaders/Compiled/XboxOneNormalMapEffect_PSNormalPixelLightingTx.inc"
    #include "Shaders/Compiled/XboxOneNormalMapEffect_PSNormalPixelLightingTxNoFog.inc"
    #include "Shaders/Compiled/XboxOneNormalMapEffect_PSNormalPixelLightingTxNoSpec.inc"
    #include "Shaders/Compiled/XboxOneNormalMapEffect_PSNormalPixelLightingTxNoFogSpec.inc"
#else    
    #include "Shaders/Compiled/NormalMapEffect_VSNormalPixelLightingTx.inc"
    #include "Shaders/Compiled/NormalMapEffect_VSNormalPixelLightingTxVc.inc"
    #include "Shaders/Compiled/NormalMapEffect_VSNormalPixelLightingTxNoSpec.inc"
    #include "Shaders/Compiled/NormalMapEffect_VSNormalPixelLightingTxVcNoSpec.inc"

    #include "Shaders/Compiled/NormalMapEffect_VSNormalPixelLightingTxBn.inc"
    #include "Shaders/Compiled/NormalMapEffect_VSNormalPixelLightingTxVcBn.inc"
    #include "Shaders/Compiled/NormalMapEffect_VSNormalPixelLightingTxNoSpecBn.inc"
    #include "Shaders/Compiled/NormalMapEffect_VSNormalPixelLightingTxVcNoSpecBn.inc"

    #include "Shaders/Compiled/NormalMapEffect_VSNormalPixelLightingTxInst.inc"
    #include "Shaders/Compiled/NormalMapEffect_VSNormalPixelLightingTxVcInst.inc"
    #include "Shaders/Compiled/NormalMapEffect_VSNormalPixelLightingTxNoSpecInst.inc"
    #include "Shaders/Compiled/NormalMapEffect_VSNormalPixelLightingTxVcNoSpecInst.inc"

    #include "Shaders/Compiled/NormalMapEffect_VSNormalPixelLightingTxBnInst.inc"
    #include "Shaders/Compiled/NormalMapEffect_VSNormalPixelLightingTxVcBnInst.inc"
    #include "Shaders/Compiled/NormalMapEffect_VSNormalPixelLightingTxNoSpecBnInst.inc"
    #include "Shaders/Compiled/NormalMapEffect_VSNormalPixelLightingTxVcNoSpecBnInst.inc"

    #include "Shaders/Compiled/NormalMapEffect_PSNormalPixelLightingTx.inc"
    #include "Shaders/Compiled/NormalMapEffect_PSNormalPixelLightingTxNoFog.inc"
    #include "Shaders/Compiled/NormalMapEffect_PSNormalPixelLightingTxNoSpec.inc"
    #include "Shaders/Compiled/NormalMapEffect_PSNormalPixelLightingTxNoFogSpec.inc"
#endif
}


template<>
const D3D12_SHADER_BYTECODE EffectBase<NormalMapEffectTraits>::VertexShaderBytecode[] =
{
    { NormalMapEffect_VSNormalPixelLightingTx,               sizeof(NormalMapEffect_VSNormalPixelLightingTx)               },
    { NormalMapEffect_VSNormalPixelLightingTxVc,             sizeof(NormalMapEffect_VSNormalPixelLightingTxVc)             },

    { NormalMapEffect_VSNormalPixelLightingTxBn,             sizeof(NormalMapEffect_VSNormalPixelLightingTxBn)             },
    { NormalMapEffect_VSNormalPixelLightingTxVcBn,           sizeof(NormalMapEffect_VSNormalPixelLightingTxVcBn)           },

    { NormalMapEffect_VSNormalPixelLightingTxNoSpec,         sizeof(NormalMapEffect_VSNormalPixelLightingTxNoSpec)         },
    { NormalMapEffect_VSNormalPixelLightingTxVcNoSpec,       sizeof(NormalMapEffect_VSNormalPixelLightingTxVcNoSpec)       },

    { NormalMapEffect_VSNormalPixelLightingTxNoSpecBn,       sizeof(NormalMapEffect_VSNormalPixelLightingTxNoSpecBn)       },
    { NormalMapEffect_VSNormalPixelLightingTxVcNoSpecBn,     sizeof(NormalMapEffect_VSNormalPixelLightingTxVcNoSpecBn)     },

    { NormalMapEffect_VSNormalPixelLightingTxInst,           sizeof(NormalMapEffect_VSNormalPixelLightingTxInst)           },
    { NormalMapEffect_VSNormalPixelLightingTxVcInst,         sizeof(NormalMapEffect_VSNormalPixelLightingTxVcInst)         },

    { NormalMapEffect_VSNormalPixelLightingTxBnInst,         sizeof(NormalMapEffect_VSNormalPixelLightingTxBnInst)         },
    { NormalMapEffect_VSNormalPixelLightingTxVcBnInst,       sizeof(NormalMapEffect_VSNormalPixelLightingTxVcBnInst)       },

    { NormalMapEffect_VSNormalPixelLightingTxNoSpecInst,     sizeof(NormalMapEffect_VSNormalPixelLightingTxNoSpecInst)     },
    { NormalMapEffect_VSNormalPixelLightingTxVcNoSpecInst,   sizeof(NormalMapEffect_VSNormalPixelLightingTxVcNoSpecInst)   },

    { NormalMapEffect_VSNormalPixelLightingTxNoSpecBnInst,   sizeof(NormalMapEffect_VSNormalPixelLightingTxNoSpecBnInst)   },
    { NormalMapEffect_VSNormalPixelLightingTxVcNoSpecBnInst, sizeof(NormalMapEffect_VSNormalPixelLightingTxVcNoSpecBnInst) },
};


template<>
const int EffectBase<NormalMapEffectTraits>::VertexShaderIndices[] =
{
    0,     // pixel lighting + texture
    0,     // pixel lighting + texture, no fog
    1,     // pixel lighting + texture + vertex color
    1,     // pixel lighting + texture + vertex color, no fog

    4,     // pixel lighting + texture, no specular
    4,     // pixel lighting + texture, no fog or specular
    5,     // pixel lighting + texture + vertex color, no specular
    5,     // pixel lighting + texture + vertex color, no fog or specular

    2,     // pixel lighting (biased vertex normal) + texture
    2,     // pixel lighting (biased vertex normal) + texture, no fog
    3,     // pixel lighting (biased vertex normal) + texture + vertex color
    3,     // pixel lighting (biased vertex normal) + texture + vertex color, no fog

    6,     // pixel lighting (biased vertex normal) + texture, no specular
    6,     // pixel lighting (biased vertex normal) + texture, no fog or specular
    7,     // pixel lighting (biased vertex normal) + texture + vertex color, no specular
    7,     // pixel lighting (biased vertex normal) + texture + vertex color, no fog or specular

    8,     // instancing + pixel lighting + texture
    8,     // instancing + pixel lighting + texture, no fog
    9,     // instancing + pixel lighting + texture + vertex color
    9,     // instancing + pixel lighting + texture + vertex color, no fog

    12,    // instancing + pixel lighting + texture, no specular
    12,    // instancing + pixel lighting + texture, no fog or specular
    13,    // instancing + pixel lighting + texture + vertex color, no specular
    13,    // instancing + pixel lighting + texture + vertex color, no fog or specular

    10,    // instancing + pixel lighting (biased vertex normal) + texture
    10,    // instancing + pixel lighting (biased vertex normal) + texture, no fog
    11,    // instancing + pixel lighting (biased vertex normal) + texture + vertex color
    11,    // instancing + pixel lighting (biased vertex normal) + texture + vertex color, no fog

    14,    // instancing + pixel lighting (biased vertex normal) + texture, no specular
    14,    // instancing + pixel lighting (biased vertex normal) + texture, no fog or specular
    15,    // instancing + pixel lighting (biased vertex normal) + texture + vertex color, no specular
    15,    // instancing + pixel lighting (biased vertex normal) + texture + vertex color, no fog or specular
};


template<>
const D3D12_SHADER_BYTECODE EffectBase<NormalMapEffectTraits>::PixelShaderBytecode[] =
{
    { NormalMapEffect_PSNormalPixelLightingTx,          sizeof(NormalMapEffect_PSNormalPixelLightingTx)          },
    { NormalMapEffect_PSNormalPixelLightingTxNoFog,     sizeof(NormalMapEffect_PSNormalPixelLightingTxNoFog)     },
    { NormalMapEffect_PSNormalPixelLightingTxNoSpec,    sizeof(NormalMapEffect_PSNormalPixelLightingTxNoSpec)    },
    { NormalMapEffect_PSNormalPixelLightingTxNoFogSpec, sizeof(NormalMapEffect_PSNormalPixelLightingTxNoFogSpec) },
};


template<>
const int EffectBase<NormalMapEffectTraits>::PixelShaderIndices[] =
{
    0,     // pixel lighting + texture
    1,     // pixel lighting + texture, no fog
    0,     // pixel lighting + texture + vertex color
    1,     // pixel lighting + texture + vertex color, no fog

    2,     // pixel lighting + texture, no specular
    3,     // pixel lighting + texture, no fog or specular
    2,     // pixel lighting + texture + vertex color, no specular
    3,     // pixel lighting + texture + vertex color, no fog or specular

    0,     // pixel lighting (biased vertex normal) + texture
    1,     // pixel lighting (biased vertex normal) + texture, no fog
    0,     // pixel lighting (biased vertex normal) + texture + vertex color
    1,     // pixel lighting (biased vertex normal) + texture + vertex color, no fog

    2,     // pixel lighting (biased vertex normal) + texture, no specular
    3,     // pixel lighting (biased vertex normal) + texture, no fog or specular
    2,     // pixel lighting (biased vertex normal) + texture + vertex color, no specular
    3,     // pixel lighting (biased vertex normal) + texture + vertex color, no fog or specular

    0,     // instancing + pixel lighting + texture
    1,     // instancing + pixel lighting + texture, no fog
    0,     // instancing + pixel lighting + texture + vertex color
    1,     // instancing + pixel lighting + texture + vertex color, no fog

    2,     // instancing + pixel lighting + texture, no specular
    3,     // instancing + pixel lighting + texture, no fog or specular
    2,     // instancing + pixel lighting + texture + vertex color, no specular
    3,     // instancing + pixel lighting + texture + vertex color, no fog or specular

    0,     // instancing + pixel lighting (biased vertex normal) + texture
    1,     // instancing + pixel lighting (biased vertex normal) + texture, no fog
    0,     // instancing + pixel lighting (biased vertex normal) + texture + vertex color
    1,     // instancing + pixel lighting (biased vertex normal) + texture + vertex color, no fog

    2,     // instancing + pixel lighting (biased vertex normal) + texture, no specular
    3,     // instancing + pixel lighting (biased vertex normal) + texture, no fog or specular
    2,     // instancing + pixel lighting (biased vertex normal) + texture + vertex color, no specular
    3,     // instancing + pixel lighting (biased vertex normal) + texture + vertex color, no fog or specular
};

// Global pool of per-device NormalMapEffect resources.
template<>
SharedResourcePool<ID3D12Device*, EffectBase<NormalMapEffectTraits>::DeviceResources> EffectBase<NormalMapEffectTraits>::deviceResourcesPool = {};


// Constructor.
NormalMapEffect::Impl::Impl(
    _In_ ID3D12Device* device,
    uint32_t effectFlags,
    const EffectPipelineStateDescription& pipelineDescription)
    : EffectBase(device),
        specularMap((effectFlags & EffectFlags::Specular) != 0),
        texture{},
        specular{},
        normal{},
        sampler{}
{
    static_assert(static_cast<int>(std::size(EffectBase<NormalMapEffectTraits>::VertexShaderIndices)) == NormalMapEffectTraits::ShaderPermutationCount, "array/max mismatch");
    static_assert(static_cast<int>(std::size(EffectBase<NormalMapEffectTraits>::VertexShaderBytecode)) == NormalMapEffectTraits::VertexShaderCount, "array/max mismatch");
    static_assert(static_cast<int>(std::size(EffectBase<NormalMapEffectTraits>::PixelShaderBytecode)) == NormalMapEffectTraits::PixelShaderCount, "array/max mismatch");
    static_assert(static_cast<int>(std::size(EffectBase<NormalMapEffectTraits>::PixelShaderIndices)) == NormalMapEffectTraits::ShaderPermutationCount, "array/max mismatch");

    lights.InitializeConstants(constants.specularColorAndPower, constants.lightDirection, constants.lightDiffuseColor, constants.lightSpecularColor);

    // Create root signature.
    {
        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;

        CD3DX12_ROOT_PARAMETER rootParameters[RootParameterIndex::RootParameterCount] = {};
        CD3DX12_DESCRIPTOR_RANGE textureSRV(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
        rootParameters[RootParameterIndex::TextureSRV].InitAsDescriptorTable(1, &textureSRV, D3D12_SHADER_VISIBILITY_PIXEL);

        CD3DX12_DESCRIPTOR_RANGE textureSRV2(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
        rootParameters[RootParameterIndex::TextureNormalSRV].InitAsDescriptorTable(1, &textureSRV2, D3D12_SHADER_VISIBILITY_PIXEL);

        CD3DX12_DESCRIPTOR_RANGE textureSampler(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
        rootParameters[RootParameterIndex::TextureSampler].InitAsDescriptorTable(1, &textureSampler, D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[RootParameterIndex::ConstantBuffer].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);

        CD3DX12_ROOT_SIGNATURE_DESC rsigDesc = {};

        if (specularMap)
        {
            CD3DX12_DESCRIPTOR_RANGE textureSRV3(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);
            rootParameters[RootParameterIndex::TextureSpecularSRV].InitAsDescriptorTable(1, &textureSRV3, D3D12_SHADER_VISIBILITY_PIXEL);

            rsigDesc.Init(static_cast<UINT>(std::size(rootParameters)), rootParameters, 0, nullptr, rootSignatureFlags);

            mRootSignature = GetRootSignature(1, rsigDesc);
        }
        else
        {
            rsigDesc.Init(static_cast<UINT>(std::size(rootParameters) - 1), rootParameters, 0, nullptr, rootSignatureFlags);

            mRootSignature = GetRootSignature(0, rsigDesc);
        }
    }

    assert(mRootSignature != nullptr);

    fog.enabled = (effectFlags & EffectFlags::Fog) != 0;

    // Create pipeline state.
    int sp = GetPipelineStatePermutation(effectFlags);
    assert(sp >= 0 && sp < NormalMapEffectTraits::ShaderPermutationCount);
    _Analysis_assume_(sp >= 0 && sp < NormalMapEffectTraits::ShaderPermutationCount);

    int vi = EffectBase<NormalMapEffectTraits>::VertexShaderIndices[sp];
    assert(vi >= 0 && vi < NormalMapEffectTraits::VertexShaderCount);
    _Analysis_assume_(vi >= 0 && vi < NormalMapEffectTraits::VertexShaderCount);
    int pi = EffectBase<NormalMapEffectTraits>::PixelShaderIndices[sp];
    assert(pi >= 0 && pi < NormalMapEffectTraits::PixelShaderCount);
    _Analysis_assume_(pi >= 0 && pi < NormalMapEffectTraits::PixelShaderCount);

    pipelineDescription.CreatePipelineState(
        device,
        mRootSignature,
        EffectBase<NormalMapEffectTraits>::VertexShaderBytecode[vi],
        EffectBase<NormalMapEffectTraits>::PixelShaderBytecode[pi],
        mPipelineState.GetAddressOf());

    SetDebugObjectName(mPipelineState.Get(), L"NormalMapEffect");
}


int NormalMapEffect::Impl::GetPipelineStatePermutation(uint32_t effectFlags) const noexcept
{
    int permutation = 0;

    // Use optimized shaders if fog is disabled.
    if (!fog.enabled)
    {
        permutation += 1;
    }

    // Support vertex coloring?
    if (effectFlags & EffectFlags::VertexColor)
    {
        permutation += 2;
    }

    if (!specularMap)
    {
        permutation += 4;
    }

    if (effectFlags & EffectFlags::BiasedVertexNormals)
    {
        // Compressed normals need to be scaled and biased in the vertex shader.
        permutation += 8;
    }

    if (effectFlags & EffectFlags::Instancing)
    {
        // Vertex shader needs to use vertex matrix transform.
        permutation += 16;
    }

    return permutation;
}


// Sets our state onto the D3D device.
void NormalMapEffect::Impl::Apply(_In_ ID3D12GraphicsCommandList* commandList)
{
    // Compute derived parameter values.
    matrices.SetConstants(dirtyFlags, constants.worldViewProj);
    fog.SetConstants(dirtyFlags, matrices.worldView, constants.fogVector);
    lights.SetConstants(dirtyFlags, matrices, constants.world, constants.worldInverseTranspose, constants.eyePosition, constants.diffuseColor, constants.emissiveColor, true);

    UpdateConstants();

    // Set the root signature
    commandList->SetGraphicsRootSignature(mRootSignature);

    // Set the texture
    if (!texture.ptr || !sampler.ptr || !normal.ptr)
    {
        DebugTrace("ERROR: Missing texture(s) or sampler for NormalMapEffect (texture %llu, normal %llu, sampler %llu)\n", texture.ptr, normal.ptr, sampler.ptr);
        throw std::runtime_error("NormalMapEffect");
    }

    // **NOTE** If D3D asserts or crashes here, you probably need to call commandList->SetDescriptorHeaps() with the required descriptor heaps.
    commandList->SetGraphicsRootDescriptorTable(RootParameterIndex::TextureSRV, texture);
    commandList->SetGraphicsRootDescriptorTable(RootParameterIndex::TextureNormalSRV, normal);
    commandList->SetGraphicsRootDescriptorTable(RootParameterIndex::TextureSampler, sampler);

    if (specularMap)
    {
        if (!specular.ptr)
        {
            DebugTrace("ERROR: Missing specular texure NormalMapEffect (texture %llu)\n", specular.ptr);
            throw std::runtime_error("NormalMapEffect");
        }
        commandList->SetGraphicsRootDescriptorTable(RootParameterIndex::TextureSpecularSRV, specular);
    }

    // Set constants
    commandList->SetGraphicsRootConstantBufferView(RootParameterIndex::ConstantBuffer, GetConstantBufferGpuAddress());

    // Set the pipeline state
    commandList->SetPipelineState(EffectBase::mPipelineState.Get());
}


// Public constructor.
NormalMapEffect::NormalMapEffect(
    _In_ ID3D12Device* device,
    uint32_t effectFlags,
    const EffectPipelineStateDescription& pipelineDescription)
    : pImpl(std::make_unique<Impl>(device, effectFlags, pipelineDescription))
{
}


// Move constructor.
NormalMapEffect::NormalMapEffect(NormalMapEffect&& moveFrom) noexcept
    : pImpl(std::move(moveFrom.pImpl))
{
}


// Move assignment.
NormalMapEffect& NormalMapEffect::operator= (NormalMapEffect&& moveFrom) noexcept
{
    pImpl = std::move(moveFrom.pImpl);
    return *this;
}


// Public destructor.
NormalMapEffect::~NormalMapEffect()
{
}


// IEffect methods
void NormalMapEffect::Apply(_In_ ID3D12GraphicsCommandList* commandList)
{
    pImpl->Apply(commandList);
}


// Camera settings
void XM_CALLCONV NormalMapEffect::SetWorld(FXMMATRIX value)
{
    pImpl->matrices.world = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj | EffectDirtyFlags::WorldInverseTranspose | EffectDirtyFlags::FogVector;
}


void XM_CALLCONV NormalMapEffect::SetView(FXMMATRIX value)
{
    pImpl->matrices.view = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj | EffectDirtyFlags::EyePosition | EffectDirtyFlags::FogVector;
}


void XM_CALLCONV NormalMapEffect::SetProjection(FXMMATRIX value)
{
    pImpl->matrices.projection = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj;
}


void XM_CALLCONV NormalMapEffect::SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection)
{
    pImpl->matrices.world = world;
    pImpl->matrices.view = view;
    pImpl->matrices.projection = projection;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj | EffectDirtyFlags::WorldInverseTranspose | EffectDirtyFlags::EyePosition | EffectDirtyFlags::FogVector;
}


// Material settings
void XM_CALLCONV NormalMapEffect::SetDiffuseColor(FXMVECTOR value)
{
    pImpl->lights.diffuseColor = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::MaterialColor;
}


void XM_CALLCONV NormalMapEffect::SetEmissiveColor(FXMVECTOR value)
{
    pImpl->lights.emissiveColor = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::MaterialColor;
}


void XM_CALLCONV NormalMapEffect::SetSpecularColor(FXMVECTOR value)
{
    // Set xyz to new value, but preserve existing w (specular power).
    pImpl->constants.specularColorAndPower = XMVectorSelect(pImpl->constants.specularColorAndPower, value, g_XMSelect1110);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void NormalMapEffect::SetSpecularPower(float value)
{
    // Set w to new value, but preserve existing xyz (specular color).
    pImpl->constants.specularColorAndPower = XMVectorSetW(pImpl->constants.specularColorAndPower, value);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void NormalMapEffect::DisableSpecular()
{
    // Set specular color to black, power to 1
    // Note: Don't use a power of 0 or the shader will generate strange highlights on non-specular materials

    pImpl->constants.specularColorAndPower = g_XMIdentityR3; 

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void NormalMapEffect::SetAlpha(float value)
{
    pImpl->lights.alpha = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::MaterialColor;
}


void XM_CALLCONV NormalMapEffect::SetColorAndAlpha(FXMVECTOR value)
{
    pImpl->lights.diffuseColor = value;
    pImpl->lights.alpha = XMVectorGetW(value);

    pImpl->dirtyFlags |= EffectDirtyFlags::MaterialColor;
}


// Light settings
void XM_CALLCONV NormalMapEffect::SetAmbientLightColor(FXMVECTOR value)
{
    pImpl->lights.ambientLightColor = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::MaterialColor;
}


void NormalMapEffect::SetLightEnabled(int whichLight, bool value)
{
    pImpl->dirtyFlags |= pImpl->lights.SetLightEnabled(whichLight, value, pImpl->constants.lightDiffuseColor, pImpl->constants.lightSpecularColor);
}


void XM_CALLCONV NormalMapEffect::SetLightDirection(int whichLight, FXMVECTOR value)
{
    EffectLights::ValidateLightIndex(whichLight);

    pImpl->constants.lightDirection[whichLight] = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void XM_CALLCONV NormalMapEffect::SetLightDiffuseColor(int whichLight, FXMVECTOR value)
{
    pImpl->dirtyFlags |= pImpl->lights.SetLightDiffuseColor(whichLight, value, pImpl->constants.lightDiffuseColor);
}


void XM_CALLCONV NormalMapEffect::SetLightSpecularColor(int whichLight, FXMVECTOR value)
{
    pImpl->dirtyFlags |= pImpl->lights.SetLightSpecularColor(whichLight, value, pImpl->constants.lightSpecularColor);
}


void NormalMapEffect::EnableDefaultLighting()
{
    EffectLights::EnableDefaultLighting(this);
}


// Fog settings.
void NormalMapEffect::SetFogStart(float value)
{
    pImpl->fog.start = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::FogVector;
}


void NormalMapEffect::SetFogEnd(float value)
{
    pImpl->fog.end = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::FogVector;
}


void XM_CALLCONV NormalMapEffect::SetFogColor(FXMVECTOR value)
{
    pImpl->constants.fogColor = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


// Texture settings.
void NormalMapEffect::SetTexture(D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor, D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptor)
{
    pImpl->texture = srvDescriptor;
    pImpl->sampler = samplerDescriptor;
}


void NormalMapEffect::SetNormalTexture(D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor)
{
    pImpl->normal = srvDescriptor;
}


void NormalMapEffect::SetSpecularTexture(D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor)
{
    if (!pImpl->specularMap)
    {
        DebugTrace("WARNING: Specular texture set on NormalMapEffect instance created without specular shader (texture %llu)\n", srvDescriptor.ptr);
    }

    pImpl->specular = srvDescriptor;
}
