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

namespace DirectX
{
    namespace EffectDirtyFlags
    {
        constexpr int ConstantBufferBones = 0x100000;
    }
}

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

    XM_ALIGNED_STRUCT(16) BoneConstants
    {
        XMVECTOR Bones[SkinnedNormalMapEffect::MaxBones][3];
    };

    static_assert((sizeof(BoneConstants) % 16) == 0, "CB size not padded correctly");


    // Traits type describes our characteristics to the EffectBase template.
    struct NormalMapEffectTraits
    {
        using ConstantBufferType = NormalMapEffectConstants;

        static constexpr int VertexShaderCount = 20;
        static constexpr int PixelShaderCount = 4;
        static constexpr int ShaderPermutationCount = 40;
        static constexpr int RootSignatureCount = 2;
    };
}

// Internal NormalMapEffect implementation class.
class NormalMapEffect::Impl : public EffectBase<NormalMapEffectTraits>
{
public:
    explicit Impl(_In_ ID3D12Device* device);

    void Initialize(
        _In_ ID3D12Device* device,
        uint32_t effectFlags,
        const EffectPipelineStateDescription& pipelineDescription,
        bool enableSkinning);

    enum RootParameterIndex
    {
        TextureSRV,
        TextureNormalSRV,
        TextureSampler,
        ConstantBuffer,
        ConstantBufferBones,
        TextureSpecularSRV,
        RootParameterCount
    };

    int weightsPerVertex;
    bool specularMap;

    D3D12_GPU_DESCRIPTOR_HANDLE texture;
    D3D12_GPU_DESCRIPTOR_HANDLE normal;
    D3D12_GPU_DESCRIPTOR_HANDLE specular;
    D3D12_GPU_DESCRIPTOR_HANDLE sampler;

    EffectLights lights;

    int GetPipelineStatePermutation(uint32_t effectFlags) const noexcept;

    void Apply(_In_ ID3D12GraphicsCommandList* commandList);

    BoneConstants boneConstants;

private:
    GraphicsResource mBones;
};


