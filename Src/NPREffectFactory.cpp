//--------------------------------------------------------------------------------------
// File: NPREffectFactory.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// https://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Effects.h"
#include "CommonStates.h"
#include "DirectXHelpers.h"
#include "PlatformHelpers.h"
#include "DescriptorHeap.h"

#include <mutex>


using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace
{
    template<typename T>
    void SetMaterialProperties(_In_ T* effect, const EffectFactory::EffectInfo& info)
    {
        effect->EnableDefaultLighting();

        effect->SetAlpha(info.alphaValue);

        // Most DirectX Tool Kit effects do not have an ambient material color.

        XMVECTOR color = XMLoadFloat3(&info.diffuseColor);
        effect->SetDiffuseColor(color);

        if (info.specularColor.x != 0 || info.specularColor.y != 0 || info.specularColor.z != 0)
        {
            color = XMLoadFloat3(&info.specularColor);
            effect->SetSpecularColor(color);
        }
        else
        {
            effect->DisableSpecular();
        }
    }
}

// Internal NPREffectFactory implementation class. Only one of these helpers is allocated
// per D3D device, even if there are multiple public facing NPREffectFactory instances.
class NPREffectFactory::Impl
{
public:
    Impl(_In_ ID3D12Device* device, _In_ ID3D12DescriptorHeap* textureDescriptors, _In_ ID3D12DescriptorHeap* samplerDescriptors) noexcept(false)
        : mTextureDescriptors(nullptr)
        , mSamplerDescriptors(nullptr)
        , mMode(NPREffect::Mode_Cel)
        , mSharing(true)
        , mEnableInstancing(false)
        , mUseEmissiveForMatCap(true)
        , mMatcapTextureIndex(-1)
        , mMatcapSamplerIndex(-1)
        , mDevice(device)
    {
        if (!device)
            throw std::invalid_argument("Direct3D device is null");

        if (textureDescriptors)
            mTextureDescriptors = std::make_unique<DescriptorHeap>(textureDescriptors);
        if (samplerDescriptors)
            mSamplerDescriptors = std::make_unique<DescriptorHeap>(samplerDescriptors);
    }

    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;

    Impl(Impl&&) = delete;
    Impl& operator=(Impl&&) = delete;

    std::shared_ptr<IEffect> CreateEffect(
        const EffectInfo& info,
        const EffectPipelineStateDescription& opaquePipelineState,
        const EffectPipelineStateDescription& alphaPipelineState,
        const D3D12_INPUT_LAYOUT_DESC& inputLayout,
        int textureDescriptorOffset,
        int samplerDescriptorOffset);

    void ReleaseCache();

    std::unique_ptr<DescriptorHeap> mTextureDescriptors;
    std::unique_ptr<DescriptorHeap> mSamplerDescriptors;

    NPREffect::Mode mMode;
    bool mSharing;
    bool mEnableInstancing;
    bool mUseEmissiveForMatCap;
    int mMatcapTextureIndex;
    int mMatcapSamplerIndex;

    ComPtr<ID3D12Device> mDevice;

private:
    using EffectCache = std::map< std::wstring, std::shared_ptr<IEffect> >;

    EffectCache  mEffectCache;
    EffectCache  mEffectCacheSkinning;
    EffectCache  mEffectCacheDualTexture;

    std::mutex mutex;
};


