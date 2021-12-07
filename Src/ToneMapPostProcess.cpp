//--------------------------------------------------------------------------------------
// File: ToneMapPostProcess.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "PostProcess.h"

#include "AlignedNew.h"
#include "CommonStates.h"
#include "DemandCreate.h"
#include "DirectXHelpers.h"
#include "EffectPipelineStateDescription.h"
#include "GraphicsMemory.h"
#include "SharedResourcePool.h"

using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
    constexpr int Dirty_ConstantBuffer  = 0x01;
    constexpr int Dirty_Parameters      = 0x02;

#if (defined(_XBOX_ONE) && defined(_TITLE)) || defined(_GAMING_XBOX)
    constexpr int PixelShaderCount = 15;
    constexpr int ShaderPermutationCount = 24;
#else
    constexpr int PixelShaderCount = 9;
    constexpr int ShaderPermutationCount = 12;
#endif

    // Constant buffer layout. Must match the shader!
    XM_ALIGNED_STRUCT(16) ToneMapConstants
    {
        // linearExposure is .x
        // paperWhiteNits is .y
        XMVECTOR parameters;
        XMVECTOR colorRotation[3];
    };

    static_assert((sizeof(ToneMapConstants) % 16) == 0, "CB size not padded correctly");

    // HDTV to UHDTV (Rec.709 color primaries into Rec.2020)
    constexpr float c_from709to2020[12] =
    {
          0.6274040f, 0.3292820f, 0.0433136f, 0.f,
          0.0690970f, 0.9195400f, 0.0113612f, 0.f,
          0.0163916f, 0.0880132f, 0.8955950f, 0.f,
    };

    // DCI-P3-D65 https://en.wikipedia.org/wiki/DCI-P3 to UHDTV (DCI-P3-D65 color primaries into Rec.2020)
    constexpr float c_fromP3D65to2020[12] =
    {
           0.753845f,  0.198593f,  0.047562f, 0.f,
          0.0457456f,  0.941777f, 0.0124772f, 0.f,
        -0.00121055f, 0.0176041f,  0.983607f, 0.f,
    };

    // HDTV to DCI-P3-D65 (a.k.a. Display P3 or P3D65)
    constexpr float c_from709toP3D65[12] =
    {
        0.822461969f, 0.1775380f,        0.f, 0.f,
        0.033194199f, 0.9668058f,        0.f, 0.f,
        0.017082631f, 0.0723974f, 0.9105199f, 0.f,
    };
}

// Include the precompiled shader code.
namespace
{
#ifdef _GAMING_XBOX_SCARLETT
    #include "XboxGamingScarlettToneMap_VSQuad.inc"

    #include "XboxGamingScarlettToneMap_PSCopy.inc"
    #include "XboxGamingScarlettToneMap_PSSaturate.inc"
    #include "XboxGamingScarlettToneMap_PSReinhard.inc"
    #include "XboxGamingScarlettToneMap_PSACESFilmic.inc"
    #include "XboxGamingScarlettToneMap_PS_SRGB.inc"
    #include "XboxGamingScarlettToneMap_PSSaturate_SRGB.inc"
    #include "XboxGamingScarlettToneMap_PSReinhard_SRGB.inc"
    #include "XboxGamingScarlettToneMap_PSACESFilmic_SRGB.inc"
    #include "XboxGamingScarlettToneMap_PSHDR10.inc"
    #include "XboxGamingScarlettToneMap_PSHDR10_Saturate.inc"
    #include "XboxGamingScarlettToneMap_PSHDR10_Reinhard.inc"
    #include "XboxGamingScarlettToneMap_PSHDR10_ACESFilmic.inc"
    #include "XboxGamingScarlettToneMap_PSHDR10_Saturate_SRGB.inc"
    #include "XboxGamingScarlettToneMap_PSHDR10_Reinhard_SRGB.inc"
    #include "XboxGamingScarlettToneMap_PSHDR10_ACESFilmic_SRGB.inc"
#elif defined(_GAMING_XBOX)
    #include "XboxGamingXboxOneToneMap_VSQuad.inc"