#pragma region Shaders
// Include the precompiled shader code.
namespace
{
#ifdef _GAMING_XBOX_SCARLETT
#include "XboxGamingScarlettNormalMapEffect_VSNormalPixelLightingTx.inc"
#include "XboxGamingScarlettNormalMapEffect_VSNormalPixelLightingTxVc.inc"
#include "XboxGamingScarlettNormalMapEffect_VSNormalPixelLightingTxNoSpec.inc"
#include "XboxGamingScarlettNormalMapEffect_VSNormalPixelLightingTxVcNoSpec.inc"

#include "XboxGamingScarlettNormalMapEffect_VSNormalPixelLightingTxBn.inc"
#include "XboxGamingScarlettNormalMapEffect_VSNormalPixelLightingTxVcBn.inc"
#include "XboxGamingScarlettNormalMapEffect_VSNormalPixelLightingTxNoSpecBn.inc"
#include "XboxGamingScarlettNormalMapEffect_VSNormalPixelLightingTxVcNoSpecBn.inc"

#include "XboxGamingScarlettNormalMapEffect_VSNormalPixelLightingTxInst.inc"
#include "XboxGamingScarlettNormalMapEffect_VSNormalPixelLightingTxVcInst.inc"
#include "XboxGamingScarlettNormalMapEffect_VSNormalPixelLightingTxNoSpecInst.inc"
#include "XboxGamingScarlettNormalMapEffect_VSNormalPixelLightingTxVcNoSpecInst.inc"

#include "XboxGamingScarlettNormalMapEffect_VSNormalPixelLightingTxBnInst.inc"
#include "XboxGamingScarlettNormalMapEffect_VSNormalPixelLightingTxVcBnInst.inc"
#include "XboxGamingScarlettNormalMapEffect_VSNormalPixelLightingTxNoSpecBnInst.inc"
#include "XboxGamingScarlettNormalMapEffect_VSNormalPixelLightingTxVcNoSpecBnInst.inc"

#include "XboxGamingScarlettNormalMapEffect_VSSkinnedPixelLightingTx.inc"
#include "XboxGamingScarlettNormalMapEffect_VSSkinnedPixelLightingTxBn.inc"
#include "XboxGamingScarlettNormalMapEffect_VSSkinnedPixelLightingTxNoSpec.inc"
#include "XboxGamingScarlettNormalMapEffect_VSSkinnedPixelLightingTxNoSpecBn.inc"

#include "XboxGamingScarlettNormalMapEffect_PSNormalPixelLightingTx.inc"
#include "XboxGamingScarlettNormalMapEffect_PSNormalPixelLightingTxNoFog.inc"
#include "XboxGamingScarlettNormalMapEffect_PSNormalPixelLightingTxNoSpec.inc"
#include "XboxGamingScarlettNormalMapEffect_PSNormalPixelLightingTxNoFogSpec.inc"
#elif defined(_GAMING_XBOX)
#include "XboxGamingXboxOneNormalMapEffect_VSNormalPixelLightingTx.inc"
#include "XboxGamingXboxOneNormalMapEffect_VSNormalPixelLightingTxVc.inc"
#include "XboxGamingXboxOneNormalMapEffect_VSNormalPixelLightingTxNoSpec.inc"
#include "XboxGamingXboxOneNormalMapEffect_VSNormalPixelLightingTxVcNoSpec.inc"

#include "XboxGamingXboxOneNormalMapEffect_VSNormalPixelLightingTxBn.inc"
#include "XboxGamingXboxOneNormalMapEffect_VSNormalPixelLightingTxVcBn.inc"
#include "XboxGamingXboxOneNormalMapEffect_VSNormalPixelLightingTxNoSpecBn.inc"
#include "XboxGamingXboxOneNormalMapEffect_VSNormalPixelLightingTxVcNoSpecBn.inc"

#include "XboxGamingXboxOneNormalMapEffect_VSNormalPixelLightingTxInst.inc"
#include "XboxGamingXboxOneNormalMapEffect_VSNormalPixelLightingTxVcInst.inc"
#include "XboxGamingXboxOneNormalMapEffect_VSNormalPixelLightingTxNoSpecInst.inc"
#include "XboxGamingXboxOneNormalMapEffect_VSNormalPixelLightingTxVcNoSpecInst.inc"

#include "XboxGamingXboxOneNormalMapEffect_VSNormalPixelLightingTxBnInst.inc"
#include "XboxGamingXboxOneNormalMapEffect_VSNormalPixelLightingTxVcBnInst.inc"
#include "XboxGamingXboxOneNormalMapEffect_VSNormalPixelLightingTxNoSpecBnInst.inc"
#include "XboxGamingXboxOneNormalMapEffect_VSNormalPixelLightingTxVcNoSpecBnInst.inc"

#include "XboxGamingXboxOneNormalMapEffect_VSSkinnedPixelLightingTx.inc"
#include "XboxGamingXboxOneNormalMapEffect_VSSkinnedPixelLightingTxBn.inc"
#include "XboxGamingXboxOneNormalMapEffect_VSSkinnedPixelLightingTxNoSpec.inc"
#include "XboxGamingXboxOneNormalMapEffect_VSSkinnedPixelLightingTxNoSpecBn.inc"

#include "XboxGamingXboxOneNormalMapEffect_PSNormalPixelLightingTx.inc"
#include "XboxGamingXboxOneNormalMapEffect_PSNormalPixelLightingTxNoFog.inc"
#include "XboxGamingXboxOneNormalMapEffect_PSNormalPixelLightingTxNoSpec.inc"
#include "XboxGamingXboxOneNormalMapEffect_PSNormalPixelLightingTxNoFogSpec.inc"
#elif defined(_XBOX_ONE) && defined(_TITLE)
#include "XboxOneNormalMapEffect_VSNormalPixelLightingTx.inc"
#include "XboxOneNormalMapEffect_VSNormalPixelLightingTxVc.inc"
#include "XboxOneNormalMapEffect_VSNormalPixelLightingTxNoSpec.inc"
#include "XboxOneNormalMapEffect_VSNormalPixelLightingTxVcNoSpec.inc"

#include "XboxOneNormalMapEffect_VSNormalPixelLightingTxBn.inc"
#include "XboxOneNormalMapEffect_VSNormalPixelLightingTxVcBn.inc"
#include "XboxOneNormalMapEffect_VSNormalPixelLightingTxNoSpecBn.inc"
#include "XboxOneNormalMapEffect_VSNormalPixelLightingTxVcNoSpecBn.inc"

#include "XboxOneNormalMapEffect_VSNormalPixelLightingTxInst.inc"
#include "XboxOneNormalMapEffect_VSNormalPixelLightingTxVcInst.inc"
#include "XboxOneNormalMapEffect_VSNormalPixelLightingTxNoSpecInst.inc"
#include "XboxOneNormalMapEffect_VSNormalPixelLightingTxVcNoSpecInst.inc"

#include "XboxOneNormalMapEffect_VSNormalPixelLightingTxBnInst.inc"
#include "XboxOneNormalMapEffect_VSNormalPixelLightingTxVcBnInst.inc"
#include "XboxOneNormalMapEffect_VSNormalPixelLightingTxNoSpecBnInst.inc"
#include "XboxOneNormalMapEffect_VSNormalPixelLightingTxVcNoSpecBnInst.inc"

#include "XboxOneNormalMapEffect_VSSkinnedPixelLightingTx.inc"
#include "XboxOneNormalMapEffect_VSSkinnedPixelLightingTxBn.inc"
#include "XboxOneNormalMapEffect_VSSkinnedPixelLightingTxNoSpec.inc"
#include "XboxOneNormalMapEffect_VSSkinnedPixelLightingTxNoSpecBn.inc"

#include "XboxOneNormalMapEffect_PSNormalPixelLightingTx.inc"
#include "XboxOneNormalMapEffect_PSNormalPixelLightingTxNoFog.inc"
#include "XboxOneNormalMapEffect_PSNormalPixelLightingTxNoSpec.inc"
#include "XboxOneNormalMapEffect_PSNormalPixelLightingTxNoFogSpec.inc"
#else
#include "NormalMapEffect_VSNormalPixelLightingTx.inc"
#include "NormalMapEffect_VSNormalPixelLightingTxVc.inc"
#include "NormalMapEffect_VSNormalPixelLightingTxNoSpec.inc"
#include "NormalMapEffect_VSNormalPixelLightingTxVcNoSpec.inc"

#include "NormalMapEffect_VSNormalPixelLightingTxBn.inc"
#include "NormalMapEffect_VSNormalPixelLightingTxVcBn.inc"
#include "NormalMapEffect_VSNormalPixelLightingTxNoSpecBn.inc"
#include "NormalMapEffect_VSNormalPixelLightingTxVcNoSpecBn.inc"

#include "NormalMapEffect_VSNormalPixelLightingTxInst.inc"
#include "NormalMapEffect_VSNormalPixelLightingTxVcInst.inc"
#include "NormalMapEffect_VSNormalPixelLightingTxNoSpecInst.inc"
#include "NormalMapEffect_VSNormalPixelLightingTxVcNoSpecInst.inc"

#include "NormalMapEffect_VSNormalPixelLightingTxBnInst.inc"
#include "NormalMapEffect_VSNormalPixelLightingTxVcBnInst.inc"
#include "NormalMapEffect_VSNormalPixelLightingTxNoSpecBnInst.inc"
#include "NormalMapEffect_VSNormalPixelLightingTxVcNoSpecBnInst.inc"

#include "NormalMapEffect_VSSkinnedPixelLightingTx.inc"
#include "NormalMapEffect_VSSkinnedPixelLightingTxBn.inc"
#include "NormalMapEffect_VSSkinnedPixelLightingTxNoSpec.inc"
#include "NormalMapEffect_VSSkinnedPixelLightingTxNoSpecBn.inc"

#include "NormalMapEffect_PSNormalPixelLightingTx.inc"
#include "NormalMapEffect_PSNormalPixelLightingTxNoFog.inc"
#include "NormalMapEffect_PSNormalPixelLightingTxNoSpec.inc"
#include "NormalMapEffect_PSNormalPixelLightingTxNoFogSpec.inc"
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

