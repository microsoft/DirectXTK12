//--------------------------------------------------------------------------------------
// File: BasicEffect.cpp
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
    struct BasicEffectConstants
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

    static_assert((sizeof(BasicEffectConstants) % 16) == 0, "CB size not padded correctly");


    // Traits type describes our characteristics to the EffectBase template.
    struct BasicEffectTraits
    {
        using ConstantBufferType = BasicEffectConstants;

        static constexpr int VertexShaderCount = 24;
        static constexpr int PixelShaderCount = 10;
        static constexpr int ShaderPermutationCount = 40;
        static constexpr int RootSignatureCount = 2;
    };
}

// Internal BasicEffect implementation class.
class BasicEffect::Impl : public EffectBase<BasicEffectTraits>
{
public:
    Impl(_In_ ID3D12Device* device, uint32_t effectFlags, const EffectPipelineStateDescription& pipelineDescription);

    enum RootParameterIndex
    {
        ConstantBuffer,
        TextureSRV,
        TextureSampler,
        RootParameterCount
    };

    bool lightingEnabled;
    bool textureEnabled;

    D3D12_GPU_DESCRIPTOR_HANDLE texture;
    D3D12_GPU_DESCRIPTOR_HANDLE sampler;

    EffectLights lights;

    int GetPipelineStatePermutation(uint32_t effectFlags) const noexcept;

    void Apply(_In_ ID3D12GraphicsCommandList* commandList);
};


