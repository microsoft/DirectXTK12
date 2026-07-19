//--------------------------------------------------------------------------------------
// File: NPREffect.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// https://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "EffectCommon.h"

using namespace DirectX;

namespace
{
    // Constant buffer layout. Must match the shader!
    struct NPREffectConstants
    {
        XMVECTOR lightDirectionAndCelBands;
        XMVECTOR diffuseColorAndAlpha;
        XMVECTOR specularColorAndSpecularThreshold;
        XMVECTOR rimColorAndPower;
        XMVECTOR extraSettings; // .x = SpecularSmoothing, .y = RimStrength, .z = RimStart, .w = RimEnd
        XMVECTOR goochCoolColorAndAlpha;
        XMVECTOR goochWarmColorAndBeta;
        XMVECTOR eyePosition;
        XMMATRIX world;
        XMVECTOR worldInverseTranspose[3];
        XMMATRIX worldViewProj;
    };

    static_assert((sizeof(NPREffectConstants) % 16) == 0, "CB size not padded correctly");


    // Traits type describes our characteristics to the EffectBase template.
    struct NPREffectTraits
    {
        using ConstantBufferType = NPREffectConstants;

        static constexpr int VertexShaderCount = 32;
        static constexpr int PixelShaderCount = 6;
        static constexpr int ShaderPermutationCount = 48;
        static constexpr int RootSignatureCount = 3;
    };


    // Default values
    constexpr XMVECTORF32 s_defaultLightDir = { { { 0.f, -1.f, 0.f, 4.f } } };
    constexpr XMVECTORF32 s_defaultDiffuse = { { { 1.f, 1.f, 1.f, 1.f } } };
    constexpr XMVECTORF32 s_defaultSpecular = { { { 1.f, 1.f, 1.f, 0.95f } } };
    constexpr XMVECTORF32 s_defaultRim = { { { 0.f, 0.f, 0.f, 4.f } } };
    constexpr XMVECTORF32 s_defaultExtraSettings = { { { 0.004f, 0.75f, 0.2f, 0.6f } } };
    constexpr XMVECTORF32 s_defaultCool = { { { 0.f, 0.f, 0.55f, 0.25f } } };
    constexpr XMVECTORF32 s_defaultWarm = { { { 0.3f, 0.3f, 0.f, 0.25f } } };
}


// Internal NPREffect implementation class.
class NPREffect::Impl : public EffectBase<NPREffectTraits>
{
public:
    Impl(_In_ ID3D12Device* device, uint32_t effectFlags, const EffectPipelineStateDescription& pipelineDescription,
        NPREffect::Mode nprMode);

    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;

    Impl(Impl&&) = default;
    Impl& operator=(Impl&&) = default;

    enum RootParameterIndex
    {
        ConstantBuffer,
        TextureSRV,
        TextureSampler,
        Texture2SRV,
        RootParameterCount
    };

    bool matCapEnabled;
    bool textureEnabled;

    D3D12_GPU_DESCRIPTOR_HANDLE texture;
    D3D12_GPU_DESCRIPTOR_HANDLE sampler;
    D3D12_GPU_DESCRIPTOR_HANDLE matcap;

    int GetPipelineStatePermutation(NPREffect::Mode nprMode, uint32_t effectFlags) const noexcept;

    void Apply(_In_ ID3D12GraphicsCommandList* commandList);
};