    { NormalMapEffect_VSSkinnedPixelLightingTx,              sizeof(NormalMapEffect_VSSkinnedPixelLightingTx)              },
    { NormalMapEffect_VSSkinnedPixelLightingTxBn,            sizeof(NormalMapEffect_VSSkinnedPixelLightingTxBn)            },
    { NormalMapEffect_VSSkinnedPixelLightingTxNoSpec,        sizeof(NormalMapEffect_VSSkinnedPixelLightingTxNoSpec)        },
    { NormalMapEffect_VSSkinnedPixelLightingTxNoSpecBn,      sizeof(NormalMapEffect_VSSkinnedPixelLightingTxNoSpecBn)      },
};


template<>
const int EffectBase<NormalMapEffectTraits>::VertexShaderIndices[] =
{
    0,     // pixel lighting + texture
    0,     // pixel lighting + texture, no fog
    4,     // pixel lighting + texture, no specular
    4,     // pixel lighting + texture, no fog or specular

    2,     // pixel lighting (biased vertex normal) + texture
    2,     // pixel lighting (biased vertex normal) + texture, no fog
    6,     // pixel lighting (biased vertex normal) + texture, no specular
    6,     // pixel lighting (biased vertex normal) + texture, no fog or specular

    1,     // pixel lighting + texture + vertex color
    1,     // pixel lighting + texture + vertex color, no fog
    5,     // pixel lighting + texture + vertex color, no specular
    5,     // pixel lighting + texture + vertex color, no fog or specular

    3,     // pixel lighting (biased vertex normal) + texture + vertex color
    3,     // pixel lighting (biased vertex normal) + texture + vertex color, no fog
    7,     // pixel lighting (biased vertex normal) + texture + vertex color, no specular
    7,     // pixel lighting (biased vertex normal) + texture + vertex color, no fog or specular

    8,     // instancing + pixel lighting + texture
    8,     // instancing + pixel lighting + texture, no fog
    12,    // instancing + pixel lighting + texture, no specular
    12,    // instancing + pixel lighting + texture, no fog or specular

    10,    // instancing + pixel lighting (biased vertex normal) + texture
    10,    // instancing + pixel lighting (biased vertex normal) + texture, no fog
    14,    // instancing + pixel lighting (biased vertex normal) + texture, no specular
    14,    // instancing + pixel lighting (biased vertex normal) + texture, no fog or specular

    9,     // instancing + pixel lighting + texture + vertex color
    9,     // instancing + pixel lighting + texture + vertex color, no fog
    13,    // instancing + pixel lighting + texture + vertex color, no specular
    13,    // instancing + pixel lighting + texture + vertex color, no fog or specular

    11,    // instancing + pixel lighting (biased vertex normal) + texture + vertex color
    11,    // instancing + pixel lighting (biased vertex normal) + texture + vertex color, no fog
    15,    // instancing + pixel lighting (biased vertex normal) + texture + vertex color, no specular
    15,    // instancing + pixel lighting (biased vertex normal) + texture + vertex color, no fog or specular

    16,    // skinning + pixel lighting + texture
    16,    // skinning + pixel lighting + texture, no fog
    18,    // skinning + pixel lighting + texture, no specular
    18,    // skinning + pixel lighting + texture, no fog or specular

    17,    // skinning + pixel lighting (biased vertex normal) + texture
    17,    // skinning + pixel lighting (biased vertex normal) + texture, no fog
    19,    // skinning + pixel lighting (biased vertex normal) + texture, no specular
    19,    // skinning + pixel lighting (biased vertex normal) + texture, no fog or specular
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
    2,     // pixel lighting + texture, no specular
    3,     // pixel lighting + texture, no fog or specular

    0,     // pixel lighting (biased vertex normal) + texture
    1,     // pixel lighting (biased vertex normal) + texture, no fog
    2,     // pixel lighting (biased vertex normal) + texture, no specular
    3,     // pixel lighting (biased vertex normal) + texture, no fog or specular

    0,     // pixel lighting + texture + vertex color
    1,     // pixel lighting + texture + vertex color, no fog
    2,     // pixel lighting + texture + vertex color, no specular
    3,     // pixel lighting + texture + vertex color, no fog or specular

    0,     // pixel lighting (biased vertex normal) + texture + vertex color
    1,     // pixel lighting (biased vertex normal) + texture + vertex color, no fog
    2,     // pixel lighting (biased vertex normal) + texture + vertex color, no specular
    3,     // pixel lighting (biased vertex normal) + texture + vertex color, no fog or specular

    0,     // instancing + pixel lighting + texture
    1,     // instancing + pixel lighting + texture, no fog
    2,     // instancing + pixel lighting + texture, no specular
    3,     // instancing + pixel lighting + texture, no fog or specular

    0,     // instancing + pixel lighting (biased vertex normal) + texture
    1,     // instancing + pixel lighting (biased vertex normal) + texture, no fog
    2,     // instancing + pixel lighting (biased vertex normal) + texture, no specular
    3,     // instancing + pixel lighting (biased vertex normal) + texture, no fog or specular

    0,     // instancing + pixel lighting + texture + vertex color
    1,     // instancing + pixel lighting + texture + vertex color, no fog
    2,     // instancing + pixel lighting + texture + vertex color, no specular
    3,     // instancing + pixel lighting + texture + vertex color, no fog or specular

    0,     // instancing + pixel lighting (biased vertex normal) + texture + vertex color
    1,     // instancing + pixel lighting (biased vertex normal) + texture + vertex color, no fog
    2,     // instancing + pixel lighting (biased vertex normal) + texture + vertex color, no specular
    3,     // instancing + pixel lighting (biased vertex normal) + texture + vertex color, no fog or specular

    0,      // skinning + pixel lighting + texture
    1,      // skinning + pixel lighting + texture, no fog
    2,      // skinning + pixel lighting + texture, no specular
    3,      // skinning + pixel lighting + texture, no fog or specular

    0,      // skinning + pixel lighting (biased vertex normal) + texture
    1,      // skinning + pixel lighting (biased vertex normal) + texture, no fog
    2,      // skinning + pixel lighting (biased vertex normal) + texture, no specular
    3,      // skinning + pixel lighting (biased vertex normal) + texture, no fog or specular
};
#pragma endregion

