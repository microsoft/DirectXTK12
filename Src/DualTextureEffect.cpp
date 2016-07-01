//--------------------------------------------------------------------------------------
// File: DualTextureEffect.cpp
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
struct DualTextureEffectConstants
{
    XMVECTOR diffuseColor;
    XMVECTOR fogColor;
    XMVECTOR fogVector;
    XMMATRIX worldViewProj;
};

static_assert( ( sizeof(DualTextureEffectConstants) % 16 ) == 0, "CB size not padded correctly" );


// Traits type describes our characteristics to the EffectBase template.
struct DualTextureEffectTraits
{
    typedef DualTextureEffectConstants ConstantBufferType;

    static const int VertexShaderCount = 4;
    static const int PixelShaderCount = 2;
    static const int ShaderPermutationCount = 4;
};


// Internal DualTextureEffect implementation class.
class DualTextureEffect::Impl : public EffectBase<DualTextureEffectTraits>
{
public:
    Impl(_In_ ID3D12Device* device, int effectFlags, const EffectPipelineStateDescription& pipelineDescription);
    
    enum RootParameterIndex
    {
        Texture1SRV,
        Texture2SRV,
        ConstantBuffer,
        RootParameterCount
    };

    bool vertexColorEnabled;
    EffectColor color;

    D3D12_GPU_DESCRIPTOR_HANDLE texture1;
    D3D12_GPU_DESCRIPTOR_HANDLE texture2;

    int GetCurrentPipelineStatePermutation() const;

    void Apply(_In_ ID3D12GraphicsCommandList* commandList);
};


// Include the precompiled shader code.
namespace
{
#if defined(_XBOX_ONE) && defined(_TITLE)
    #include "Shaders/Compiled/XboxOneDualTextureEffect_VSDualTexture.inc"
    #include "Shaders/Compiled/XboxOneDualTextureEffect_VSDualTextureNoFog.inc"
    #include "Shaders/Compiled/XboxOneDualTextureEffect_VSDualTextureVc.inc"
    #include "Shaders/Compiled/XboxOneDualTextureEffect_VSDualTextureVcNoFog.inc"

    #include "Shaders/Compiled/XboxOneDualTextureEffect_PSDualTexture.inc"
    #include "Shaders/Compiled/XboxOneDualTextureEffect_PSDualTextureNoFog.inc"
#else
    #include "Shaders/Compiled/DualTextureEffect_VSDualTexture.inc"
    #include "Shaders/Compiled/DualTextureEffect_VSDualTextureNoFog.inc"
    #include "Shaders/Compiled/DualTextureEffect_VSDualTextureVc.inc"
    #include "Shaders/Compiled/DualTextureEffect_VSDualTextureVcNoFog.inc"

    #include "Shaders/Compiled/DualTextureEffect_PSDualTexture.inc"
    #include "Shaders/Compiled/DualTextureEffect_PSDualTextureNoFog.inc"
#endif
}


const D3D12_SHADER_BYTECODE EffectBase<DualTextureEffectTraits>::VertexShaderBytecode[] =
{
    { DualTextureEffect_VSDualTexture,        sizeof(DualTextureEffect_VSDualTexture)        },
    { DualTextureEffect_VSDualTextureNoFog,   sizeof(DualTextureEffect_VSDualTextureNoFog)   },
    { DualTextureEffect_VSDualTextureVc,      sizeof(DualTextureEffect_VSDualTextureVc)      },
    { DualTextureEffect_VSDualTextureVcNoFog, sizeof(DualTextureEffect_VSDualTextureVcNoFog) },

};


const int EffectBase<DualTextureEffectTraits>::VertexShaderIndices[] =
{
    0,      // basic
    1,      // no fog
    2,      // vertex color
    3,      // vertex color, no fog
};


const D3D12_SHADER_BYTECODE EffectBase<DualTextureEffectTraits>::PixelShaderBytecode[] =
{
    { DualTextureEffect_PSDualTexture,        sizeof(DualTextureEffect_PSDualTexture)        },
    { DualTextureEffect_PSDualTextureNoFog,   sizeof(DualTextureEffect_PSDualTextureNoFog)   },

};


const int EffectBase<DualTextureEffectTraits>::PixelShaderIndices[] =
{
    0,      // basic
    1,      // no fog
    0,      // vertex color
    1,      // vertex color, no fog
};


// Global pool of per-device DualTextureEffect resources.
SharedResourcePool<ID3D12Device*, EffectBase<DualTextureEffectTraits>::DeviceResources> EffectBase<DualTextureEffectTraits>::deviceResourcesPool;