std::shared_ptr<IEffect> NPREffectFactory::Impl::CreateEffect(
    const EffectInfo& info,
    const EffectPipelineStateDescription& opaquePipelineState,
    const EffectPipelineStateDescription& alphaPipelineState,
    const D3D12_INPUT_LAYOUT_DESC& inputLayoutDesc,
    int textureDescriptorOffset,
    int samplerDescriptorOffset)
{
    // If textures are required, make sure we have a descriptor heap
    if (!mTextureDescriptors && (info.diffuseTextureIndex != -1 || info.specularTextureIndex != -1 || info.emissiveTextureIndex != -1))
    {
        DebugTrace("ERROR: NPREffectFactory created without texture descriptor heap with texture index set (diffuse %d, specular %d, emissive %d)!\n",
            info.diffuseTextureIndex, info.specularTextureIndex, info.emissiveTextureIndex);
        throw std::runtime_error("NPREffectFactory");
    }
    if (!mSamplerDescriptors && (info.samplerIndex != -1 || info.samplerIndex2 != -1))
    {
        DebugTrace("ERROR: NPREffectFactory created without sampler descriptor heap with sampler index set (samplerIndex %d, samplerIndex2 %d)!\n",
            info.samplerIndex, info.samplerIndex2);
        throw std::runtime_error("NPREffectFactory");
    }

    // If we have descriptors, make sure we have both texture and sampler descriptors
    if ((mTextureDescriptors == nullptr) != (mSamplerDescriptors == nullptr))
    {
        DebugTrace("ERROR: A texture or sampler descriptor heap was provided, but both are required.\n");
        throw std::runtime_error("NPREffectFactory");
    }

    // Validate the we have either both texture and sampler descriptors, or neither
    if ((info.diffuseTextureIndex == -1) != (info.samplerIndex == -1))
    {
        DebugTrace("ERROR: Material provides either a texture or sampler, but both are required.\n");
        throw std::runtime_error("NPREffectFactory");
    }

    const int diffuseTextureIndex = (info.diffuseTextureIndex != -1 && mTextureDescriptors != nullptr) ? info.diffuseTextureIndex + textureDescriptorOffset : -1;
    const int specularTextureIndex = (info.specularTextureIndex != -1 && mTextureDescriptors != nullptr) ? info.specularTextureIndex + textureDescriptorOffset : -1;
    const int emissiveTextureIndex = (info.emissiveTextureIndex != -1 && mTextureDescriptors != nullptr) ? info.emissiveTextureIndex + textureDescriptorOffset : -1;
    const int samplerIndex = (info.samplerIndex != -1 && mSamplerDescriptors != nullptr) ? info.samplerIndex + samplerDescriptorOffset : -1;
    const int samplerIndex2 = (info.samplerIndex2 != -1 && mSamplerDescriptors != nullptr) ? info.samplerIndex2 + samplerDescriptorOffset : -1;

    // Modify base pipeline state
    EffectPipelineStateDescription derivedPSD = (info.alphaValue < 1.0f) ? alphaPipelineState : opaquePipelineState;
    derivedPSD.inputLayout = inputLayoutDesc;

    std::wstring cacheName;
    if (info.enableSkinning)
    {
        int effectflags = EffectFlags::None;

        if (info.biasedVertexNormals)
        {
            effectflags |= EffectFlags::BiasedVertexNormals;
        }

        // SkinnedNPREffect
        if (mSharing && !info.name.empty())
        {
            const uint32_t hash = derivedPSD.ComputeHash();
            cacheName = std::to_wstring(effectflags) + std::to_wstring(mMode) + info.name + std::to_wstring(hash);

            auto it = mEffectCacheSkinning.find(cacheName);
            if (mSharing && it != mEffectCacheSkinning.end())
            {
                return it->second;
            }
        }

        auto effect = std::make_shared<SkinnedNPREffect>(mDevice.Get(), effectflags, derivedPSD, mMode);

        SetMaterialProperties(effect.get(), info);

        if (diffuseTextureIndex != -1)
        {
            effect->SetTexture(
                mTextureDescriptors->GetGpuHandle(static_cast<size_t>(diffuseTextureIndex)),
                mSamplerDescriptors->GetGpuHandle(static_cast<size_t>(samplerIndex)));
        }

        if (mMode == NPREffect::Mode_MatCap)
        {
            if (mUseEmissiveForMatCap && emissiveTextureIndex != -1)
            {
                if (samplerIndex == -1)
                {
                    DebugTrace("ERROR: SkinnedNPREffect (matcap) requires a sampler (emissive %d)\n", emissiveTextureIndex);
                    throw std::runtime_error("NPREffectFactory");
                }

                effect->SetMatCap(
                    mTextureDescriptors->GetGpuHandle(static_cast<size_t>(emissiveTextureIndex)),
                    mSamplerDescriptors->GetGpuHandle(static_cast<size_t>(samplerIndex)));
            }
            else if (mMatcapTextureIndex != -1)
            {
                auto matcap = mTextureDescriptors->GetGpuHandle(static_cast<size_t>(mMatcapTextureIndex));
                if (samplerIndex != -1)
                {
                    effect->SetMatCap(matcap,
                        mSamplerDescriptors->GetGpuHandle(static_cast<size_t>(samplerIndex)));
                }
                else if (mMatcapSamplerIndex != -1)
                {
                    effect->SetMatCap(matcap,
                        mSamplerDescriptors->GetGpuHandle(static_cast<size_t>(mMatcapSamplerIndex)));
                }
            }
        }

        if (mSharing && !info.name.empty())
        {
            std::lock_guard<std::mutex> lock(mutex);
            EffectCache::value_type v(cacheName, effect);
            mEffectCacheSkinning.insert(v);
        }

        return std::move(effect);
    }
    else if (info.enableDualTexture)
    {
        // DualTextureEffect
        int effectflags = EffectFlags::None;

        if (mSharing && !info.name.empty())
        {
            const uint32_t hash = derivedPSD.ComputeHash();
            cacheName = std::to_wstring(effectflags) + info.name + std::to_wstring(hash);

            auto it = mEffectCacheDualTexture.find(cacheName);
            if (mSharing && it != mEffectCacheDualTexture.end())
            {
                return it->second;
            }
        }

        if (info.perVertexColor)
        {
            effectflags |= EffectFlags::VertexColor;
        }

        auto effect = std::make_shared<DualTextureEffect>(mDevice.Get(), effectflags, derivedPSD);

        // Dual texture effect doesn't support lighting (usually it's lightmaps)
        effect->SetAlpha(info.alphaValue);

        const XMVECTOR color = XMLoadFloat3(&info.diffuseColor);
        effect->SetDiffuseColor(color);

        if (diffuseTextureIndex != -1)
        {
            effect->SetTexture(
                mTextureDescriptors->GetGpuHandle(static_cast<size_t>(diffuseTextureIndex)),
                mSamplerDescriptors->GetGpuHandle(static_cast<size_t>(samplerIndex)));
        }

        if (emissiveTextureIndex != -1)
        {
            if (samplerIndex2 == -1)
            {
                DebugTrace("ERROR: Dual-texture requires a second sampler (emissive %d)\n", emissiveTextureIndex);
                throw std::runtime_error("NPREffectFactory");
            }

            effect->SetTexture2(
                mTextureDescriptors->GetGpuHandle(static_cast<size_t>(emissiveTextureIndex)),
                mSamplerDescriptors->GetGpuHandle(static_cast<size_t>(samplerIndex2)));
        }
        else if (specularTextureIndex != -1)
        {
            // If there's no emissive texture specified, use the specular texture as the second texture
            if (samplerIndex2 == -1)
            {
                DebugTrace("ERROR: Dual-texture requires a second sampler (specular %d)\n", specularTextureIndex);
                throw std::runtime_error("NPREffectFactory");
            }

            effect->SetTexture2(
                mTextureDescriptors->GetGpuHandle(static_cast<size_t>(specularTextureIndex)),
                mSamplerDescriptors->GetGpuHandle(static_cast<size_t>(samplerIndex2)));
        }

        if (mSharing && !info.name.empty())
        {
            std::lock_guard<std::mutex> lock(mutex);
            EffectCache::value_type v(cacheName, effect);
            mEffectCacheDualTexture.insert(v);
        }

        return std::move(effect);
    }
    else
    {
        // set effect flags for creation
        int effectflags = EffectFlags::None;

        if (info.perVertexColor)
        {
            effectflags |= EffectFlags::VertexColor;
        }

        if (diffuseTextureIndex != -1)
        {
            effectflags |= EffectFlags::Texture;
        }

        if (info.biasedVertexNormals)
        {
            effectflags |= EffectFlags::BiasedVertexNormals;
        }

        // NPREffect
        if (mSharing && !info.name.empty())
        {
            const uint32_t hash = derivedPSD.ComputeHash();
            cacheName = std::to_wstring(effectflags) + std::to_wstring(mMode) + info.name + std::to_wstring(hash);

            auto it = mEffectCache.find(cacheName);
            if (mSharing && it != mEffectCache.end())
            {
                return it->second;
            }
        }

        auto effect = std::make_shared<NPREffect>(mDevice.Get(), effectflags, derivedPSD, mMode);

        SetMaterialProperties(effect.get(), info);

        if (diffuseTextureIndex != -1)
        {
            effect->SetTexture(
                mTextureDescriptors->GetGpuHandle(static_cast<size_t>(diffuseTextureIndex)),
                mSamplerDescriptors->GetGpuHandle(static_cast<size_t>(samplerIndex)));
        }

        if (mMode == NPREffect::Mode_MatCap)
        {
            if (mUseEmissiveForMatCap && emissiveTextureIndex != -1)
            {
                if (samplerIndex == -1)
                {
                    DebugTrace("ERROR: NPREffect (matcap) requires a sampler (emissive %d)\n", emissiveTextureIndex);
                    throw std::runtime_error("NPREffectFactory");
                }

                effect->SetMatCap(
                    mTextureDescriptors->GetGpuHandle(static_cast<size_t>(emissiveTextureIndex)),
                    mSamplerDescriptors->GetGpuHandle(static_cast<size_t>(samplerIndex)));
            }
            else if (mMatcapTextureIndex != -1)
            {
                auto matcap = mTextureDescriptors->GetGpuHandle(static_cast<size_t>(mMatcapTextureIndex));
                if (samplerIndex != -1)
                {
                    effect->SetMatCap(matcap,
                        mSamplerDescriptors->GetGpuHandle(static_cast<size_t>(samplerIndex)));
                }
                else if (mMatcapSamplerIndex != -1)
                {
                    effect->SetMatCap(matcap,
                        mSamplerDescriptors->GetGpuHandle(static_cast<size_t>(mMatcapSamplerIndex)));
                }
            }
        }

        if (mSharing && !info.name.empty())
        {
            std::lock_guard<std::mutex> lock(mutex);
            EffectCache::value_type v(cacheName, effect);
            mEffectCache.insert(v);
        }

        return std::move(effect);
    }
}