// Global pool of per-device NormalMapEffect resources.
template<>
SharedResourcePool<ID3D12Device*, EffectBase<NormalMapEffectTraits>::DeviceResources> EffectBase<NormalMapEffectTraits>::deviceResourcesPool = {};


// Constructor.
NormalMapEffect::Impl::Impl(
    _In_ ID3D12Device* device)
    : EffectBase(device),
    weightsPerVertex(0),
    specularMap(false),
    texture{},
    normal{},
    specular{},
    sampler{},
    boneConstants{}
{
    static_assert(static_cast<int>(std::size(EffectBase<NormalMapEffectTraits>::VertexShaderIndices)) == NormalMapEffectTraits::ShaderPermutationCount, "array/max mismatch");
    static_assert(static_cast<int>(std::size(EffectBase<NormalMapEffectTraits>::VertexShaderBytecode)) == NormalMapEffectTraits::VertexShaderCount, "array/max mismatch");
    static_assert(static_cast<int>(std::size(EffectBase<NormalMapEffectTraits>::PixelShaderBytecode)) == NormalMapEffectTraits::PixelShaderCount, "array/max mismatch");
    static_assert(static_cast<int>(std::size(EffectBase<NormalMapEffectTraits>::PixelShaderIndices)) == NormalMapEffectTraits::ShaderPermutationCount, "array/max mismatch");
}