// Constructor.
DualTextureEffect::Impl::Impl(_In_ ID3D12Device* device, int flags, const EffectPipelineStateDescription& pipelineDescription)
    : EffectBase(device)
{
    static_assert(_countof(EffectBase<DualTextureEffectTraits>::VertexShaderIndices) == DualTextureEffectTraits::ShaderPermutationCount, "array/max mismatch");
    static_assert(_countof(EffectBase<DualTextureEffectTraits>::VertexShaderBytecode) == DualTextureEffectTraits::VertexShaderCount, "array/max mismatch");
    static_assert(_countof(EffectBase<DualTextureEffectTraits>::PixelShaderBytecode) == DualTextureEffectTraits::PixelShaderCount, "array/max mismatch");
    static_assert(_countof(EffectBase<DualTextureEffectTraits>::PixelShaderIndices) == DualTextureEffectTraits::ShaderPermutationCount, "array/max mismatch");
    
    {   // Create Root signature
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
        rootParameters[RootParameterIndex::Texture1SRV].InitAsDescriptorTable(1, &descriptorRange1);
        
        // Texture 2
        CD3DX12_DESCRIPTOR_RANGE descriptorRange2;
        descriptorRange2.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
        rootParameters[RootParameterIndex::Texture2SRV].InitAsDescriptorTable(1, &descriptorRange2);

        // Create the root signature
        CD3DX12_ROOT_SIGNATURE_DESC rsigDesc;
        rsigDesc.Init(_countof(rootParameters), rootParameters, _countof(samplers), samplers, rootSignatureFlags);

        ThrowIfFailed(CreateRootSignature(device, &rsigDesc, mRootSignature.ReleaseAndGetAddressOf()));
    }

    // Validate flags & state
    fog.enabled = (flags & EffectFlags::Fog) != 0;
    vertexColorEnabled = (flags & EffectFlags::VertexColor) != 0;

    {   // Create pipeline state
        int sp = GetCurrentPipelineStatePermutation();
        int vi = EffectBase<DualTextureEffectTraits>::VertexShaderIndices[sp];
        int pi = EffectBase<DualTextureEffectTraits>::PixelShaderIndices[sp];

        EffectBase::CreatePipelineState(
            mRootSignature.Get(),
            pipelineDescription.inputLayout,
            &EffectBase<DualTextureEffectTraits>::VertexShaderBytecode[vi],
            &EffectBase<DualTextureEffectTraits>::PixelShaderBytecode[pi],
            pipelineDescription.blendDesc,
            pipelineDescription.depthStencilDesc,
            pipelineDescription.rasterizerDesc,
            pipelineDescription.renderTargetState,
            pipelineDescription.primitiveTopology,
            pipelineDescription.stripCutValue);
    }
}


int DualTextureEffect::Impl::GetCurrentPipelineStatePermutation() const
{
    int permutation = 0;

    // Use optimized shaders if fog is disabled.
    if (!fog.enabled)
    {
        permutation += 1;
    }

    // Support vertex coloring?
    if (vertexColorEnabled)
    {
        permutation += 2;
    }

    return permutation;
}


// Sets our state onto the D3D device.
void DualTextureEffect::Impl::Apply(_In_ ID3D12GraphicsCommandList* commandList)
{
    // Compute derived parameter values.
    matrices.SetConstants(dirtyFlags, constants.worldViewProj);

    fog.SetConstants(dirtyFlags, matrices.worldView, constants.fogVector);

    color.SetConstants(dirtyFlags, constants.diffuseColor);

    UpdateConstants();

    // Set the root signature
    commandList->SetGraphicsRootSignature(mRootSignature.Get());
    commandList->SetGraphicsRootDescriptorTable(RootParameterIndex::Texture1SRV, texture1);
    commandList->SetGraphicsRootDescriptorTable(RootParameterIndex::Texture2SRV, texture2);
    commandList->SetGraphicsRootConstantBufferView(RootParameterIndex::ConstantBuffer, GetConstantBufferGpuAddress());

    // Set the pipeline state
    commandList->SetPipelineState(EffectBase::mPipelineState.Get());
}


// Public constructor.
DualTextureEffect::DualTextureEffect(_In_ ID3D12Device* device, int effectFlags, const EffectPipelineStateDescription& pipelineDescription)
    : pImpl(new Impl(device, effectFlags, pipelineDescription))
{
}


// Move constructor.
DualTextureEffect::DualTextureEffect(DualTextureEffect&& moveFrom)
  : pImpl(std::move(moveFrom.pImpl))
{
}


// Move assignment.
DualTextureEffect& DualTextureEffect::operator= (DualTextureEffect&& moveFrom)
{
    pImpl = std::move(moveFrom.pImpl);
    return *this;
}


// Public destructor.
DualTextureEffect::~DualTextureEffect()
{
}


void DualTextureEffect::Apply(_In_ ID3D12GraphicsCommandList* commandList)
{
    pImpl->Apply(commandList);
}

void XM_CALLCONV DualTextureEffect::SetWorld(FXMMATRIX value)
{
    pImpl->matrices.world = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj | EffectDirtyFlags::WorldInverseTranspose | EffectDirtyFlags::FogVector;
}


void XM_CALLCONV DualTextureEffect::SetView(FXMMATRIX value)
{
    pImpl->matrices.view = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj | EffectDirtyFlags::EyePosition | EffectDirtyFlags::FogVector;
}


void XM_CALLCONV DualTextureEffect::SetProjection(FXMMATRIX value)
{
    pImpl->matrices.projection = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj;
}


void XM_CALLCONV DualTextureEffect::SetDiffuseColor(FXMVECTOR value)
{
    pImpl->color.diffuseColor = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::MaterialColor;
}


void DualTextureEffect::SetAlpha(float value)
{
    pImpl->color.alpha = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::MaterialColor;
}

void DualTextureEffect::SetFogStart(float value)
{
    pImpl->fog.start = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::FogVector;
}


void DualTextureEffect::SetFogEnd(float value)
{
    pImpl->fog.end = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::FogVector;
}


void XM_CALLCONV DualTextureEffect::SetFogColor(FXMVECTOR value)
{
    pImpl->constants.fogColor = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void DualTextureEffect::SetTexture(_In_ D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor)
{
    pImpl->texture1 = srvDescriptor;
}

void DualTextureEffect::SetTexture2(_In_ D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor)
{
    pImpl->texture2 = srvDescriptor;
}