#pragma region Shaders
// Include the precompiled shader code.
namespace
{
#ifdef _GAMING_XBOX_SCARLETT
#include "XboxGamingScarlettBasicEffect_VSBasic.inc"
#include "XboxGamingScarlettBasicEffect_VSBasicNoFog.inc"
#include "XboxGamingScarlettBasicEffect_VSBasicVc.inc"
#include "XboxGamingScarlettBasicEffect_VSBasicVcNoFog.inc"
#include "XboxGamingScarlettBasicEffect_VSBasicTx.inc"
#include "XboxGamingScarlettBasicEffect_VSBasicTxNoFog.inc"
#include "XboxGamingScarlettBasicEffect_VSBasicTxVc.inc"
#include "XboxGamingScarlettBasicEffect_VSBasicTxVcNoFog.inc"

#include "XboxGamingScarlettBasicEffect_VSBasicVertexLighting.inc"
#include "XboxGamingScarlettBasicEffect_VSBasicVertexLightingVc.inc"
#include "XboxGamingScarlettBasicEffect_VSBasicVertexLightingTx.inc"
#include "XboxGamingScarlettBasicEffect_VSBasicVertexLightingTxVc.inc"

#include "XboxGamingScarlettBasicEffect_VSBasicPixelLighting.inc"
#include "XboxGamingScarlettBasicEffect_VSBasicPixelLightingVc.inc"
#include "XboxGamingScarlettBasicEffect_VSBasicPixelLightingTx.inc"
#include "XboxGamingScarlettBasicEffect_VSBasicPixelLightingTxVc.inc"

#include "XboxGamingScarlettBasicEffect_VSBasicVertexLightingBn.inc"
#include "XboxGamingScarlettBasicEffect_VSBasicVertexLightingVcBn.inc"
#include "XboxGamingScarlettBasicEffect_VSBasicVertexLightingTxBn.inc"
#include "XboxGamingScarlettBasicEffect_VSBasicVertexLightingTxVcBn.inc"

#include "XboxGamingScarlettBasicEffect_VSBasicPixelLightingBn.inc"
#include "XboxGamingScarlettBasicEffect_VSBasicPixelLightingVcBn.inc"
#include "XboxGamingScarlettBasicEffect_VSBasicPixelLightingTxBn.inc"
#include "XboxGamingScarlettBasicEffect_VSBasicPixelLightingTxVcBn.inc"

#include "XboxGamingScarlettBasicEffect_PSBasic.inc"
#include "XboxGamingScarlettBasicEffect_PSBasicNoFog.inc"
#include "XboxGamingScarlettBasicEffect_PSBasicTx.inc"
#include "XboxGamingScarlettBasicEffect_PSBasicTxNoFog.inc"

#include "XboxGamingScarlettBasicEffect_PSBasicVertexLighting.inc"
#include "XboxGamingScarlettBasicEffect_PSBasicVertexLightingNoFog.inc"
#include "XboxGamingScarlettBasicEffect_PSBasicVertexLightingTx.inc"
#include "XboxGamingScarlettBasicEffect_PSBasicVertexLightingTxNoFog.inc"

#include "XboxGamingScarlettBasicEffect_PSBasicPixelLighting.inc"
#include "XboxGamingScarlettBasicEffect_PSBasicPixelLightingTx.inc"
#elif defined(_GAMING_XBOX)
#include "XboxGamingXboxOneBasicEffect_VSBasic.inc"
#include "XboxGamingXboxOneBasicEffect_VSBasicNoFog.inc"
#include "XboxGamingXboxOneBasicEffect_VSBasicVc.inc"
#include "XboxGamingXboxOneBasicEffect_VSBasicVcNoFog.inc"
#include "XboxGamingXboxOneBasicEffect_VSBasicTx.inc"
#include "XboxGamingXboxOneBasicEffect_VSBasicTxNoFog.inc"
#include "XboxGamingXboxOneBasicEffect_VSBasicTxVc.inc"
#include "XboxGamingXboxOneBasicEffect_VSBasicTxVcNoFog.inc"

#include "XboxGamingXboxOneBasicEffect_VSBasicVertexLighting.inc"
#include "XboxGamingXboxOneBasicEffect_VSBasicVertexLightingVc.inc"
#include "XboxGamingXboxOneBasicEffect_VSBasicVertexLightingTx.inc"
#include "XboxGamingXboxOneBasicEffect_VSBasicVertexLightingTxVc.inc"

#include "XboxGamingXboxOneBasicEffect_VSBasicPixelLighting.inc"
#include "XboxGamingXboxOneBasicEffect_VSBasicPixelLightingVc.inc"
#include "XboxGamingXboxOneBasicEffect_VSBasicPixelLightingTx.inc"
#include "XboxGamingXboxOneBasicEffect_VSBasicPixelLightingTxVc.inc"

#include "XboxGamingXboxOneBasicEffect_VSBasicVertexLightingBn.inc"
#include "XboxGamingXboxOneBasicEffect_VSBasicVertexLightingVcBn.inc"
#include "XboxGamingXboxOneBasicEffect_VSBasicVertexLightingTxBn.inc"
#include "XboxGamingXboxOneBasicEffect_VSBasicVertexLightingTxVcBn.inc"

#include "XboxGamingXboxOneBasicEffect_VSBasicPixelLightingBn.inc"
#include "XboxGamingXboxOneBasicEffect_VSBasicPixelLightingVcBn.inc"
#include "XboxGamingXboxOneBasicEffect_VSBasicPixelLightingTxBn.inc"
#include "XboxGamingXboxOneBasicEffect_VSBasicPixelLightingTxVcBn.inc"

#include "XboxGamingXboxOneBasicEffect_PSBasic.inc"
#include "XboxGamingXboxOneBasicEffect_PSBasicNoFog.inc"
#include "XboxGamingXboxOneBasicEffect_PSBasicTx.inc"
#include "XboxGamingXboxOneBasicEffect_PSBasicTxNoFog.inc"

#include "XboxGamingXboxOneBasicEffect_PSBasicVertexLighting.inc"
#include "XboxGamingXboxOneBasicEffect_PSBasicVertexLightingNoFog.inc"
#include "XboxGamingXboxOneBasicEffect_PSBasicVertexLightingTx.inc"
#include "XboxGamingXboxOneBasicEffect_PSBasicVertexLightingTxNoFog.inc"

#include "XboxGamingXboxOneBasicEffect_PSBasicPixelLighting.inc"
#include "XboxGamingXboxOneBasicEffect_PSBasicPixelLightingTx.inc"
#elif defined(_XBOX_ONE) && defined(_TITLE)
#include "XboxOneBasicEffect_VSBasic.inc"
#include "XboxOneBasicEffect_VSBasicNoFog.inc"
#include "XboxOneBasicEffect_VSBasicVc.inc"
#include "XboxOneBasicEffect_VSBasicVcNoFog.inc"
#include "XboxOneBasicEffect_VSBasicTx.inc"
#include "XboxOneBasicEffect_VSBasicTxNoFog.inc"
#include "XboxOneBasicEffect_VSBasicTxVc.inc"
#include "XboxOneBasicEffect_VSBasicTxVcNoFog.inc"

#include "XboxOneBasicEffect_VSBasicVertexLighting.inc"
#include "XboxOneBasicEffect_VSBasicVertexLightingVc.inc"
#include "XboxOneBasicEffect_VSBasicVertexLightingTx.inc"
#include "XboxOneBasicEffect_VSBasicVertexLightingTxVc.inc"

#include "XboxOneBasicEffect_VSBasicPixelLighting.inc"
#include "XboxOneBasicEffect_VSBasicPixelLightingVc.inc"
#include "XboxOneBasicEffect_VSBasicPixelLightingTx.inc"
#include "XboxOneBasicEffect_VSBasicPixelLightingTxVc.inc"

#include "XboxOneBasicEffect_VSBasicVertexLightingBn.inc"
#include "XboxOneBasicEffect_VSBasicVertexLightingVcBn.inc"
#include "XboxOneBasicEffect_VSBasicVertexLightingTxBn.inc"
#include "XboxOneBasicEffect_VSBasicVertexLightingTxVcBn.inc"

#include "XboxOneBasicEffect_VSBasicPixelLightingBn.inc"
#include "XboxOneBasicEffect_VSBasicPixelLightingVcBn.inc"
#include "XboxOneBasicEffect_VSBasicPixelLightingTxBn.inc"
#include "XboxOneBasicEffect_VSBasicPixelLightingTxVcBn.inc"

#include "XboxOneBasicEffect_PSBasic.inc"
#include "XboxOneBasicEffect_PSBasicNoFog.inc"
#include "XboxOneBasicEffect_PSBasicTx.inc"
#include "XboxOneBasicEffect_PSBasicTxNoFog.inc"

#include "XboxOneBasicEffect_PSBasicVertexLighting.inc"
#include "XboxOneBasicEffect_PSBasicVertexLightingNoFog.inc"
#include "XboxOneBasicEffect_PSBasicVertexLightingTx.inc"
#include "XboxOneBasicEffect_PSBasicVertexLightingTxNoFog.inc"

#include "XboxOneBasicEffect_PSBasicPixelLighting.inc"
#include "XboxOneBasicEffect_PSBasicPixelLightingTx.inc"
#else
#include "BasicEffect_VSBasic.inc"
#include "BasicEffect_VSBasicNoFog.inc"
#include "BasicEffect_VSBasicVc.inc"
#include "BasicEffect_VSBasicVcNoFog.inc"
#include "BasicEffect_VSBasicTx.inc"
#include "BasicEffect_VSBasicTxNoFog.inc"
#include "BasicEffect_VSBasicTxVc.inc"
#include "BasicEffect_VSBasicTxVcNoFog.inc"

#include "BasicEffect_VSBasicVertexLighting.inc"
#include "BasicEffect_VSBasicVertexLightingVc.inc"
#include "BasicEffect_VSBasicVertexLightingTx.inc"
#include "BasicEffect_VSBasicVertexLightingTxVc.inc"

#include "BasicEffect_VSBasicPixelLighting.inc"
#include "BasicEffect_VSBasicPixelLightingVc.inc"
#include "BasicEffect_VSBasicPixelLightingTx.inc"
#include "BasicEffect_VSBasicPixelLightingTxVc.inc"

#include "BasicEffect_VSBasicVertexLightingBn.inc"
#include "BasicEffect_VSBasicVertexLightingVcBn.inc"
#include "BasicEffect_VSBasicVertexLightingTxBn.inc"
#include "BasicEffect_VSBasicVertexLightingTxVcBn.inc"

#include "BasicEffect_VSBasicPixelLightingBn.inc"
#include "BasicEffect_VSBasicPixelLightingVcBn.inc"
#include "BasicEffect_VSBasicPixelLightingTxBn.inc"
#include "BasicEffect_VSBasicPixelLightingTxVcBn.inc"

#include "BasicEffect_PSBasic.inc"
#include "BasicEffect_PSBasicNoFog.inc"
#include "BasicEffect_PSBasicTx.inc"
#include "BasicEffect_PSBasicTxNoFog.inc"

#include "BasicEffect_PSBasicVertexLighting.inc"
#include "BasicEffect_PSBasicVertexLightingNoFog.inc"
#include "BasicEffect_PSBasicVertexLightingTx.inc"
#include "BasicEffect_PSBasicVertexLightingTxNoFog.inc"

#include "BasicEffect_PSBasicPixelLighting.inc"
#include "BasicEffect_PSBasicPixelLightingTx.inc"
#endif
}