void NormalMapEffect::Impl::Initialize(
    _In_ ID3D12Device* device,
    uint32_t effectFlags,
    const EffectPipelineStateDescription& pipelineDescription,
    bool enableSkinning)
{
    specularMap = (effectFlags & EffectFlags::Specular) != 0;

    lights.InitializeConstants(constants.specularColorAndPower, constants.lightDirection, constants.lightDiffuseColor, constants.lightSpecularColor);

    if (enableSkinning)
    {
        if (effectFlags & EffectFlags::VertexColor)
        {
            DebugTrace("ERROR: SkinnedNormalMapEffect does not implement EffectFlags::VertexColor\n");
            throw std::invalid_argument("VertexColor effect flag is invalid");
        }
        else if (effectFlags & EffectFlags::Instancing)
        {
            DebugTrace("ERROR: SkinnedNormalMapEffect does not implement EffectFlags::Instancing\n");
            throw std::invalid_argument("Instancing effect flag is invalid");
        }

        weightsPerVertex = 4;

        for (size_t j = 0; j < SkinnedNormalMapEffect::MaxBones; ++j)
        {
            boneConstants.Bones[j][0] = g_XMIdentityR0;
            boneConstants.Bones[j][1] = g_XMIdentityR1;
            boneConstants.Bones[j][2] = g_XMIdentityR2;
        }
    }

    // Create root signature.
    {
        ENUM_FLAGS_CONSTEXPR D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
            | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
            | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
            | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;

        CD3DX12_ROOT_PARAMETER rootParameters[RootParameterIndex::RootParameterCount] = {};
        const CD3DX12_DESCRIPTOR_RANGE textureSRV(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
        rootParameters[RootParameterIndex::TextureSRV].InitAsDescriptorTable(1, &textureSRV, D3D12_SHADER_VISIBILITY_PIXEL);

        const CD3DX12_DESCRIPTOR_RANGE textureSRV2(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
        rootParameters[RootParameterIndex::TextureNormalSRV].InitAsDescriptorTable(1, &textureSRV2, D3D12_SHADER_VISIBILITY_PIXEL);

        const CD3DX12_DESCRIPTOR_RANGE textureSampler(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
        rootParameters[RootParameterIndex::TextureSampler].InitAsDescriptorTable(1, &textureSampler, D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[RootParameterIndex::ConstantBuffer].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
        rootParameters[RootParameterIndex::ConstantBufferBones].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_VERTEX);

        CD3DX12_ROOT_SIGNATURE_DESC rsigDesc = {};

        if (specularMap)
        {
            const CD3DX12_DESCRIPTOR_RANGE textureSRV3(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);
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
    const int sp = GetPipelineStatePermutation(effectFlags);
    assert(sp >= 0 && sp < NormalMapEffectTraits::ShaderPermutationCount);
    _Analysis_assume_(sp >= 0 && sp < NormalMapEffectTraits::ShaderPermutationCount);

    const int vi = EffectBase<NormalMapEffectTraits>::VertexShaderIndices[sp];
    assert(vi >= 0 && vi < NormalMapEffectTraits::VertexShaderCount);
    _Analysis_assume_(vi >= 0 && vi < NormalMapEffectTraits::VertexShaderCount);
    const int pi = EffectBase<NormalMapEffectTraits>::PixelShaderIndices[sp];
    assert(pi >= 0 && pi < NormalMapEffectTraits::PixelShaderCount);
    _Analysis_assume_(pi >= 0 && pi < NormalMapEffectTraits::PixelShaderCount);

    pipelineDescription.CreatePipelineState(
        device,
        mRootSignature,
        EffectBase<NormalMapEffectTraits>::VertexShaderBytecode[vi],
        EffectBase<NormalMapEffectTraits>::PixelShaderBytecode[pi],
        mPipelineState.GetAddressOf());

    if (enableSkinning)
    {
        SetDebugObjectName(mPipelineState.Get(), L"SkinnedNormalMapEffect");
    }
    else
    {
        SetDebugObjectName(mPipelineState.Get(), L"NormalMapEffect");
    }
}


int NormalMapEffect::Impl::GetPipelineStatePermutation(uint32_t effectFlags) const noexcept
{
    int permutation = 0;

    // Use optimized shaders if fog is disabled.
    if (!fog.enabled)
    {
        permutation += 1;
    }

    if (!specularMap)
    {
        permutation += 2;
    }

    if (effectFlags & EffectFlags::BiasedVertexNormals)
    {
        // Compressed normals need to be scaled and biased in the vertex shader.
        permutation += 4;
    }

    if (weightsPerVertex > 0)
    {
        // Vertex skinning.
        permutation += 32;
    }
    else
    {
        // Support vertex coloring?
        if (effectFlags & EffectFlags::VertexColor)
        {
            permutation += 8;
        }

        if (effectFlags & EffectFlags::Instancing)
        {
            // Vertex shader needs to use vertex matrix transform.
            permutation += 16;
        }
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

    if (weightsPerVertex > 0)
    {
        if (dirtyFlags & EffectDirtyFlags::ConstantBufferBones)
        {
            mBones = GraphicsMemory::Get(GetDevice()).AllocateConstant(boneConstants);
            dirtyFlags &= ~EffectDirtyFlags::ConstantBufferBones;
        }
    }

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
    auto const cbuffer = GetConstantBufferGpuAddress();
    commandList->SetGraphicsRootConstantBufferView(RootParameterIndex::ConstantBuffer, cbuffer);
    commandList->SetGraphicsRootConstantBufferView(RootParameterIndex::ConstantBufferBones,
        (weightsPerVertex > 0) ? mBones.GpuAddress() : cbuffer);

    // Set the pipeline state
    commandList->SetPipelineState(EffectBase::mPipelineState.Get());
}


//--------------------------------------------------------------------------------------
// NormalMapEffect
//--------------------------------------------------------------------------------------

NormalMapEffect::NormalMapEffect(
    _In_ ID3D12Device* device,
    uint32_t effectFlags,
    const EffectPipelineStateDescription& pipelineDescription,
    bool skinningEnabled)
    : pImpl(std::make_unique<Impl>(device))
{
    pImpl->Initialize(device, effectFlags, pipelineDescription, skinningEnabled);
}

NormalMapEffect::NormalMapEffect(NormalMapEffect&&) noexcept = default;
NormalMapEffect& NormalMapEffect::operator= (NormalMapEffect&&) noexcept = default;
NormalMapEffect::~NormalMapEffect() = default;


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


//--------------------------------------------------------------------------------------
// SkinnedNormalMapEffect
//--------------------------------------------------------------------------------------

// Animation settings.
void SkinnedNormalMapEffect::SetBoneTransforms(_In_reads_(count) XMMATRIX const* value, size_t count)
{
    if (count > MaxBones)
        throw std::invalid_argument("count parameter exceeds MaxBones");

    auto boneConstant = pImpl->boneConstants.Bones;

    for (size_t i = 0; i < count; i++)
    {
    #if DIRECTX_MATH_VERSION >= 313
        XMStoreFloat3x4A(reinterpret_cast<XMFLOAT3X4A*>(&boneConstant[i]), value[i]);
    #else
            // Xbox One XDK has an older version of DirectXMath
        XMMATRIX boneMatrix = XMMatrixTranspose(value[i]);

        boneConstant[i][0] = boneMatrix.r[0];
        boneConstant[i][1] = boneMatrix.r[1];
        boneConstant[i][2] = boneMatrix.r[2];
    #endif
    }

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBufferBones;
}


void SkinnedNormalMapEffect::ResetBoneTransforms()
{
    auto boneConstant = pImpl->boneConstants.Bones;

    for (size_t i = 0; i < MaxBones; ++i)
    {
        boneConstant[i][0] = g_XMIdentityR0;
        boneConstant[i][1] = g_XMIdentityR1;
        boneConstant[i][2] = g_XMIdentityR2;
    }

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBufferBones;
}
