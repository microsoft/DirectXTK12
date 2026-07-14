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
        XMVECTOR specularColorAndSpecularPower;
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

        static constexpr int VertexShaderCount = 16;
        static constexpr int PixelShaderCount = 4;
        static constexpr int ShaderPermutationCount = 32;
        static constexpr int RootSignatureCount = 2;
    };


    // Default values
    constexpr XMVECTORF32 s_defaultLightDir = { { { 0.f, -1.f, 0.f, 4.f } } };
    constexpr XMVECTORF32 s_defaultDiffuse = { { { 1.f, 1.f, 1.f, 1.f } } };
    constexpr XMVECTORF32 s_defaultSpecular = { { { 1.f, 1.f, 1.f, 32.f } } };
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
        RootParameterCount
    };

    bool textureEnabled;

    D3D12_GPU_DESCRIPTOR_HANDLE texture;
    D3D12_GPU_DESCRIPTOR_HANDLE sampler;

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
#endif
}


template<>
const D3D12_SHADER_BYTECODE EffectBase<NPREffectTraits>::VertexShaderBytecode[] =
{
    { NPREffect_VSNPREffect,           sizeof(NPREffect_VSNPREffect)           },
    { NPREffect_VSNPREffectVc,         sizeof(NPREffect_VSNPREffectVc)         },
    { NPREffect_VSNPREffectBn,         sizeof(NPREffect_VSNPREffectBn)         },
    { NPREffect_VSNPREffectVcBn,       sizeof(NPREffect_VSNPREffectVcBn)       },
    { NPREffect_VSNPREffectInst,       sizeof(NPREffect_VSNPREffectInst)       },
    { NPREffect_VSNPREffectVcInst,     sizeof(NPREffect_VSNPREffectVcInst)     },
    { NPREffect_VSNPREffectBnInst,     sizeof(NPREffect_VSNPREffectBnInst)     },
    { NPREffect_VSNPREffectVcBnInst,   sizeof(NPREffect_VSNPREffectVcBnInst)   },
    { NPREffect_VSNPREffectTx,         sizeof(NPREffect_VSNPREffectTx)         },
    { NPREffect_VSNPREffectVcTx,       sizeof(NPREffect_VSNPREffectVcTx)       },
    { NPREffect_VSNPREffectBnTx,       sizeof(NPREffect_VSNPREffectBnTx)       },
    { NPREffect_VSNPREffectVcBnTx,     sizeof(NPREffect_VSNPREffectVcBnTx)     },
    { NPREffect_VSNPREffectInstTx,     sizeof(NPREffect_VSNPREffectInstTx)     },
    { NPREffect_VSNPREffectVcInstTx,   sizeof(NPREffect_VSNPREffectVcInstTx)   },
    { NPREffect_VSNPREffectBnInstTx,   sizeof(NPREffect_VSNPREffectBnInstTx)   },
    { NPREffect_VSNPREffectVcBnInstTx, sizeof(NPREffect_VSNPREffectVcBnInstTx) },
};


template<>
const int EffectBase<NPREffectTraits>::VertexShaderIndices[] =
{
    0,      // cel shading
    0,      // gooch shading

    1,      // vertex color + cel shading
    1,      // vertex color + gooch shading

    2,      // cel shading (biased vertex normal)
    2,      // gooch shading (biased vertex normal)

    3,      // vertex color (biased vertex normal) + cel shading
    3,      // vertex color (biased vertex normal) + gooch shading

    4,      // instancing + cel shading
    4,      // instancing + gooch shading

    5,      // instancing + vertex color + cel shading
    5,      // instancing + vertex color + gooch shading

    6,      // instancing (biased vertex normal) + cel shading
    6,      // instancing (biased vertex normal) + gooch shading

    7,      // instancing + vertex color (biased vertex normal) + cel shading
    7,      // instancing + vertex color (biased vertex normal) + gooch shading

    8,      // cel shading + texture
    8,      // gooch shading + texture

    9,      // vertex color + cel shading + texture
    9,      // vertex color + gooch shading + texture

    10,     // cel shading (biased vertex normal) + texture
    10,     // gooch shading (biased vertex normal) + texture

    11,     // vertex color (biased vertex normal) + cel shading + texture
    11,     // vertex color (biased vertex normal) + gooch shading + texture

    12,     // instancing + cel shading + texture
    12,     // instancing + gooch shading + texture

    13,     // instancing + vertex color + cel shading + texture
    13,     // instancing + vertex color + gooch shading + texture

    14,     // instancing (biased vertex normal) + cel shading + texture
    14,     // instancing (biased vertex normal) + gooch shading + texture

    15,     // instancing + vertex color (biased vertex normal) + cel shading + texture
    15,     // instancing + vertex color (biased vertex normal) + gooch shading + texture
};


template<>
const D3D12_SHADER_BYTECODE EffectBase<NPREffectTraits>::PixelShaderBytecode[] =
{
    { NPREffect_PSCelShading,     sizeof(NPREffect_PSCelShading)     },
    { NPREffect_PSGoochShading,   sizeof(NPREffect_PSGoochShading)   },
    { NPREffect_PSCelShadingTx,   sizeof(NPREffect_PSCelShadingTx)   },
    { NPREffect_PSGoochShadingTx, sizeof(NPREffect_PSGoochShadingTx) },
};