template<>
const D3D12_SHADER_BYTECODE EffectBase<BasicEffectTraits>::VertexShaderBytecode[] =
{
    { BasicEffect_VSBasic,                     sizeof(BasicEffect_VSBasic)                     },
    { BasicEffect_VSBasicNoFog,                sizeof(BasicEffect_VSBasicNoFog)                },
    { BasicEffect_VSBasicVc,                   sizeof(BasicEffect_VSBasicVc)                   },
    { BasicEffect_VSBasicVcNoFog,              sizeof(BasicEffect_VSBasicVcNoFog)              },
    { BasicEffect_VSBasicTx,                   sizeof(BasicEffect_VSBasicTx)                   },
    { BasicEffect_VSBasicTxNoFog,              sizeof(BasicEffect_VSBasicTxNoFog)              },
    { BasicEffect_VSBasicTxVc,                 sizeof(BasicEffect_VSBasicTxVc)                 },
    { BasicEffect_VSBasicTxVcNoFog,            sizeof(BasicEffect_VSBasicTxVcNoFog)            },

    { BasicEffect_VSBasicVertexLighting,       sizeof(BasicEffect_VSBasicVertexLighting)       },
    { BasicEffect_VSBasicVertexLightingVc,     sizeof(BasicEffect_VSBasicVertexLightingVc)     },
    { BasicEffect_VSBasicVertexLightingTx,     sizeof(BasicEffect_VSBasicVertexLightingTx)     },
    { BasicEffect_VSBasicVertexLightingTxVc,   sizeof(BasicEffect_VSBasicVertexLightingTxVc)   },

    { BasicEffect_VSBasicPixelLighting,        sizeof(BasicEffect_VSBasicPixelLighting)        },
    { BasicEffect_VSBasicPixelLightingVc,      sizeof(BasicEffect_VSBasicPixelLightingVc)      },
    { BasicEffect_VSBasicPixelLightingTx,      sizeof(BasicEffect_VSBasicPixelLightingTx)      },
    { BasicEffect_VSBasicPixelLightingTxVc,    sizeof(BasicEffect_VSBasicPixelLightingTxVc)    },

    { BasicEffect_VSBasicVertexLightingBn,     sizeof(BasicEffect_VSBasicVertexLightingBn)     },
    { BasicEffect_VSBasicVertexLightingVcBn,   sizeof(BasicEffect_VSBasicVertexLightingVcBn)   },
    { BasicEffect_VSBasicVertexLightingTxBn,   sizeof(BasicEffect_VSBasicVertexLightingTxBn)   },
    { BasicEffect_VSBasicVertexLightingTxVcBn, sizeof(BasicEffect_VSBasicVertexLightingTxVcBn) },

    { BasicEffect_VSBasicPixelLightingBn,      sizeof(BasicEffect_VSBasicPixelLightingBn)      },
    { BasicEffect_VSBasicPixelLightingVcBn,    sizeof(BasicEffect_VSBasicPixelLightingVcBn)    },
    { BasicEffect_VSBasicPixelLightingTxBn,    sizeof(BasicEffect_VSBasicPixelLightingTxBn)    },
    { BasicEffect_VSBasicPixelLightingTxVcBn,  sizeof(BasicEffect_VSBasicPixelLightingTxVcBn)  },
};