#pragma region Shaders
// Include the precompiled shader code.
namespace
{
#ifdef _GAMING_XBOX_SCARLETT
#include "XboxGamingScarlettNPREffect_VSNPREffect.inc"
#include "XboxGamingScarlettNPREffect_VSNPREffectInst.inc"

#include "XboxGamingScarlettNPREffect_VSNPREffectVc.inc"
#include "XboxGamingScarlettNPREffect_VSNPREffectVcInst.inc"

#include "XboxGamingScarlettNPREffect_VSNPREffectBn.inc"
#include "XboxGamingScarlettNPREffect_VSNPREffectBnInst.inc"

#include "XboxGamingScarlettNPREffect_VSNPREffectVcBn.inc"
#include "XboxGamingScarlettNPREffect_VSNPREffectVcBnInst.inc"

#include "XboxGamingScarlettNPREffect_VSNPREffectTx.inc"
#include "XboxGamingScarlettNPREffect_VSNPREffectInstTx.inc"

#include "XboxGamingScarlettNPREffect_VSNPREffectVcTx.inc"
#include "XboxGamingScarlettNPREffect_VSNPREffectVcInstTx.inc"

#include "XboxGamingScarlettNPREffect_VSNPREffectBnTx.inc"
#include "XboxGamingScarlettNPREffect_VSNPREffectBnInstTx.inc"

#include "XboxGamingScarlettNPREffect_VSNPREffectVcBnTx.inc"
#include "XboxGamingScarlettNPREffect_VSNPREffectVcBnInstTx.inc"

#include "XboxGamingScarlettNPREffect_PSCelShading.inc"
#include "XboxGamingScarlettNPREffect_PSCelShadingTx.inc"

#include "XboxGamingScarlettNPREffect_PSGoochShading.inc"
#include "XboxGamingScarlettNPREffect_PSGoochShadingTx.inc"

#include "XboxGamingScarlettNPREffect_VSNPREffectMC.inc"
#include "XboxGamingScarlettNPREffect_VSNPREffectInstMC.inc"

#include "XboxGamingScarlettNPREffect_VSNPREffectVcMC.inc"
#include "XboxGamingScarlettNPREffect_VSNPREffectVcInstMC.inc"

#include "XboxGamingScarlettNPREffect_VSNPREffectBnMC.inc"
#include "XboxGamingScarlettNPREffect_VSNPREffectBnInstMC.inc"

#include "XboxGamingScarlettNPREffect_VSNPREffectVcBnMC.inc"
#include "XboxGamingScarlettNPREffect_VSNPREffectVcBnInstMC.inc"

#include "XboxGamingScarlettNPREffect_VSNPREffectTxMC.inc"
#include "XboxGamingScarlettNPREffect_VSNPREffectInstTxMC.inc"

#include "XboxGamingScarlettNPREffect_VSNPREffectVcTxMC.inc"
#include "XboxGamingScarlettNPREffect_VSNPREffectVcInstTxMC.inc"

#include "XboxGamingScarlettNPREffect_VSNPREffectBnTxMC.inc"
#include "XboxGamingScarlettNPREffect_VSNPREffectBnInstTxMC.inc"

#include "XboxGamingScarlettNPREffect_VSNPREffectVcBnTxMC.inc"
#include "XboxGamingScarlettNPREffect_VSNPREffectVcBnInstTxMC.inc"

#include "XboxGamingScarlettNPREffect_PSMatCapShading.inc"
#include "XboxGamingScarlettNPREffect_PSMatCapShadingTx.inc"
#elif defined(_GAMING_XBOX)
#include "XboxGamingXboxOneNPREffect_VSNPREffect.inc"
#include "XboxGamingXboxOneNPREffect_VSNPREffectInst.inc"

#include "XboxGamingXboxOneNPREffect_VSNPREffectVc.inc"
#include "XboxGamingXboxOneNPREffect_VSNPREffectVcInst.inc"

#include "XboxGamingXboxOneNPREffect_VSNPREffectBn.inc"
#include "XboxGamingXboxOneNPREffect_VSNPREffectBnInst.inc"

#include "XboxGamingXboxOneNPREffect_VSNPREffectVcBn.inc"
#include "XboxGamingXboxOneNPREffect_VSNPREffectVcBnInst.inc"

#include "XboxGamingXboxOneNPREffect_VSNPREffectTx.inc"
#include "XboxGamingXboxOneNPREffect_VSNPREffectInstTx.inc"

#include "XboxGamingXboxOneNPREffect_VSNPREffectVcTx.inc"
#include "XboxGamingXboxOneNPREffect_VSNPREffectVcInstTx.inc"

#include "XboxGamingXboxOneNPREffect_VSNPREffectBnTx.inc"
#include "XboxGamingXboxOneNPREffect_VSNPREffectBnInstTx.inc"

#include "XboxGamingXboxOneNPREffect_VSNPREffectVcBnTx.inc"
#include "XboxGamingXboxOneNPREffect_VSNPREffectVcBnInstTx.inc"

#include "XboxGamingXboxOneNPREffect_PSCelShading.inc"
#include "XboxGamingXboxOneNPREffect_PSCelShadingTx.inc"

#include "XboxGamingXboxOneNPREffect_PSGoochShading.inc"
#include "XboxGamingXboxOneNPREffect_PSGoochShadingTx.inc"

#include "XboxGamingXboxOneNPREffect_VSNPREffectMC.inc"
#include "XboxGamingXboxOneNPREffect_VSNPREffectInstMC.inc"

#include "XboxGamingXboxOneNPREffect_VSNPREffectVcMC.inc"
#include "XboxGamingXboxOneNPREffect_VSNPREffectVcInstMC.inc"

#include "XboxGamingXboxOneNPREffect_VSNPREffectBnMC.inc"
#include "XboxGamingXboxOneNPREffect_VSNPREffectBnInstMC.inc"

#include "XboxGamingXboxOneNPREffect_VSNPREffectVcBnMC.inc"
#include "XboxGamingXboxOneNPREffect_VSNPREffectVcBnInstMC.inc"

#include "XboxGamingXboxOneNPREffect_VSNPREffectTxMC.inc"
#include "XboxGamingXboxOneNPREffect_VSNPREffectInstTxMC.inc"

#include "XboxGamingXboxOneNPREffect_VSNPREffectVcTxMC.inc"
#include "XboxGamingXboxOneNPREffect_VSNPREffectVcInstTxMC.inc"

#include "XboxGamingXboxOneNPREffect_VSNPREffectBnTxMC.inc"
#include "XboxGamingXboxOneNPREffect_VSNPREffectBnInstTxMC.inc"

#include "XboxGamingXboxOneNPREffect_VSNPREffectVcBnTxMC.inc"
#include "XboxGamingXboxOneNPREffect_VSNPREffectVcBnInstTxMC.inc"

#include "XboxGamingXboxOneNPREffect_PSMatCapShading.inc"
#include "XboxGamingXboxOneNPREffect_PSMatCapShadingTx.inc"
#elif defined(_XBOX_ONE) && defined(_TITLE)
#include "XboxOneNPREffect_VSNPREffect.inc"
#include "XboxOneNPREffect_VSNPREffectInst.inc"

#include "XboxOneNPREffect_VSNPREffectVc.inc"
#include "XboxOneNPREffect_VSNPREffectVcInst.inc"

#include "XboxOneNPREffect_VSNPREffectBn.inc"
#include "XboxOneNPREffect_VSNPREffectBnInst.inc"

#include "XboxOneNPREffect_VSNPREffectVcBn.inc"
#include "XboxOneNPREffect_VSNPREffectVcBnInst.inc"

#include "XboxOneNPREffect_VSNPREffectTx.inc"
#include "XboxOneNPREffect_VSNPREffectInstTx.inc"

#include "XboxOneNPREffect_VSNPREffectVcTx.inc"
#include "XboxOneNPREffect_VSNPREffectVcInstTx.inc"

#include "XboxOneNPREffect_VSNPREffectBnTx.inc"
#include "XboxOneNPREffect_VSNPREffectBnInstTx.inc"

#include "XboxOneNPREffect_VSNPREffectVcBnTx.inc"
#include "XboxOneNPREffect_VSNPREffectVcBnInstTx.inc"

#include "XboxOneNPREffect_PSCelShading.inc"
#include "XboxOneNPREffect_PSCelShadingTx.inc"

#include "XboxOneNPREffect_PSGoochShading.inc"
#include "XboxOneNPREffect_PSGoochShadingTx.inc"

#include "XboxOneNPREffect_VSNPREffectMC.inc"
#include "XboxOneNPREffect_VSNPREffectInstMC.inc"

#include "XboxOneNPREffect_VSNPREffectVcMC.inc"
#include "XboxOneNPREffect_VSNPREffectVcInstMC.inc"

#include "XboxOneNPREffect_VSNPREffectBnMC.inc"
#include "XboxOneNPREffect_VSNPREffectBnInstMC.inc"

#include "XboxOneNPREffect_VSNPREffectVcBnMC.inc"
#include "XboxOneNPREffect_VSNPREffectVcBnInstMC.inc"

#include "XboxOneNPREffect_VSNPREffectTxMC.inc"
#include "XboxOneNPREffect_VSNPREffectInstTxMC.inc"

#include "XboxOneNPREffect_VSNPREffectVcTxMC.inc"
#include "XboxOneNPREffect_VSNPREffectVcInstTxMC.inc"

#include "XboxOneNPREffect_VSNPREffectBnTxMC.inc"
#include "XboxOneNPREffect_VSNPREffectBnInstTxMC.inc"

#include "XboxOneNPREffect_VSNPREffectVcBnTxMC.inc"
#include "XboxOneNPREffect_VSNPREffectVcBnInstTxMC.inc"

#include "XboxOneNPREffect_PSMatCapShading.inc"
#include "XboxOneNPREffect_PSMatCapShadingTx.inc"
#else
#include "NPREffect_VSNPREffect.inc"
#include "NPREffect_VSNPREffectInst.inc"

#include "NPREffect_VSNPREffectVc.inc"
#include "NPREffect_VSNPREffectVcInst.inc"

#include "NPREffect_VSNPREffectBn.inc"
#include "NPREffect_VSNPREffectBnInst.inc"

#include "NPREffect_VSNPREffectVcBn.inc"
#include "NPREffect_VSNPREffectVcBnInst.inc"

#include "NPREffect_VSNPREffectTx.inc"
#include "NPREffect_VSNPREffectInstTx.inc"

#include "NPREffect_VSNPREffectVcTx.inc"
#include "NPREffect_VSNPREffectVcInstTx.inc"

#include "NPREffect_VSNPREffectBnTx.inc"
#include "NPREffect_VSNPREffectBnInstTx.inc"

#include "NPREffect_VSNPREffectVcBnTx.inc"
#include "NPREffect_VSNPREffectVcBnInstTx.inc"

#include "NPREffect_PSCelShading.inc"
#include "NPREffect_PSCelShadingTx.inc"

#include "NPREffect_PSGoochShading.inc"
#include "NPREffect_PSGoochShadingTx.inc"

#include "NPREffect_VSNPREffectMC.inc"
#include "NPREffect_VSNPREffectInstMC.inc"

#include "NPREffect_VSNPREffectVcMC.inc"
#include "NPREffect_VSNPREffectVcInstMC.inc"

#include "NPREffect_VSNPREffectBnMC.inc"
#include "NPREffect_VSNPREffectBnInstMC.inc"

#include "NPREffect_VSNPREffectVcBnMC.inc"
#include "NPREffect_VSNPREffectVcBnInstMC.inc"

#include "NPREffect_VSNPREffectTxMC.inc"
#include "NPREffect_VSNPREffectInstTxMC.inc"

#include "NPREffect_VSNPREffectVcTxMC.inc"
#include "NPREffect_VSNPREffectVcInstTxMC.inc"

#include "NPREffect_VSNPREffectBnTxMC.inc"
#include "NPREffect_VSNPREffectBnInstTxMC.inc"

#include "NPREffect_VSNPREffectVcBnTxMC.inc"
#include "NPREffect_VSNPREffectVcBnInstTxMC.inc"

#include "NPREffect_PSMatCapShading.inc"
#include "NPREffect_PSMatCapShadingTx.inc"
#endif
}