template<>
const int EffectBase<NPREffectTraits>::PixelShaderIndices[] =
{
    0,      // cel shading
    1,      // gooch shading

    0,      // vertex color + cel shading
    1,      // vertex color + gooch shading

    0,      // cel shading (biased vertex normal)
    1,      // gooch shading (biased vertex normal)

    0,      // vertex color (biased vertex normal) + cel shading
    1,      // vertex color (biased vertex normal) + gooch shading

    0,      // instancing + cel shading
    1,      // instancing + gooch shading

    0,      // instancing + vertex color + cel shading
    1,      // instancing + vertex color + gooch shading

    0,      // instancing (biased vertex normal) + cel shading
    1,      // instancing (biased vertex normal) + gooch shading

    0,      // instancing + vertex color (biased vertex normal) + cel shading
    1,      // instancing + vertex color (biased vertex normal) + gooch shading

    2,      // cel shading + texture
    3,      // gooch shading + texture

    2,      // vertex color + cel shading + texture
    3,      // vertex color + gooch shading + texture

    2,      // cel shading (biased vertex normal) + texture
    3,      // gooch shading (biased vertex normal) + texture

    2,      // vertex color (biased vertex normal) + cel shading + texture
    3,      // vertex color (biased vertex normal) + gooch shading + texture

    2,      // instancing + cel shading + texture
    3,      // instancing + gooch shading + texture

    2,      // instancing + vertex color + cel shading + texture
    3,      // instancing + vertex color + gooch shading + texture

    2,      // instancing (biased vertex normal) + cel shading + texture
    3,      // instancing (biased vertex normal) + gooch shading + texture

    2,      // instancing + vertex color (biased vertex normal) + cel shading + texture
    3,      // instancing + vertex color (biased vertex normal) + gooch shading + texture
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
    sampler{}
{
    static_assert(static_cast<int>(std::size(EffectBase<NPREffectTraits>::VertexShaderIndices)) == NPREffectTraits::ShaderPermutationCount, "array/max mismatch");
    static_assert(static_cast<int>(std::size(EffectBase<NPREffectTraits>::VertexShaderBytecode)) == NPREffectTraits::VertexShaderCount, "array/max mismatch");
    static_assert(static_cast<int>(std::size(EffectBase<NPREffectTraits>::PixelShaderBytecode)) == NPREffectTraits::PixelShaderCount, "array/max mismatch");
    static_assert(static_cast<int>(std::size(EffectBase<NPREffectTraits>::PixelShaderIndices)) == NPREffectTraits::ShaderPermutationCount, "array/max mismatch");

    constants.lightDirectionAndCelBands = s_defaultLightDir;
    constants.diffuseColorAndAlpha = s_defaultDiffuse;
    constants.specularColorAndSpecularPower = s_defaultSpecular;
    constants.goochCoolColorAndAlpha = s_defaultCool;
    constants.goochWarmColorAndBeta = s_defaultWarm;
    constants.eyePosition = g_XMZero;

    switch (nprMode)
    {
    case Mode_Cel:
    case Mode_Gooch:
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
        permutation += 2;
    }

    if (effectFlags & EffectFlags::BiasedVertexNormals)
    {
        // Compressed normals need to be scaled and biased in the vertex shader.
        permutation += 4;
    }

    if (effectFlags & EffectFlags::Instancing)
    {
        // Vertex shader needs to use vertex matrix transform.
        permutation += 8;
    }

    // Use shaders without texture coordinates?
    if (textureEnabled)
    {
        permutation += 16;
    }

    return permutation;
}


// Sets our state onto the D3D device.
void NPREffect::Impl::Apply(_In_ ID3D12GraphicsCommandList* commandList)
{
    // Compute derived parameter values.
    matrices.SetConstants(dirtyFlags, constants.worldViewProj);

    // World inverse transpose matrix.
    if (dirtyFlags & EffectDirtyFlags::WorldInverseTranspose)
    {
        constants.world = XMMatrixTranspose(matrices.world);

        const XMMATRIX worldInverse = XMMatrixInverse(nullptr, matrices.world);

        constants.worldInverseTranspose[0] = worldInverse.r[0];
        constants.worldInverseTranspose[1] = worldInverse.r[1];
        constants.worldInverseTranspose[2] = worldInverse.r[2];

        dirtyFlags &= ~EffectDirtyFlags::WorldInverseTranspose;
        dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
    }

    // Eye position.
    if (dirtyFlags & EffectDirtyFlags::EyePosition)
    {
        const XMMATRIX viewInverse = XMMatrixInverse(nullptr, matrices.view);
        constants.eyePosition = viewInverse.r[3];

        dirtyFlags &= ~EffectDirtyFlags::EyePosition;
        dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
    }

    UpdateConstants();

    // Set the root signature
    commandList->SetGraphicsRootSignature(mRootSignature);

    // Set the texture
    if (textureEnabled)
    {
        if (!texture.ptr || !sampler.ptr)
        {
            DebugTrace("ERROR: Missing texture or sampler for NPREffect (texture %llu, sampler %llu)\n", texture.ptr, sampler.ptr);
            throw std::runtime_error("NPREffect");
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
    // TODO: specular power needs to be specular threshold!
    // Set xyz, preserve w (specular power).
    pImpl->constants.specularColorAndSpecularPower = XMVectorSelect(pImpl->constants.specularColorAndSpecularPower, value, g_XMSelect1110);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void NPREffect::SetSpecularPower(float value)
{
    // Set w of specularColorAndSpecularPower.
    pImpl->constants.specularColorAndSpecularPower = XMVectorSetW(pImpl->constants.specularColorAndSpecularPower, value);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void NPREffect::DisableSpecular()
{
    // Set specular color to black, power to 1
    // Note: Don't use a power of 0 or the shader will generate strange highlights on non-specular materials

    pImpl->constants.specularColorAndSpecularPower = g_XMIdentityR3;

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


// Cel shading settings.
void NPREffect::SetCelShaderBands(int bands)
{
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