    #include "XboxGamingXboxOneToneMap_PSCopy.inc"
    #include "XboxGamingXboxOneToneMap_PSSaturate.inc"
    #include "XboxGamingXboxOneToneMap_PSReinhard.inc"
    #include "XboxGamingXboxOneToneMap_PSACESFilmic.inc"
    #include "XboxGamingXboxOneToneMap_PS_SRGB.inc"
    #include "XboxGamingXboxOneToneMap_PSSaturate_SRGB.inc"
    #include "XboxGamingXboxOneToneMap_PSReinhard_SRGB.inc"
    #include "XboxGamingXboxOneToneMap_PSACESFilmic_SRGB.inc"
    #include "XboxGamingXboxOneToneMap_PSHDR10.inc"
    #include "XboxGamingXboxOneToneMap_PSHDR10_Saturate.inc"
    #include "XboxGamingXboxOneToneMap_PSHDR10_Reinhard.inc"
    #include "XboxGamingXboxOneToneMap_PSHDR10_ACESFilmic.inc"
    #include "XboxGamingXboxOneToneMap_PSHDR10_Saturate_SRGB.inc"
    #include "XboxGamingXboxOneToneMap_PSHDR10_Reinhard_SRGB.inc"
    #include "XboxGamingXboxOneToneMap_PSHDR10_ACESFilmic_SRGB.inc"
#elif defined(_XBOX_ONE) && defined(_TITLE)
    #include "XboxOneToneMap_VSQuad.inc"

    #include "XboxOneToneMap_PSCopy.inc"
    #include "XboxOneToneMap_PSSaturate.inc"
    #include "XboxOneToneMap_PSReinhard.inc"
    #include "XboxOneToneMap_PSACESFilmic.inc"
    #include "XboxOneToneMap_PS_SRGB.inc"
    #include "XboxOneToneMap_PSSaturate_SRGB.inc"
    #include "XboxOneToneMap_PSReinhard_SRGB.inc"
    #include "XboxOneToneMap_PSACESFilmic_SRGB.inc"
    #include "XboxOneToneMap_PSHDR10.inc"
    #include "XboxOneToneMap_PSHDR10_Saturate.inc"
    #include "XboxOneToneMap_PSHDR10_Reinhard.inc"
    #include "XboxOneToneMap_PSHDR10_ACESFilmic.inc"
    #include "XboxOneToneMap_PSHDR10_Saturate_SRGB.inc"
    #include "XboxOneToneMap_PSHDR10_Reinhard_SRGB.inc"
    #include "XboxOneToneMap_PSHDR10_ACESFilmic_SRGB.inc"
#else
    #include "ToneMap_VSQuad.inc"

    #include "ToneMap_PSCopy.inc"
    #include "ToneMap_PSSaturate.inc"
    #include "ToneMap_PSReinhard.inc"
    #include "ToneMap_PSACESFilmic.inc"
    #include "ToneMap_PS_SRGB.inc"
    #include "ToneMap_PSSaturate_SRGB.inc"
    #include "ToneMap_PSReinhard_SRGB.inc"
    #include "ToneMap_PSACESFilmic_SRGB.inc"
    #include "ToneMap_PSHDR10.inc"
#endif
}

namespace
{
    const D3D12_SHADER_BYTECODE vertexShader =
        { ToneMap_VSQuad,                   sizeof(ToneMap_VSQuad) };