template<>
const D3D12_SHADER_BYTECODE EffectBase<NPREffectTraits>::VertexShaderBytecode[] =
{
    { NPREffect_VSNPREffect,             sizeof(NPREffect_VSNPREffect)             },
    { NPREffect_VSNPREffectVc,           sizeof(NPREffect_VSNPREffectVc)           },
    { NPREffect_VSNPREffectBn,           sizeof(NPREffect_VSNPREffectBn)           },
    { NPREffect_VSNPREffectVcBn,         sizeof(NPREffect_VSNPREffectVcBn)         },
    { NPREffect_VSNPREffectInst,         sizeof(NPREffect_VSNPREffectInst)         },
    { NPREffect_VSNPREffectVcInst,       sizeof(NPREffect_VSNPREffectVcInst)       },
    { NPREffect_VSNPREffectBnInst,       sizeof(NPREffect_VSNPREffectBnInst)       },
    { NPREffect_VSNPREffectVcBnInst,     sizeof(NPREffect_VSNPREffectVcBnInst)     },
    { NPREffect_VSNPREffectTx,           sizeof(NPREffect_VSNPREffectTx)           },
    { NPREffect_VSNPREffectVcTx,         sizeof(NPREffect_VSNPREffectVcTx)         },
    { NPREffect_VSNPREffectBnTx,         sizeof(NPREffect_VSNPREffectBnTx)         },
    { NPREffect_VSNPREffectVcBnTx,       sizeof(NPREffect_VSNPREffectVcBnTx)       },
    { NPREffect_VSNPREffectInstTx,       sizeof(NPREffect_VSNPREffectInstTx)       },
    { NPREffect_VSNPREffectVcInstTx,     sizeof(NPREffect_VSNPREffectVcInstTx)     },
    { NPREffect_VSNPREffectBnInstTx,     sizeof(NPREffect_VSNPREffectBnInstTx)     },
    { NPREffect_VSNPREffectVcBnInstTx,   sizeof(NPREffect_VSNPREffectVcBnInstTx)   },
    { NPREffect_VSNPREffectMC,           sizeof(NPREffect_VSNPREffectMC)           }, // Different rootsig
    { NPREffect_VSNPREffectVcMC,         sizeof(NPREffect_VSNPREffectVcMC)         },
    { NPREffect_VSNPREffectBnMC,         sizeof(NPREffect_VSNPREffectBnMC)         },
    { NPREffect_VSNPREffectVcBnMC,       sizeof(NPREffect_VSNPREffectVcBnMC)       },
    { NPREffect_VSNPREffectInstMC,       sizeof(NPREffect_VSNPREffectInstMC)       },
    { NPREffect_VSNPREffectVcInstMC,     sizeof(NPREffect_VSNPREffectVcInstMC)     },
    { NPREffect_VSNPREffectBnInstMC,     sizeof(NPREffect_VSNPREffectBnInstMC)     },
    { NPREffect_VSNPREffectVcBnInstMC,   sizeof(NPREffect_VSNPREffectVcBnInstMC)   },
    { NPREffect_VSNPREffectTxMC,         sizeof(NPREffect_VSNPREffectTxMC)         },
    { NPREffect_VSNPREffectVcTxMC,       sizeof(NPREffect_VSNPREffectVcTxMC)       },
    { NPREffect_VSNPREffectBnTxMC,       sizeof(NPREffect_VSNPREffectBnTxMC)       },
    { NPREffect_VSNPREffectVcBnTxMC,     sizeof(NPREffect_VSNPREffectVcBnTxMC)     },
    { NPREffect_VSNPREffectInstTxMC,     sizeof(NPREffect_VSNPREffectInstTxMC)     },
    { NPREffect_VSNPREffectVcInstTxMC,   sizeof(NPREffect_VSNPREffectVcInstTxMC)   },
    { NPREffect_VSNPREffectBnInstTxMC,   sizeof(NPREffect_VSNPREffectBnInstTxMC)   },
    { NPREffect_VSNPREffectVcBnInstTxMC, sizeof(NPREffect_VSNPREffectVcBnInstTxMC) },
};


