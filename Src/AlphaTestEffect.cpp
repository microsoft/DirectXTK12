//--------------------------------------------------------------------------------------
// File: AlphaTestEffect.cpp
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


// Constant buffer layout. Must match the shader!
struct AlphaTestEffectConstants
{
    XMVECTOR diffuseColor;
    XMVECTOR alphaTest;
    XMVECTOR fogColor;
    XMVECTOR fogVector;
    XMMATRIX worldViewProj;
};

static_assert( ( sizeof(AlphaTestEffectConstants) % 16 ) == 0, "CB size not padded correctly" );


// Traits type describes our characteristics to the EffectBase template.
struct AlphaTestEffectTraits
{
    typedef AlphaTestEffectConstants ConstantBufferType;

    static const int VertexShaderCount = 4;
    static const int PixelShaderCount = 4;
    static const int ShaderPermutationCount = 8;
};


// Internal AlphaTestEffect implementation class.
class AlphaTestEffect::Impl : public EffectBase<AlphaTestEffectTraits>
{
public:
    Impl(_In_ ID3D12Device* device, int effectFlags, const EffectPipelineStateDescription& pipelineDescription, D3D12_COMPARISON_FUNC alphaFunction);

    enum DescriptorIndex
    {
        Texture,
        DescriptorCount
    };

    enum RootParameterIndex
    {
        TextureSRV,
        ConstantBuffer,
        RootParameterCount
    };

    D3D12_COMPARISON_FUNC mAlphaFunction;
    int referenceAlpha;

    bool vertexColorEnabled;
    EffectColor color;

    D3D12_GPU_DESCRIPTOR_HANDLE texture;
    
    int GetCurrentPipelineStatePermutation() const;

    void Apply(_In_ ID3D12GraphicsCommandList* commandList);
};


// Include the precompiled shader code.
namespace
{
#if defined(_XBOX_ONE) && defined(_TITLE)
    #include "Shaders/Compiled/XboxOneAlphaTestEffect_VSAlphaTest.inc"
    #include "Shaders/Compiled/XboxOneAlphaTestEffect_VSAlphaTestNoFog.inc"
    #include "Shaders/Compiled/XboxOneAlphaTestEffect_VSAlphaTestVc.inc"
    #include "Shaders/Compiled/XboxOneAlphaTestEffect_VSAlphaTestVcNoFog.inc"

    #include "Shaders/Compiled/XboxOneAlphaTestEffect_PSAlphaTestLtGt.inc"
    #include "Shaders/Compiled/XboxOneAlphaTestEffect_PSAlphaTestLtGtNoFog.inc"
    #include "Shaders/Compiled/XboxOneAlphaTestEffect_PSAlphaTestEqNe.inc"
    #include "Shaders/Compiled/XboxOneAlphaTestEffect_PSAlphaTestEqNeNoFog.inc"
#else
    #include "Shaders/Compiled/AlphaTestEffect_VSAlphaTest.inc"
    #include "Shaders/Compiled/AlphaTestEffect_VSAlphaTestNoFog.inc"
    #include "Shaders/Compiled/AlphaTestEffect_VSAlphaTestVc.inc"
    #include "Shaders/Compiled/AlphaTestEffect_VSAlphaTestVcNoFog.inc"

    #include "Shaders/Compiled/AlphaTestEffect_PSAlphaTestLtGt.inc"
    #include "Shaders/Compiled/AlphaTestEffect_PSAlphaTestLtGtNoFog.inc"
    #include "Shaders/Compiled/AlphaTestEffect_PSAlphaTestEqNe.inc"
    #include "Shaders/Compiled/AlphaTestEffect_PSAlphaTestEqNeNoFog.inc"
#endif
}


const D3D12_SHADER_BYTECODE EffectBase<AlphaTestEffectTraits>::VertexShaderBytecode[] =
{
    { AlphaTestEffect_VSAlphaTest,        sizeof(AlphaTestEffect_VSAlphaTest)        },
    { AlphaTestEffect_VSAlphaTestNoFog,   sizeof(AlphaTestEffect_VSAlphaTestNoFog)   },
    { AlphaTestEffect_VSAlphaTestVc,      sizeof(AlphaTestEffect_VSAlphaTestVc)      },
    { AlphaTestEffect_VSAlphaTestVcNoFog, sizeof(AlphaTestEffect_VSAlphaTestVcNoFog) },
};