    const D3D12_SHADER_BYTECODE pixelShaders[] =
    {
        { ToneMap_PSCopy,                   sizeof(ToneMap_PSCopy) },
        { ToneMap_PSSaturate,               sizeof(ToneMap_PSSaturate) },
        { ToneMap_PSReinhard,               sizeof(ToneMap_PSReinhard) },
        { ToneMap_PSACESFilmic,             sizeof(ToneMap_PSACESFilmic) },
        { ToneMap_PS_SRGB,                  sizeof(ToneMap_PS_SRGB) },
        { ToneMap_PSSaturate_SRGB,          sizeof(ToneMap_PSSaturate_SRGB) },
        { ToneMap_PSReinhard_SRGB,          sizeof(ToneMap_PSReinhard_SRGB) },
        { ToneMap_PSACESFilmic_SRGB,        sizeof(ToneMap_PSACESFilmic_SRGB) },
        { ToneMap_PSHDR10,                  sizeof(ToneMap_PSHDR10) },

#if (defined(_XBOX_ONE) && defined(_TITLE)) || defined(_GAMING_XBOX)
        // Shaders that generate both HDR10 and GameDVR SDR signals via Multiple Render Targets.
        { ToneMap_PSHDR10_Saturate,         sizeof(ToneMap_PSHDR10_Saturate) },
        { ToneMap_PSHDR10_Reinhard,         sizeof(ToneMap_PSHDR10_Reinhard) },
        { ToneMap_PSHDR10_ACESFilmic,       sizeof(ToneMap_PSHDR10_ACESFilmic) },
        { ToneMap_PSHDR10_Saturate_SRGB,    sizeof(ToneMap_PSHDR10_Saturate_SRGB) },
        { ToneMap_PSHDR10_Reinhard_SRGB,    sizeof(ToneMap_PSHDR10_Reinhard_SRGB) },
        { ToneMap_PSHDR10_ACESFilmic_SRGB,  sizeof(ToneMap_PSHDR10_ACESFilmic_SRGB) },
#endif
    };

    static_assert(static_cast<int>(std::size(pixelShaders)) == PixelShaderCount, "array/max mismatch");

    const int pixelShaderIndices[] =
    {
        // Linear EOTF
        0,  // Copy
        1,  // Saturate
        2,  // Reinhard
        3,  // ACES Filmic

        // Gamam22 EOTF
        4,  // SRGB
        5,  // Saturate_SRGB
        6,  // Reinhard_SRGB
        7,  // ACES Filmic

        // ST.2084 EOTF
        8,  // HDR10
        8,  // HDR10
        8,  // HDR10
        8,  // HDR10

#if (defined(_XBOX_ONE) && defined(_TITLE)) || defined(_GAMING_XBOX)
        // MRT Linear EOTF
        9,  // HDR10+Saturate
        9,  // HDR10+Saturate
        10, // HDR10+Reinhard
        11, // HDR10+ACESFilmic

        // MRT Gamma22 EOTF
        12, // HDR10+Saturate_SRGB
        12, // HDR10+Saturate_SRGB
        13, // HDR10+Reinhard_SRGB
        14,  // HDR10+ACESFilmic

        // MRT ST.2084 EOTF
        9,  // HDR10+Saturate
        9,  // HDR10+Saturate
        10, // HDR10+Reinhard
        11, // HDR10+ACESFilmic
#endif
    };

    static_assert(static_cast<int>(std::size(pixelShaderIndices)) == ShaderPermutationCount, "array/max mismatch");

    // Factory for lazily instantiating shared root signatures.
    class DeviceResources
    {
    public:
        DeviceResources(_In_ ID3D12Device* device) noexcept
            : mDevice(device)
        { }

        ID3D12RootSignature* GetRootSignature(const D3D12_ROOT_SIGNATURE_DESC& desc)
        {
            return DemandCreate(mRootSignature, mMutex, [&](ID3D12RootSignature** pResult) noexcept -> HRESULT
            {
                HRESULT hr = CreateRootSignature(mDevice.Get(), &desc, pResult);

                if (SUCCEEDED(hr))
                    SetDebugObjectName(*pResult, L"ToneMapPostProcess");

                return hr;
            });
        }

        ID3D12Device* GetDevice() const noexcept { return mDevice.Get(); }

    protected:
        ComPtr<ID3D12Device>        mDevice;
        ComPtr<ID3D12RootSignature> mRootSignature;
        std::mutex                  mMutex;
    };
}

class ToneMapPostProcess::Impl : public AlignedNew<ToneMapConstants>
{
public:
    Impl(_In_ ID3D12Device* device, const RenderTargetState& rtState, Operator op, TransferFunction func, bool mrt = false);

    void Process(_In_ ID3D12GraphicsCommandList* commandList);

    void SetDirtyFlag() noexcept { mDirtyFlags = INT_MAX; }

    enum RootParameterIndex
    {
        TextureSRV,
        ConstantBuffer,
        RootParameterCount
    };

    // Fields.
    ToneMapConstants                        constants;
    D3D12_GPU_DESCRIPTOR_HANDLE             texture;
    float                                   linearExposure;
    float                                   paperWhiteNits;

private:
    int                                     mDirtyFlags;