template<>
const int EffectBase<NPREffectTraits>::VertexShaderIndices[] =
{
    0,      // cel shading
    0,      // gooch shading
    16,     // matcap shading

    1,      // vertex color + cel shading
    1,      // vertex color + gooch shading
    17,     // vertex color + matcap shading

    2,      // cel shading (biased vertex normal)
    2,      // gooch shading (biased vertex normal)
    18,     // matcap shading (biased vertex normal)

    3,      // vertex color (biased vertex normal) + cel shading
    3,      // vertex color (biased vertex normal) + gooch shading
    19,     // vertex color (biased vertex normal) + matcap shading

    4,      // instancing + cel shading
    4,      // instancing + gooch shading
    20,     // instancing + matcap shading

    5,      // instancing + vertex color + cel shading
    5,      // instancing + vertex color + gooch shading
    21,     // instancing + vertex color + matcap shading

    6,      // instancing (biased vertex normal) + cel shading
    6,      // instancing (biased vertex normal) + gooch shading
    22,     // instancing (biased vertex normal) + matcap shading

    7,      // instancing + vertex color (biased vertex normal) + cel shading
    7,      // instancing + vertex color (biased vertex normal) + gooch shading
    23,     // instancing + vertex color (biased vertex normal) + matcap shading

    8,      // cel shading + texture
    8,      // gooch shading + texture
    24,     // matcap shading + texture

    9,      // vertex color + cel shading + texture
    9,      // vertex color + gooch shading + texture
    25,     // vertex color + matcap shading + texture

    10,     // cel shading (biased vertex normal) + texture
    10,     // gooch shading (biased vertex normal) + texture
    26,     // matcap shading (biased vertex normal) + texture

    11,     // vertex color (biased vertex normal) + cel shading + texture
    11,     // vertex color (biased vertex normal) + gooch shading + texture
    27,     // vertex color (biased vertex normal) + matcap shading + texture

    12,     // instancing + cel shading + texture
    12,     // instancing + gooch shading + texture
    28,     // instancing + matcap shading + texture

    13,     // instancing + vertex color + cel shading + texture
    13,     // instancing + vertex color + gooch shading + texture
    29,     // instancing + vertex color + matcap shading + texture

    14,     // instancing (biased vertex normal) + cel shading + texture
    14,     // instancing (biased vertex normal) + gooch shading + texture
    30,     // instancing (biased vertex normal) + matcap shading + texture

    15,     // instancing + vertex color (biased vertex normal) + cel shading + texture
    15,     // instancing + vertex color (biased vertex normal) + gooch shading + texture
    31,     // instancing + vertex color (biased vertex normal) + matcap shading + texture
};