template<>
const int EffectBase<BasicEffectTraits>::VertexShaderIndices[] =
{
    0,      // basic
    1,      // no fog
    2,      // vertex color
    3,      // vertex color, no fog
    4,      // texture
    5,      // texture, no fog
    6,      // texture + vertex color
    7,      // texture + vertex color, no fog

    8,      // vertex lighting
    8,      // vertex lighting, no fog
    9,      // vertex lighting + vertex color
    9,      // vertex lighting + vertex color, no fog
    10,     // vertex lighting + texture
    10,     // vertex lighting + texture, no fog
    11,     // vertex lighting + texture + vertex color
    11,     // vertex lighting + texture + vertex color, no fog

    12,     // pixel lighting
    12,     // pixel lighting, no fog
    13,     // pixel lighting + vertex color
    13,     // pixel lighting + vertex color, no fog
    14,     // pixel lighting + texture
    14,     // pixel lighting + texture, no fog
    15,     // pixel lighting + texture + vertex color
    15,     // pixel lighting + texture + vertex color, no fog

    16,     // vertex lighting (biased vertex normals)
    16,     // vertex lighting (biased vertex normals), no fog
    17,     // vertex lighting (biased vertex normals) + vertex color
    17,     // vertex lighting (biased vertex normals) + vertex color, no fog
    18,     // vertex lighting (biased vertex normals) + texture
    18,     // vertex lighting (biased vertex normals) + texture, no fog
    19,     // vertex lighting (biased vertex normals) + texture + vertex color
    19,     // vertex lighting (biased vertex normals) + texture + vertex color, no fog

    20,     // pixel lighting (biased vertex normals)
    20,     // pixel lighting (biased vertex normals), no fog
    21,     // pixel lighting (biased vertex normals) + vertex color
    21,     // pixel lighting (biased vertex normals) + vertex color, no fog
    22,     // pixel lighting (biased vertex normals) + texture
    22,     // pixel lighting (biased vertex normals) + texture, no fog
    23,     // pixel lighting (biased vertex normals) + texture + vertex color
    23,     // pixel lighting (biased vertex normals) + texture + vertex color, no fog
};