   // D3D constant buffer holds a copy of the same data as the public 'constants' field.
    GraphicsResource mConstantBuffer;

    // Per instance cache of PSOs, populated with variants for each shader & layout
    Microsoft::WRL::ComPtr<ID3D12PipelineState> mPipelineState;

    // Per instance root signature
    ID3D12RootSignature* mRootSignature;

    // Per-device resources.
    std::shared_ptr<DeviceResources> mDeviceResources;

    static SharedResourcePool<ID3D12Device*, DeviceResources> deviceResourcesPool;
};


// Global pool of per-device ToneMapPostProcess resources.
SharedResourcePool<ID3D12Device*, DeviceResources> ToneMapPostProcess::Impl::deviceResourcesPool;


// Constructor.
ToneMapPostProcess::Impl::Impl(_In_ ID3D12Device* device, const RenderTargetState& rtState, Operator op, TransferFunction func, bool mrt)
    : constants{},
    texture{},
    linearExposure(1.f),
    paperWhiteNits(200.f),
    mDirtyFlags(INT_MAX),
    mDeviceResources(deviceResourcesPool.DemandCreate(device))
{
    if (op >= Operator_Max)
        throw std::invalid_argument("Tonemap operator not defined");

    if (func > TransferFunction_Max)
        throw std::invalid_argument("Transfer function not defined");

    // Create root signature.
    {
        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;

        CD3DX12_DESCRIPTOR_RANGE textureSRVs(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

        // Same as CommonStates::StaticPointClamp
        CD3DX12_STATIC_SAMPLER_DESC sampler(
            0, // register
            D3D12_FILTER_MIN_MAG_MIP_POINT,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            0.f,
            16,
            D3D12_COMPARISON_FUNC_LESS_EQUAL,
            D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
            0.f,
            D3D12_FLOAT32_MAX,
            D3D12_SHADER_VISIBILITY_PIXEL);

        CD3DX12_ROOT_PARAMETER rootParameters[RootParameterIndex::RootParameterCount] = {};

        CD3DX12_DESCRIPTOR_RANGE texture1Range(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
        rootParameters[RootParameterIndex::TextureSRV].InitAsDescriptorTable(1, &textureSRVs, D3D12_SHADER_VISIBILITY_PIXEL);

        // Root parameter descriptor
        CD3DX12_ROOT_SIGNATURE_DESC rsigDesc = {};

        // Constant buffer
        rootParameters[RootParameterIndex::ConstantBuffer].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);

        rsigDesc.Init(static_cast<UINT>(std::size(rootParameters)), rootParameters, 1, &sampler, rootSignatureFlags);

        mRootSignature = mDeviceResources->GetRootSignature(rsigDesc);
    }

    assert(mRootSignature != nullptr);

    // Determine shader permutation.
#if (defined(_XBOX_ONE) && defined(_TITLE)) || defined(_GAMING_XBOX)
    int permutation = (mrt) ? 12 : 0;
    permutation += (static_cast<int>(func) * static_cast<int>(Operator_Max)) + static_cast<int>(op);
#else
    UNREFERENCED_PARAMETER(mrt);
    int permutation = (static_cast<int>(func) * static_cast<int>(Operator_Max)) + static_cast<int>(op);
#endif

    assert(permutation >= 0 && permutation < ShaderPermutationCount);
    _Analysis_assume_(permutation >= 0 && permutation < ShaderPermutationCount);

    int shaderIndex = pixelShaderIndices[permutation];
    assert(shaderIndex >= 0 && shaderIndex < PixelShaderCount);
    _Analysis_assume_(shaderIndex >= 0 && shaderIndex < PixelShaderCount);

    // Create pipeline state.
    EffectPipelineStateDescription psd(nullptr,
        CommonStates::Opaque,
        CommonStates::DepthNone,
        CommonStates::CullNone,
        rtState,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

    psd.CreatePipelineState(
        device,
        mRootSignature,
        vertexShader,
        pixelShaders[shaderIndex],
        mPipelineState.GetAddressOf());

    memcpy(constants.colorRotation, c_from709to2020, sizeof(c_from709to2020));

    SetDebugObjectName(mPipelineState.Get(), L"ToneMapPostProcess");
}


// Sets our state onto the D3D device.
void ToneMapPostProcess::Impl::Process(_In_ ID3D12GraphicsCommandList* commandList)
{
    // Set the root signature.
    commandList->SetGraphicsRootSignature(mRootSignature);

    // Set the texture.
    if (!texture.ptr)
    {
        DebugTrace("ERROR: Missing texture for ToneMapPostProcess (texture %llu)\n", texture.ptr);
        throw std::runtime_error("ToneMapPostProcess");
    }
    commandList->SetGraphicsRootDescriptorTable(RootParameterIndex::TextureSRV, texture);

    // Set constants.
    if (mDirtyFlags & Dirty_Parameters)
    {
        mDirtyFlags &= ~Dirty_Parameters;
        mDirtyFlags |= Dirty_ConstantBuffer;

        constants.parameters = XMVectorSet(linearExposure, paperWhiteNits, 0.f, 0.f);
    }

    if (mDirtyFlags & Dirty_ConstantBuffer)
    {
        mDirtyFlags &= ~Dirty_ConstantBuffer;
        mConstantBuffer = GraphicsMemory::Get(mDeviceResources->GetDevice()).AllocateConstant(constants);
    }

    commandList->SetGraphicsRootConstantBufferView(RootParameterIndex::ConstantBuffer, mConstantBuffer.GpuAddress());

    // Set the pipeline state.
    commandList->SetPipelineState(mPipelineState.Get());

    // Draw quad.
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->DrawInstanced(3, 1, 0, 0);
}


// Public constructor.
#if (defined(_XBOX_ONE) && defined(_TITLE)) || defined(_GAMING_XBOX)
ToneMapPostProcess::ToneMapPostProcess(_In_ ID3D12Device* device, const RenderTargetState& rtState, Operator op, TransferFunction func, bool mrt)
  : pImpl(std::make_unique<Impl>(device, rtState, op, func, mrt))
#else
ToneMapPostProcess::ToneMapPostProcess(_In_ ID3D12Device* device, const RenderTargetState& rtState, Operator op, TransferFunction func)
    : pImpl(std::make_unique<Impl>(device, rtState, op, func))
#endif
{
}


ToneMapPostProcess::ToneMapPostProcess(ToneMapPostProcess&&) noexcept = default;
ToneMapPostProcess& ToneMapPostProcess::operator= (ToneMapPostProcess&&) noexcept = default;
ToneMapPostProcess::~ToneMapPostProcess() = default;


// IPostProcess methods.
void ToneMapPostProcess::Process(_In_ ID3D12GraphicsCommandList* commandList)
{
    pImpl->Process(commandList);
}


// Properties
void ToneMapPostProcess::SetHDRSourceTexture(D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor)
{
    pImpl->texture = srvDescriptor;
}


void ToneMapPostProcess::SetColorRotation(ColorPrimaryRotation value)
{
    switch (value)
    {
    case DCI_P3_D65_to_UHDTV:   memcpy(pImpl->constants.colorRotation, c_fromP3D65to2020, sizeof(c_fromP3D65to2020)); break;
    case HDTV_to_DCI_P3_D65:    memcpy(pImpl->constants.colorRotation, c_from709toP3D65, sizeof(c_from709toP3D65)); break;
    default:                    memcpy(pImpl->constants.colorRotation, c_from709to2020, sizeof(c_from709to2020)); break;
    }

    pImpl->SetDirtyFlag();
}


void ToneMapPostProcess::SetColorRotation(CXMMATRIX value)
{
    XMMATRIX transpose = XMMatrixTranspose(value);
    pImpl->constants.colorRotation[0] = transpose.r[0];
    pImpl->constants.colorRotation[1] = transpose.r[1];
    pImpl->constants.colorRotation[2] = transpose.r[2];
    pImpl->SetDirtyFlag();
}


void ToneMapPostProcess::SetExposure(float exposureValue)
{
    pImpl->linearExposure = powf(2.f, exposureValue);
    pImpl->SetDirtyFlag();
}


void ToneMapPostProcess::SetST2084Parameter(float paperWhiteNits)
{
    pImpl->paperWhiteNits = paperWhiteNits;
    pImpl->SetDirtyFlag();
}