template<>
const D3D12_SHADER_BYTECODE EffectBase<NPREffectTraits>::PixelShaderBytecode[] =
{
    { NPREffect_PSCelShading,      sizeof(NPREffect_PSCelShading)      },
    { NPREffect_PSGoochShading,    sizeof(NPREffect_PSGoochShading)    },
    { NPREffect_PSMatCapShading,   sizeof(NPREffect_PSMatCapShading)   },
    { NPREffect_PSCelShadingTx,    sizeof(NPREffect_PSCelShadingTx)    },
    { NPREffect_PSGoochShadingTx,  sizeof(NPREffect_PSGoochShadingTx)  },
    { NPREffect_PSMatCapShadingTx, sizeof(NPREffect_PSMatCapShadingTx) },
};


template<>
const int EffectBase<NPREffectTraits>::PixelShaderIndices[] =
{
    0,      // cel shading
    1,      // gooch shading
    2,      // matcap shading

    0,      // vertex color + cel shading
    1,      // vertex color + gooch shading
    2,      // vertex color + matcap shading

    0,      // cel shading (biased vertex normal)
    1,      // gooch shading (biased vertex normal)
    2,      // matcap shading (biased vertex normal)

    0,      // vertex color (biased vertex normal) + cel shading
    1,      // vertex color (biased vertex normal) + gooch shading
    2,      // vertex color (biased vertex normal) + matcap shading

    0,      // instancing + cel shading
    1,      // instancing + gooch shading
    2,      // instancing + matcap shading

    0,      // instancing + vertex color + cel shading
    1,      // instancing + vertex color + gooch shading
    2,      // instancing + vertex color + matcap shading

    0,      // instancing (biased vertex normal) + cel shading
    1,      // instancing (biased vertex normal) + gooch shading
    2,      // instancing (biased vertex normal) + matcap shading

    0,      // instancing + vertex color (biased vertex normal) + cel shading
    1,      // instancing + vertex color (biased vertex normal) + gooch shading
    2,      // instancing + vertex color (biased vertex normal) + matcap shading

    3,      // cel shading + texture
    4,      // gooch shading + texture
    5,      // matcap shading + texture

    3,      // vertex color + cel shading + texture
    4,      // vertex color + gooch shading + texture
    5,      // vertex color + matcap shading + texture

    3,      // cel shading (biased vertex normal) + texture
    4,      // gooch shading (biased vertex normal) + texture
    5,      // matcap shading (biased vertex normal) + texture

    3,      // vertex color (biased vertex normal) + cel shading + texture
    4,      // vertex color (biased vertex normal) + gooch shading + texture
    5,      // vertex color (biased vertex normal) + matcap shading + texture

    3,      // instancing + cel shading + texture
    4,      // instancing + gooch shading + texture
    5,      // instancing + matcap shading + texture

    3,      // instancing + vertex color + cel shading + texture
    4,      // instancing + vertex color + gooch shading + texture
    5,      // instancing + vertex color + matcap shading + texture

    3,      // instancing (biased vertex normal) + cel shading + texture
    4,      // instancing (biased vertex normal) + gooch shading + texture
    5,      // instancing (biased vertex normal) + matcap shading + texture

    3,      // instancing + vertex color (biased vertex normal) + cel shading + texture
    4,      // instancing + vertex color (biased vertex normal) + gooch shading + texture
    5,      // instancing + vertex color (biased vertex normal) + matcap shading + texture
};
#pragma endregion