template<>
const D3D12_SHADER_BYTECODE EffectBase<BasicEffectTraits>::PixelShaderBytecode[] =
{
    { BasicEffect_PSBasic,                      sizeof(BasicEffect_PSBasic)                      },
    { BasicEffect_PSBasicNoFog,                 sizeof(BasicEffect_PSBasicNoFog)                 },
    { BasicEffect_PSBasicTx,                    sizeof(BasicEffect_PSBasicTx)                    },
    { BasicEffect_PSBasicTxNoFog,               sizeof(BasicEffect_PSBasicTxNoFog)               },

    { BasicEffect_PSBasicVertexLighting,        sizeof(BasicEffect_PSBasicVertexLighting)        },
    { BasicEffect_PSBasicVertexLightingNoFog,   sizeof(BasicEffect_PSBasicVertexLightingNoFog)   },
    { BasicEffect_PSBasicVertexLightingTx,      sizeof(BasicEffect_PSBasicVertexLightingTx)      },
    { BasicEffect_PSBasicVertexLightingTxNoFog, sizeof(BasicEffect_PSBasicVertexLightingTxNoFog) },

    { BasicEffect_PSBasicPixelLighting,         sizeof(BasicEffect_PSBasicPixelLighting)         },
    { BasicEffect_PSBasicPixelLightingTx,       sizeof(BasicEffect_PSBasicPixelLightingTx)       },
};


template<>
const int EffectBase<BasicEffectTraits>::PixelShaderIndices[] =
{
    0,      // basic
    1,      // no fog
    0,      // vertex color
    1,      // vertex color, no fog
    2,      // texture
    3,      // texture, no fog
    2,      // texture + vertex color
    3,      // texture + vertex color, no fog

    4,      // vertex lighting
    5,      // vertex lighting, no fog
    4,      // vertex lighting + vertex color
    5,      // vertex lighting + vertex color, no fog
    6,      // vertex lighting + texture
    7,      // vertex lighting + texture, no fog
    6,      // vertex lighting + texture + vertex color
    7,      // vertex lighting + texture + vertex color, no fog

    8,      // pixel lighting
    8,      // pixel lighting, no fog
    8,      // pixel lighting + vertex color
    8,      // pixel lighting + vertex color, no fog
    9,      // pixel lighting + texture
    9,      // pixel lighting + texture, no fog
    9,      // pixel lighting + texture + vertex color
    9,      // pixel lighting + texture + vertex color, no fog

    4,      // vertex lighting (biased vertex normals)
    5,      // vertex lighting (biased vertex normals), no fog
    4,      // vertex lighting (biased vertex normals) + vertex color
    5,      // vertex lighting (biased vertex normals) + vertex color, no fog
    6,      // vertex lighting (biased vertex normals) + texture
    7,      // vertex lighting (biased vertex normals) + texture, no fog
    6,      // vertex lighting (biased vertex normals) + texture + vertex color
    7,      // vertex lighting (biased vertex normals) + texture + vertex color, no fog

    8,      // pixel lighting (biased vertex normals)
    8,      // pixel lighting (biased vertex normals), no fog
    8,      // pixel lighting (biased vertex normals) + vertex color
    8,      // pixel lighting (biased vertex normals) + vertex color, no fog
    9,      // pixel lighting (biased vertex normals) + texture
    9,      // pixel lighting (biased vertex normals) + texture, no fog
    9,      // pixel lighting (biased vertex normals) + texture + vertex color
    9,      // pixel lighting (biased vertex normals) + texture + vertex color, no fog
};
#pragma endregion