const int EffectBase<AlphaTestEffectTraits>::VertexShaderIndices[] =
{
    0,      // lt/gt
    1,      // lt/gt, no fog
    2,      // lt/gt, vertex color
    3,      // lt/gt, vertex color, no fog
    
    0,      // eq/ne
    1,      // eq/ne, no fog
    2,      // eq/ne, vertex color
    3,      // eq/ne, vertex color, no fog
};


const D3D12_SHADER_BYTECODE EffectBase<AlphaTestEffectTraits>::PixelShaderBytecode[] =
{
    { AlphaTestEffect_PSAlphaTestLtGt,      sizeof(AlphaTestEffect_PSAlphaTestLtGt)      },
    { AlphaTestEffect_PSAlphaTestLtGtNoFog, sizeof(AlphaTestEffect_PSAlphaTestLtGtNoFog) },
    { AlphaTestEffect_PSAlphaTestEqNe,      sizeof(AlphaTestEffect_PSAlphaTestEqNe)      },
    { AlphaTestEffect_PSAlphaTestEqNeNoFog, sizeof(AlphaTestEffect_PSAlphaTestEqNeNoFog) },
};


const int EffectBase<AlphaTestEffectTraits>::PixelShaderIndices[] =
{
    0,      // lt/gt
    1,      // lt/gt, no fog
    0,      // lt/gt, vertex color
    1,      // lt/gt, vertex color, no fog
    
    2,      // eq/ne
    3,      // eq/ne, no fog
    2,      // eq/ne, vertex color
    3,      // eq/ne, vertex color, no fog
};


// Global pool of per-device AlphaTestEffect resources.
SharedResourcePool<ID3D12Device*, EffectBase<AlphaTestEffectTraits>::DeviceResources> EffectBase<AlphaTestEffectTraits>::deviceResourcesPool;

// Constructor.
AlphaTestEffect::Impl::Impl(_In_ ID3D12Device* device, 
    int flags, const EffectPipelineStateDescription& pipelineDescription, D3D12_COMPARISON_FUNC alphaFunction)
  : EffectBase(device),
    mAlphaFunction(alphaFunction),
    referenceAlpha(0),
    vertexColorEnabled(false)
{
    static_assert( _countof(EffectBase<AlphaTestEffectTraits>::VertexShaderIndices) == AlphaTestEffectTraits::ShaderPermutationCount, "array/max mismatch" );
    static_assert( _countof(EffectBase<AlphaTestEffectTraits>::VertexShaderBytecode) == AlphaTestEffectTraits::VertexShaderCount, "array/max mismatch" );
    static_assert( _countof(EffectBase<AlphaTestEffectTraits>::PixelShaderBytecode) == AlphaTestEffectTraits::PixelShaderCount, "array/max mismatch" );
    static_assert( _countof(EffectBase<AlphaTestEffectTraits>::PixelShaderIndices) == AlphaTestEffectTraits::ShaderPermutationCount, "array/max mismatch" );

    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | // Only the input assembler stage needs access to the constant buffer.
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;

    CD3DX12_STATIC_SAMPLER_DESC sampler(0);
    CD3DX12_DESCRIPTOR_RANGE descriptorRanges[DescriptorIndex::DescriptorCount];
    descriptorRanges[DescriptorIndex::Texture].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    CD3DX12_ROOT_PARAMETER rootParameters[RootParameterIndex::RootParameterCount];
    rootParameters[RootParameterIndex::TextureSRV].InitAsDescriptorTable(
        _countof(descriptorRanges),
        descriptorRanges);
    rootParameters[RootParameterIndex::ConstantBuffer].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
    CD3DX12_ROOT_SIGNATURE_DESC rsigDesc;
    rsigDesc.Init(_countof(rootParameters), rootParameters, 1, &sampler, rootSignatureFlags);

    ThrowIfFailed(CreateRootSignature(device, &rsigDesc, mRootSignature.ReleaseAndGetAddressOf()));

    fog.enabled = (flags & EffectFlags::Fog) != 0;
    vertexColorEnabled = (flags & EffectFlags::VertexColor) != 0;
  
    // overridden alpha test state
    D3D12_DEPTH_STENCIL_DESC override = *pipelineDescription.depthStencilDesc;
    override.BackFace.StencilFunc = alphaFunction;
    override.FrontFace.StencilFunc = alphaFunction;

    int sp = GetCurrentPipelineStatePermutation();
    int vi = EffectBase<AlphaTestEffectTraits>::VertexShaderIndices[sp];
    int pi = EffectBase<AlphaTestEffectTraits>::PixelShaderIndices[sp];
  
    EffectBase::CreatePipelineState(
        mRootSignature.Get(),
        pipelineDescription.inputLayout,
        &EffectBase<AlphaTestEffectTraits>::VertexShaderBytecode[vi],
        &EffectBase<AlphaTestEffectTraits>::PixelShaderBytecode[pi],
        pipelineDescription.blendDesc,
        &override,
        pipelineDescription.rasterizerDesc,
        pipelineDescription.renderTargetState,
        pipelineDescription.primitiveTopology,
        pipelineDescription.stripCutValue);
}