// Global pool of per-device NPREffect resources.
template<>
SharedResourcePool<ID3D12Device*, EffectBase<NPREffectTraits>::DeviceResources> EffectBase<NPREffectTraits>::deviceResourcesPool = {};


// Constructor.
NPREffect::Impl::Impl(
    _In_ ID3D12Device* device,
    uint32_t effectFlags,
    const EffectPipelineStateDescription& pipelineDescription,
    NPREffect::Mode nprMode)
    : EffectBase(device),
    texture{},
    sampler{},
    matcap{}
{
    static_assert(static_cast<int>(std::size(EffectBase<NPREffectTraits>::VertexShaderIndices)) == NPREffectTraits::ShaderPermutationCount, "array/max mismatch");
    static_assert(static_cast<int>(std::size(EffectBase<NPREffectTraits>::VertexShaderBytecode)) == NPREffectTraits::VertexShaderCount, "array/max mismatch");
    static_assert(static_cast<int>(std::size(EffectBase<NPREffectTraits>::PixelShaderBytecode)) == NPREffectTraits::PixelShaderCount, "array/max mismatch");
    static_assert(static_cast<int>(std::size(EffectBase<NPREffectTraits>::PixelShaderIndices)) == NPREffectTraits::ShaderPermutationCount, "array/max mismatch");

    constants.lightDirectionAndCelBands = s_defaultLightDir;
    constants.diffuseColorAndAlpha = s_defaultDiffuse;
    constants.specularColorAndSpecularThreshold = s_defaultSpecular;
    constants.rimColorAndPower = s_defaultRim;
    constants.extraSettings = s_defaultExtraSettings;
    constants.goochCoolColorAndAlpha = s_defaultCool;
    constants.goochWarmColorAndBeta = s_defaultWarm;
    constants.eyePosition = g_XMZero;

    switch (nprMode)
    {
    case Mode_Cel:
    case Mode_Gooch:
        matCapEnabled = false;
        break;

    case Mode_MatCap:
        matCapEnabled = true;
        break;

    default:
        throw std::invalid_argument("Invalid nprMode");
    }

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

        if (textureEnabled && matCapEnabled)
        {
            // Include two textures and one sampler
            const CD3DX12_DESCRIPTOR_RANGE textureSRV(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
            const CD3DX12_DESCRIPTOR_RANGE textureSampler(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);

            rootParameters[RootParameterIndex::TextureSRV].InitAsDescriptorTable(1, &textureSRV, D3D12_SHADER_VISIBILITY_PIXEL);
            rootParameters[RootParameterIndex::TextureSampler].InitAsDescriptorTable(1, &textureSampler, D3D12_SHADER_VISIBILITY_PIXEL);

            const CD3DX12_DESCRIPTOR_RANGE texture2Range(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
            rootParameters[RootParameterIndex::Texture2SRV].InitAsDescriptorTable(1, &texture2Range, D3D12_SHADER_VISIBILITY_PIXEL);

            // use all parameters
            rsigDesc.Init(static_cast<UINT>(std::size(rootParameters)), rootParameters, 0, nullptr, rootSignatureFlags);

            mRootSignature = GetRootSignature(2, rsigDesc);
        }
        else if (textureEnabled || matCapEnabled)
        {
            // Include texture and sampler
            const CD3DX12_DESCRIPTOR_RANGE textureSRV(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
            const CD3DX12_DESCRIPTOR_RANGE textureSampler(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);

            rootParameters[RootParameterIndex::TextureSRV].InitAsDescriptorTable(1, &textureSRV, D3D12_SHADER_VISIBILITY_PIXEL);
            rootParameters[RootParameterIndex::TextureSampler].InitAsDescriptorTable(1, &textureSampler, D3D12_SHADER_VISIBILITY_PIXEL);

            // use all but last parameter
            rsigDesc.Init(static_cast<UINT>(std::size(rootParameters) - 1), rootParameters, 0, nullptr, rootSignatureFlags);

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
    const int sp = GetPipelineStatePermutation(nprMode, effectFlags);
    assert(sp >= 0 && sp < NPREffectTraits::ShaderPermutationCount);
    _Analysis_assume_(sp >= 0 && sp < NPREffectTraits::ShaderPermutationCount);

    const int vi = EffectBase<NPREffectTraits>::VertexShaderIndices[sp];
    assert(vi >= 0 && vi < NPREffectTraits::VertexShaderCount);
    _Analysis_assume_(vi >= 0 && vi < NPREffectTraits::VertexShaderCount);
    const int pi = EffectBase<NPREffectTraits>::PixelShaderIndices[sp];
    assert(pi >= 0 && pi < NPREffectTraits::PixelShaderCount);
    _Analysis_assume_(pi >= 0 && pi < NPREffectTraits::PixelShaderCount);

    pipelineDescription.CreatePipelineState(
        device,
        mRootSignature,
        EffectBase<NPREffectTraits>::VertexShaderBytecode[vi],
        EffectBase<NPREffectTraits>::PixelShaderBytecode[pi],
        mPipelineState.GetAddressOf());

    SetDebugObjectName(mPipelineState.Get(), L"NPREffect");
}


int NPREffect::Impl::GetPipelineStatePermutation(NPREffect::Mode nprMode, uint32_t effectFlags) const noexcept
{
    int permutation = static_cast<int>(nprMode);

    // Support vertex coloring?
    if (effectFlags & EffectFlags::VertexColor)
    {
        permutation += 3;
    }

    if (effectFlags & EffectFlags::BiasedVertexNormals)
    {
        // Compressed normals need to be scaled and biased in the vertex shader.
        permutation += 6;
    }

    if (effectFlags & EffectFlags::Instancing)
    {
        // Vertex shader needs to use vertex matrix transform.
        permutation += 12;
    }

    // Use shaders without texture coordinates?
    if (textureEnabled)
    {
        permutation += 24;
    }

    return permutation;
}


// Sets our state onto the D3D device.
void NPREffect::Impl::Apply(_In_ ID3D12GraphicsCommandList* commandList)
{
    // Compute derived parameter values.
    matrices.SetConstants(
        dirtyFlags,
        constants.world,
        constants.worldInverseTranspose,
        constants.worldViewProj,
        constants.eyePosition);

    UpdateConstants();

    // Set the root signature
    commandList->SetGraphicsRootSignature(mRootSignature);

    // Set the texture
    if (textureEnabled)
    {
        if (!texture.ptr || !sampler.ptr)
        {
            DebugTrace("ERROR: Missing texture or sampler for NPREffect (texture %llu, sampler %llu)\n",
                texture.ptr, sampler.ptr);
            throw std::runtime_error("NPREffect");
        }

        // **NOTE** If D3D asserts or crashes here, you probably need to call commandList->SetDescriptorHeaps()
        // with the required descriptor heaps.
        commandList->SetGraphicsRootDescriptorTable(RootParameterIndex::TextureSRV, texture);
        commandList->SetGraphicsRootDescriptorTable(RootParameterIndex::TextureSampler, sampler);
    }

    if (matCapEnabled)
    {
        if (!matcap.ptr || !sampler.ptr)
        {
            DebugTrace("ERROR: Missing matcap texture or sampler for NPREffect (texture %llu, sampler %llu)\n",
                matcap.ptr, sampler.ptr);
            throw std::runtime_error("NPREffect");
        }

        // **NOTE** If D3D asserts or crashes here, you probably need to call commandList->SetDescriptorHeaps()
        // with the required descriptor heaps.
        commandList->SetGraphicsRootDescriptorTable(
            textureEnabled ? RootParameterIndex::Texture2SRV : RootParameterIndex::TextureSRV,
            matcap);
        commandList->SetGraphicsRootDescriptorTable(RootParameterIndex::TextureSampler, sampler);
    }

    // Set constants
    commandList->SetGraphicsRootConstantBufferView(RootParameterIndex::ConstantBuffer, GetConstantBufferGpuAddress());

    // Set the pipeline state
    commandList->SetPipelineState(EffectBase::mPipelineState.Get());
}


// Public constructor.
NPREffect::NPREffect(
    _In_ ID3D12Device* device,
    uint32_t effectFlags,
    const EffectPipelineStateDescription& pipelineDescription,
    Mode nprMode)
    : pImpl(std::make_unique<Impl>(device, effectFlags, pipelineDescription, nprMode))
{}


NPREffect::NPREffect(NPREffect&&) noexcept = default;
NPREffect& NPREffect::operator= (NPREffect&&) noexcept = default;
NPREffect::~NPREffect() = default;


// IEffect methods.
void NPREffect::Apply(_In_ ID3D12GraphicsCommandList* commandList)
{
    pImpl->Apply(commandList);
}


// Camera settings.
void NPREffect::SetWorld(FXMMATRIX value)
{
    pImpl->matrices.world = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj | EffectDirtyFlags::WorldInverseTranspose;
}


void NPREffect::SetView(FXMMATRIX value)
{
    pImpl->matrices.view = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj | EffectDirtyFlags::EyePosition;
}


void NPREffect::SetProjection(FXMMATRIX value)
{
    pImpl->matrices.projection = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj;
}


void NPREffect::SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection)
{
    pImpl->matrices.world = world;
    pImpl->matrices.view = view;
    pImpl->matrices.projection = projection;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj | EffectDirtyFlags::WorldInverseTranspose | EffectDirtyFlags::EyePosition;
}


// Light settings.
void NPREffect::SetAmbientLightColor(FXMVECTOR)
{
    // Unsupported interface.
}


void NPREffect::SetLightEnabled(int, bool)
{
    // Unsupported interface.
}


void NPREffect::SetLightDirection(int whichLight, FXMVECTOR value)
{
    if (whichLight != 0)
    {
        // Only support one light
        return;
    }

    // Set xyz to new value, but preserve existing w (cel bands).
    pImpl->constants.lightDirectionAndCelBands = XMVectorSelect(pImpl->constants.lightDirectionAndCelBands, value, g_XMSelect1110);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void NPREffect::SetLightDiffuseColor(int, FXMVECTOR)
{
    // Unsupported interface.
}


void NPREffect::SetLightSpecularColor(int, FXMVECTOR)
{
    // Unsupported interface.
}


void NPREffect::EnableDefaultLighting()
{
    // Set xyz to new value, but preserve existing w (cel bands).
    pImpl->constants.lightDirectionAndCelBands = XMVectorSelect(pImpl->constants.lightDirectionAndCelBands, s_defaultLightDir, g_XMSelect1110);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


// Material settings.
void NPREffect::SetDiffuseColor(FXMVECTOR value)
{
    // Set xyz, preserve w (alpha).
    pImpl->constants.diffuseColorAndAlpha = XMVectorSelect(pImpl->constants.diffuseColorAndAlpha, value, g_XMSelect1110);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void NPREffect::SetSpecularColor(FXMVECTOR value)
{
    // Set xyz, preserve w (specular threshold).
    pImpl->constants.specularColorAndSpecularThreshold = XMVectorSelect(pImpl->constants.specularColorAndSpecularThreshold, value, g_XMSelect1110);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void NPREffect::SetSpecularThreshold(float threshold, float smoothing)
{
    if (threshold < 0.f || threshold > 1.f)
    {
        throw std::invalid_argument("Specular threshold must be between 0 and 1");
    }

    if (smoothing < 0.f || smoothing > 1.f)
    {
        throw std::invalid_argument("Specular smoothing must be between 0 and 1");
    }

    // Set w of specularColorAndSpecularThreshold.
    pImpl->constants.specularColorAndSpecularThreshold = XMVectorSetW(pImpl->constants.specularColorAndSpecularThreshold, threshold);

    // Set x of extraSettings.
    pImpl->constants.extraSettings = XMVectorSetX(pImpl->constants.extraSettings, smoothing);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void NPREffect::DisableSpecular()
{
    // Set specular color to black, threshold to 1
    pImpl->constants.specularColorAndSpecularThreshold = g_XMIdentityR3;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void NPREffect::SetAlpha(float value)
{
    // Set w of diffuseColorAndAlpha.
    pImpl->constants.diffuseColorAndAlpha = XMVectorSetW(pImpl->constants.diffuseColorAndAlpha, value);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void NPREffect::SetColorAndAlpha(FXMVECTOR value)
{
    pImpl->constants.diffuseColorAndAlpha = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


// Texture setting.
void NPREffect::SetTexture(D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor, D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptor)
{
    pImpl->texture = srvDescriptor;
    pImpl->sampler = samplerDescriptor;
}


// Cel shading setting.
void NPREffect::SetCelShaderBands(int bands)
{
    if (bands < 1)
    {
        throw std::invalid_argument("Cel shading bands must be greater than 0");
    }

    // Set w of lightDirectionAndCelBands.
    pImpl->constants.lightDirectionAndCelBands = XMVectorSetW(pImpl->constants.lightDirectionAndCelBands, static_cast<float>(bands));

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


// Gooch shading settings.
void NPREffect::SetGoochCoolColor(FXMVECTOR value, float alpha)
{
    pImpl->constants.goochCoolColorAndAlpha = XMVectorSetW(value, alpha);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void NPREffect::SetGoochWarmColor(FXMVECTOR value, float beta)
{
    pImpl->constants.goochWarmColorAndBeta = XMVectorSetW(value, beta);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


// MatCap shading setting.
void NPREffect::SetMatCap(D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor)
{
    pImpl->matcap = srvDescriptor;
}


void NPREffect::SetMatCap(D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor, D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptor)
{
    pImpl->matcap = srvDescriptor;
    pImpl->sampler = samplerDescriptor;
}


// Rim lighting settings.
void NPREffect::SetRimLightingColor(FXMVECTOR value)
{
    // Set xyz, preserve w (rim power).
    pImpl->constants.rimColorAndPower = XMVectorSelect(pImpl->constants.rimColorAndPower, value, g_XMSelect1110);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void NPREffect::SetRimLightingPower(float power)
{
    // Set w of rimColorAndPower.
    pImpl->constants.rimColorAndPower = XMVectorSetW(pImpl->constants.rimColorAndPower, power);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void NPREffect::SetRimLightingIntensity(float strength)
{
    if (strength < 0.f || strength > 1.f)
    {
        throw std::invalid_argument("Rim lighting strength must be between 0 and 1");
    }

    // Set y of extraSettings.
    pImpl->constants.extraSettings = XMVectorSetY(pImpl->constants.extraSettings, strength);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void NPREffect::SetRimLightingRange(float start, float end)
{
    if (start < 0.f || end > 1.f || start > end)
    {
        throw std::invalid_argument("Rim lighting start/end must be between 0 and 1");
    }

    // Set zw of extraSettings.
    XMVECTORF32 range = { { { 0.f, 0.f, start, end } } };
    pImpl->constants.extraSettings = XMVectorSelect(range, pImpl->constants.extraSettings, g_XMSelect1100);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void NPREffect::DisableRimLighting()
{
    pImpl->constants.rimColorAndPower = g_XMIdentityR3;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}