// Global pool of per-device BasicEffect resources.
template<>
SharedResourcePool<ID3D12Device*, EffectBase<BasicEffectTraits>::DeviceResources> EffectBase<BasicEffectTraits>::deviceResourcesPool = {};


// Constructor.
BasicEffect::Impl::Impl(
    _In_ ID3D12Device* device,
    uint32_t effectFlags,
    const EffectPipelineStateDescription& pipelineDescription)
    : EffectBase(device),
    texture{},
    sampler{}
{
    static_assert(static_cast<int>(std::size(EffectBase<BasicEffectTraits>::VertexShaderIndices)) == BasicEffectTraits::ShaderPermutationCount, "array/max mismatch");
    static_assert(static_cast<int>(std::size(EffectBase<BasicEffectTraits>::VertexShaderBytecode)) == BasicEffectTraits::VertexShaderCount, "array/max mismatch");
    static_assert(static_cast<int>(std::size(EffectBase<BasicEffectTraits>::PixelShaderBytecode)) == BasicEffectTraits::PixelShaderCount, "array/max mismatch");
    static_assert(static_cast<int>(std::size(EffectBase<BasicEffectTraits>::PixelShaderIndices)) == BasicEffectTraits::ShaderPermutationCount, "array/max mismatch");

    if (effectFlags & EffectFlags::Instancing)
    {
        DebugTrace("ERROR: BasicEffect does not implement EffectFlags::Instancing\n");
        throw std::invalid_argument("Instancing effect flag is invalid");
    }

    lights.InitializeConstants(constants.specularColorAndPower, constants.lightDirection, constants.lightDiffuseColor, constants.lightSpecularColor);

    fog.enabled = (effectFlags & EffectFlags::Fog) != 0;
    lightingEnabled = (effectFlags & EffectFlags::Lighting) != 0;
    textureEnabled = (effectFlags & EffectFlags::Texture) != 0;

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

        // Create root parameters and initialize first (constants)
        CD3DX12_ROOT_PARAMETER rootParameters[RootParameterIndex::RootParameterCount] = {};
        rootParameters[RootParameterIndex::ConstantBuffer].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);

        // Root parameter descriptor - conditionally initialized
        CD3DX12_ROOT_SIGNATURE_DESC rsigDesc = {};

        if (textureEnabled)
        {
            // Include texture and srv
            const CD3DX12_DESCRIPTOR_RANGE textureSRV(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
            const CD3DX12_DESCRIPTOR_RANGE textureSampler(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);

            rootParameters[RootParameterIndex::TextureSRV].InitAsDescriptorTable(1, &textureSRV, D3D12_SHADER_VISIBILITY_PIXEL);
            rootParameters[RootParameterIndex::TextureSampler].InitAsDescriptorTable(1, &textureSampler, D3D12_SHADER_VISIBILITY_PIXEL);

            // use all parameters
            rsigDesc.Init(static_cast<UINT>(std::size(rootParameters)), rootParameters, 0, nullptr, rootSignatureFlags);

            mRootSignature = GetRootSignature(1, rsigDesc);
        }
        else
        {
            // only use constant
            rsigDesc.Init(1, rootParameters, 0, nullptr, rootSignatureFlags);

            mRootSignature = GetRootSignature(0, rsigDesc);
        }
    }

    assert(mRootSignature != nullptr);

    // Create pipeline state.
    const int sp = GetPipelineStatePermutation(effectFlags);
    assert(sp >= 0 && sp < BasicEffectTraits::ShaderPermutationCount);
    _Analysis_assume_(sp >= 0 && sp < BasicEffectTraits::ShaderPermutationCount);

    const int vi = EffectBase<BasicEffectTraits>::VertexShaderIndices[sp];
    assert(vi >= 0 && vi < BasicEffectTraits::VertexShaderCount);
    _Analysis_assume_(vi >= 0 && vi < BasicEffectTraits::VertexShaderCount);
    const int pi = EffectBase<BasicEffectTraits>::PixelShaderIndices[sp];
    assert(pi >= 0 && pi < BasicEffectTraits::PixelShaderCount);
    _Analysis_assume_(pi >= 0 && pi < BasicEffectTraits::PixelShaderCount);

    pipelineDescription.CreatePipelineState(
        device,
        mRootSignature,
        EffectBase<BasicEffectTraits>::VertexShaderBytecode[vi],
        EffectBase<BasicEffectTraits>::PixelShaderBytecode[pi],
        mPipelineState.GetAddressOf());

    SetDebugObjectName(mPipelineState.Get(), L"BasicEffect");
}