int AlphaTestEffect::Impl::GetCurrentPipelineStatePermutation() const
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

    // Which alpha compare mode?
    if (mAlphaFunction == D3D12_COMPARISON_FUNC_EQUAL ||
        mAlphaFunction == D3D12_COMPARISON_FUNC_NOT_EQUAL)
    {
        permutation += 4;
    }

    return permutation;
}


// Sets our state onto the D3D device.
void AlphaTestEffect::Impl::Apply(_In_ ID3D12GraphicsCommandList* commandList)
{
    // Compute derived parameter values.
    matrices.SetConstants(dirtyFlags, constants.worldViewProj);
    fog.SetConstants(dirtyFlags, matrices.worldView, constants.fogVector);            
    color.SetConstants(dirtyFlags, constants.diffuseColor);

    UpdateConstants();

    // Recompute the alpha test settings?
    if (dirtyFlags & EffectDirtyFlags::AlphaTest)
    {
        // Convert reference alpha from 8 bit integer to 0-1 float format.
        float reference = (float)referenceAlpha / 255.0f;
                
        // Comparison tolerance of half the 8 bit integer precision.
        const float threshold = 0.5f / 255.0f;

        // What to do if the alpha comparison passes or fails. Positive accepts the pixel, negative clips it.
        static const XMVECTORF32 selectIfTrue  = {  1, -1 };
        static const XMVECTORF32 selectIfFalse = { -1,  1 };
        static const XMVECTORF32 selectNever   = { -1, -1 };
        static const XMVECTORF32 selectAlways  = {  1,  1 };

        float compareTo;
        XMVECTOR resultSelector;

        switch (mAlphaFunction)
        {
            case D3D12_COMPARISON_FUNC_LESS:
                // Shader will evaluate: clip((a < x) ? z : w)
                compareTo = reference - threshold;
                resultSelector = selectIfTrue;
                break;

            case D3D12_COMPARISON_FUNC_LESS_EQUAL:
                // Shader will evaluate: clip((a < x) ? z : w)
                compareTo = reference + threshold;
                resultSelector = selectIfTrue;
                break;

            case D3D12_COMPARISON_FUNC_GREATER_EQUAL:
                // Shader will evaluate: clip((a < x) ? z : w)
                compareTo = reference - threshold;
                resultSelector = selectIfFalse;
                break;

            case D3D12_COMPARISON_FUNC_GREATER:
                // Shader will evaluate: clip((a < x) ? z : w)
                compareTo = reference + threshold;
                resultSelector = selectIfFalse;
                break;

            case D3D12_COMPARISON_FUNC_EQUAL:
                // Shader will evaluate: clip((abs(a - x) < y) ? z : w)
                compareTo = reference;
                resultSelector = selectIfTrue;
                break;

            case D3D12_COMPARISON_FUNC_NOT_EQUAL:
                // Shader will evaluate: clip((abs(a - x) < y) ? z : w)
                compareTo = reference;
                resultSelector = selectIfFalse;
                break;

            case D3D12_COMPARISON_FUNC_NEVER:
                // Shader will evaluate: clip((a < x) ? z : w)
                compareTo = 0;
                resultSelector = selectNever;
                break;

            case D3D12_COMPARISON_FUNC_ALWAYS:
                // Shader will evaluate: clip((a < x) ? z : w)
                compareTo = 0;
                resultSelector = selectAlways;
                break;

            default:
                throw std::exception("Unknown alpha test function");
        }

        // x = compareTo, y = threshold, zw = resultSelector.
        constants.alphaTest = XMVectorPermute<0, 1, 4, 5>(XMVectorSet(compareTo, threshold, 0, 0), resultSelector);
                
        dirtyFlags &= ~EffectDirtyFlags::AlphaTest;
        dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
    }

    // Set the root signature
    commandList->SetGraphicsRootSignature(mRootSignature.Get());

    // Set the texture.
    commandList->SetGraphicsRootDescriptorTable(RootParameterIndex::TextureSRV, texture);
    commandList->SetGraphicsRootConstantBufferView(RootParameterIndex::ConstantBuffer, GetConstantBufferGpuAddress());

    // Set the pipeline state
    commandList->SetPipelineState(EffectBase::mPipelineState.Get());
}