void NPREffectFactory::Impl::ReleaseCache()
{
    std::lock_guard<std::mutex> lock(mutex);
    mEffectCache.clear();
    mEffectCacheSkinning.clear();
    mEffectCacheDualTexture.clear();
}



//--------------------------------------------------------------------------------------
// NPREffectFactory
//--------------------------------------------------------------------------------------

NPREffectFactory::NPREffectFactory(_In_ ID3D12Device* device) :
    pImpl(std::make_shared<Impl>(device, nullptr, nullptr))
{}

NPREffectFactory::NPREffectFactory(_In_ ID3D12DescriptorHeap* textureDescriptors, _In_ ID3D12DescriptorHeap* samplerDescriptors)
{
    if (!textureDescriptors)
    {
        throw std::invalid_argument("Texture descriptor heap cannot be null if no device is provided. Use the alternative NPREffectFactory constructor instead.");
    }
    if (!samplerDescriptors)
    {
        throw std::invalid_argument("Descriptor heap cannot be null if no device is provided. Use the alternative NPREffectFactory constructor instead.");
    }

#if defined(_MSC_VER) || !defined(_WIN32)
    const D3D12_DESCRIPTOR_HEAP_TYPE textureHeapType = textureDescriptors->GetDesc().Type;
    const D3D12_DESCRIPTOR_HEAP_TYPE samplerHeapType = samplerDescriptors->GetDesc().Type;
#else
    D3D12_DESCRIPTOR_HEAP_DESC tmpDesc1, tmpDesc2;
    const D3D12_DESCRIPTOR_HEAP_TYPE textureHeapType = textureDescriptors->GetDesc(&tmpDesc1)->Type;
    const D3D12_DESCRIPTOR_HEAP_TYPE samplerHeapType = samplerDescriptors->GetDesc(&tmpDesc2)->Type;
#endif

    if (textureHeapType != D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    {
        throw std::invalid_argument("NPREffectFactory::CreateEffect requires a CBV_SRV_UAV descriptor heap for textureDescriptors.");
    }
    if (samplerHeapType != D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
    {
        throw std::invalid_argument("NPREffectFactory::CreateEffect requires a SAMPLER descriptor heap for samplerDescriptors.");
    }

    ComPtr<ID3D12Device> device;
#if (defined(_XBOX_ONE) && defined(_TITLE)) || defined(_GAMING_XBOX)
    textureDescriptors->GetDevice(IID_GRAPHICS_PPV_ARGS(device.GetAddressOf()));
#else
    HRESULT hr = textureDescriptors->GetDevice(IID_PPV_ARGS(device.GetAddressOf()));
    ThrowIfFailed(hr);
#endif

    pImpl = std::make_shared<Impl>(device.Get(), textureDescriptors, samplerDescriptors);
}


NPREffectFactory::NPREffectFactory(NPREffectFactory&&) noexcept = default;
NPREffectFactory& NPREffectFactory::operator= (NPREffectFactory&&) noexcept = default;
NPREffectFactory::~NPREffectFactory() = default;


std::shared_ptr<IEffect> NPREffectFactory::CreateEffect(
    const EffectInfo& info,
    const EffectPipelineStateDescription& opaquePipelineState,
    const EffectPipelineStateDescription& alphaPipelineState,
    const D3D12_INPUT_LAYOUT_DESC& inputLayout,
    int textureDescriptorOffset,
    int samplerDescriptorOffset)
{
    return pImpl->CreateEffect(info, opaquePipelineState, alphaPipelineState, inputLayout, textureDescriptorOffset, samplerDescriptorOffset);
}


void NPREffectFactory::ReleaseCache()
{
    pImpl->ReleaseCache();
}


// Properties.
void NPREffectFactory::SetSharing(bool enabled) noexcept
{
    pImpl->mSharing = enabled;
}

void NPREffectFactory::EnableInstancing(bool enabled) noexcept
{
    pImpl->mEnableInstancing = enabled;
}

void NPREffectFactory::SetMode(NPREffect::Mode mode) noexcept
{
    pImpl->mMode = mode;
}

void NPREffectFactory::SetDefaultMatCap(int textureIndex, int samplerIndex) noexcept
{
    pImpl->mMatcapTextureIndex = textureIndex;
    pImpl->mMatcapSamplerIndex = samplerIndex;
}

void NPREffectFactory::SetEmissiveAsMatCap(bool value) noexcept
{
    pImpl->mUseEmissiveForMatCap = value;
}

ID3D12Device* NPREffectFactory::GetDevice() const noexcept
{
    return pImpl->mDevice.Get();
}