int BasicEffect::Impl::GetPipelineStatePermutation(uint32_t effectFlags) const noexcept
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

    // Support texturing?
    if (textureEnabled)
    {
        permutation += 4;
    }

    if (lightingEnabled)
    {
        if (effectFlags & EffectFlags::PerPixelLightingBit)
        {
            // Do lighting in the pixel shader.
            permutation += 16;
        }
        else
        {
            permutation += 8;
        }

        if (effectFlags & EffectFlags::BiasedVertexNormals)
        {
            // Compressed normals need to be scaled and biased in the vertex shader.
            permutation += 16;
        }
    }

    return permutation;
}


// Sets our state onto the D3D device.
void BasicEffect::Impl::Apply(_In_ ID3D12GraphicsCommandList* commandList)
{
    // Compute derived parameter values.
    matrices.SetConstants(dirtyFlags, constants.worldViewProj);
    fog.SetConstants(dirtyFlags, matrices.worldView, constants.fogVector);
    lights.SetConstants(dirtyFlags, matrices, constants.world, constants.worldInverseTranspose, constants.eyePosition, constants.diffuseColor, constants.emissiveColor, lightingEnabled);

    UpdateConstants();

    // Set the root signature
    commandList->SetGraphicsRootSignature(mRootSignature);

    // Set the texture
    if (textureEnabled)
    {
        if (!texture.ptr || !sampler.ptr)
        {
            DebugTrace("ERROR: Missing texture or sampler for BasicEffect (texture %llu, sampler %llu)\n", texture.ptr, sampler.ptr);
            throw std::runtime_error("BasicEffect");
        }

        // **NOTE** If D3D asserts or crashes here, you probably need to call commandList->SetDescriptorHeaps() with the required descriptor heaps.
        commandList->SetGraphicsRootDescriptorTable(RootParameterIndex::TextureSRV, texture);
        commandList->SetGraphicsRootDescriptorTable(RootParameterIndex::TextureSampler, sampler);
    }

    // Set constants
    commandList->SetGraphicsRootConstantBufferView(RootParameterIndex::ConstantBuffer, GetConstantBufferGpuAddress());

    // Set the pipeline state
    commandList->SetPipelineState(EffectBase::mPipelineState.Get());
}


// Public constructor.
BasicEffect::BasicEffect(
    _In_ ID3D12Device* device,
    uint32_t effectFlags,
    const EffectPipelineStateDescription& pipelineDescription)
    : pImpl(std::make_unique<Impl>(device, effectFlags, pipelineDescription))
{
}


// Move constructor.
BasicEffect::BasicEffect(BasicEffect&&) noexcept = default;
BasicEffect& BasicEffect::operator= (BasicEffect&&) noexcept = default;
BasicEffect::~BasicEffect() = default;


// IEffect methods
void BasicEffect::Apply(_In_ ID3D12GraphicsCommandList* commandList)
{
    pImpl->Apply(commandList);
}


// Camera settings
void XM_CALLCONV BasicEffect::SetWorld(FXMMATRIX value)
{
    pImpl->matrices.world = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj | EffectDirtyFlags::WorldInverseTranspose | EffectDirtyFlags::FogVector;
}