// Public constructor.
AlphaTestEffect::AlphaTestEffect(_In_ ID3D12Device* device, int effectFlags, const EffectPipelineStateDescription& pipelineDescription, D3D12_COMPARISON_FUNC alphaFunction)
    : pImpl(new Impl(device, effectFlags, pipelineDescription, alphaFunction))
{
}


// Move constructor.
AlphaTestEffect::AlphaTestEffect(AlphaTestEffect&& moveFrom)
  : pImpl(std::move(moveFrom.pImpl))
{
}


// Move assignment.
AlphaTestEffect& AlphaTestEffect::operator= (AlphaTestEffect&& moveFrom)
{
    pImpl = std::move(moveFrom.pImpl);
    return *this;
}


// Public destructor.
AlphaTestEffect::~AlphaTestEffect()
{
}


void AlphaTestEffect::Apply(_In_ ID3D12GraphicsCommandList* commandList)
{
    pImpl->Apply(commandList);
}

void XM_CALLCONV AlphaTestEffect::SetWorld(FXMMATRIX value)
{
    pImpl->matrices.world = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj | EffectDirtyFlags::WorldInverseTranspose | EffectDirtyFlags::FogVector;
}


void XM_CALLCONV AlphaTestEffect::SetView(FXMMATRIX value)
{
    pImpl->matrices.view = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj | EffectDirtyFlags::EyePosition | EffectDirtyFlags::FogVector;
}


void XM_CALLCONV AlphaTestEffect::SetProjection(FXMMATRIX value)
{
    pImpl->matrices.projection = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj;
}


void XM_CALLCONV AlphaTestEffect::SetDiffuseColor(FXMVECTOR value)
{
    pImpl->color.diffuseColor = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::MaterialColor;
}


void AlphaTestEffect::SetAlpha(float value)
{
    pImpl->color.alpha = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::MaterialColor;
}

void AlphaTestEffect::SetFogStart(float value)
{
    pImpl->fog.start = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::FogVector;
}


void AlphaTestEffect::SetFogEnd(float value)
{
    pImpl->fog.end = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::FogVector;
}


void XM_CALLCONV AlphaTestEffect::SetFogColor(FXMVECTOR value)
{
    pImpl->constants.fogColor = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}

void AlphaTestEffect::SetTexture(_In_ D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor)
{
    pImpl->texture = srvDescriptor;
}

void AlphaTestEffect::SetReferenceAlpha(int value)
{
    pImpl->referenceAlpha = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::AlphaTest;
}