void XM_CALLCONV BasicEffect::SetView(FXMMATRIX value)
{
    pImpl->matrices.view = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj | EffectDirtyFlags::EyePosition | EffectDirtyFlags::FogVector;
}


void XM_CALLCONV BasicEffect::SetProjection(FXMMATRIX value)
{
    pImpl->matrices.projection = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj;
}


void XM_CALLCONV BasicEffect::SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection)
{
    pImpl->matrices.world = world;
    pImpl->matrices.view = view;
    pImpl->matrices.projection = projection;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj | EffectDirtyFlags::WorldInverseTranspose | EffectDirtyFlags::EyePosition | EffectDirtyFlags::FogVector;
}


// Material settings
void XM_CALLCONV BasicEffect::SetDiffuseColor(FXMVECTOR value)
{
    pImpl->lights.diffuseColor = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::MaterialColor;
}


void XM_CALLCONV BasicEffect::SetEmissiveColor(FXMVECTOR value)
{
    pImpl->lights.emissiveColor = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::MaterialColor;
}


void XM_CALLCONV BasicEffect::SetSpecularColor(FXMVECTOR value)
{
    // Set xyz to new value, but preserve existing w (specular power).
    pImpl->constants.specularColorAndPower = XMVectorSelect(pImpl->constants.specularColorAndPower, value, g_XMSelect1110);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void BasicEffect::SetSpecularPower(float value)
{
    // Set w to new value, but preserve existing xyz (specular color).
    pImpl->constants.specularColorAndPower = XMVectorSetW(pImpl->constants.specularColorAndPower, value);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void BasicEffect::DisableSpecular()
{
    // Set specular color to black, power to 1
    // Note: Don't use a power of 0 or the shader will generate strange highlights on non-specular materials

    pImpl->constants.specularColorAndPower = g_XMIdentityR3;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void BasicEffect::SetAlpha(float value)
{
    pImpl->lights.alpha = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::MaterialColor;
}


void XM_CALLCONV BasicEffect::SetColorAndAlpha(FXMVECTOR value)
{
    pImpl->lights.diffuseColor = value;
    pImpl->lights.alpha = XMVectorGetW(value);

    pImpl->dirtyFlags |= EffectDirtyFlags::MaterialColor;
}


// Light settings
void XM_CALLCONV BasicEffect::SetAmbientLightColor(FXMVECTOR value)
{
    pImpl->lights.ambientLightColor = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::MaterialColor;
}


void BasicEffect::SetLightEnabled(int whichLight, bool value)
{
    pImpl->dirtyFlags |= pImpl->lights.SetLightEnabled(whichLight, value, pImpl->constants.lightDiffuseColor, pImpl->constants.lightSpecularColor);
}


void XM_CALLCONV BasicEffect::SetLightDirection(int whichLight, FXMVECTOR value)
{
    EffectLights::ValidateLightIndex(whichLight);

    pImpl->constants.lightDirection[whichLight] = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void XM_CALLCONV BasicEffect::SetLightDiffuseColor(int whichLight, FXMVECTOR value)
{
    pImpl->dirtyFlags |= pImpl->lights.SetLightDiffuseColor(whichLight, value, pImpl->constants.lightDiffuseColor);
}


void XM_CALLCONV BasicEffect::SetLightSpecularColor(int whichLight, FXMVECTOR value)
{
    pImpl->dirtyFlags |= pImpl->lights.SetLightSpecularColor(whichLight, value, pImpl->constants.lightSpecularColor);
}


void BasicEffect::EnableDefaultLighting()
{
    EffectLights::EnableDefaultLighting(this);
}


// Fog settings.
void BasicEffect::SetFogStart(float value)
{
    pImpl->fog.start = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::FogVector;
}


void BasicEffect::SetFogEnd(float value)
{
    pImpl->fog.end = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::FogVector;
}


void XM_CALLCONV BasicEffect::SetFogColor(FXMVECTOR value)
{
    pImpl->constants.fogColor = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


// Texture settings.
void BasicEffect::SetTexture(D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor, D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptor)
{
    pImpl->texture = srvDescriptor;
    pImpl->sampler = samplerDescriptor;
}
