//--------------------------------------------------------------------------------------
// File: EffectFactory.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=615561
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
            effect->SetSpecularPower(info.specularPower);
        }
        else
        {
            effect->DisableSpecular();
        }

        if (info.emissiveColor.x != 0 || info.emissiveColor.y != 0 || info.emissiveColor.z != 0)
        {
            color = XMLoadFloat3(&info.emissiveColor);
            effect->SetEmissiveColor(color);
        }
    }
}

// Internal EffectFactory implementation class. Only one of these helpers is allocated
// per D3D device, even if there are multiple public facing EffectFactory instances.
class EffectFactory::Impl
{
public:
    Impl(_In_ ID3D12Device* device, _In_ ID3D12DescriptorHeap* textureDescriptors, _In_ ID3D12DescriptorHeap* samplerDescriptors) noexcept(false)
        : mTextureDescriptors(nullptr)
        , mSamplerDescriptors(nullptr)
        , mSharing(true)
        , mUseNormalMapEffect(true)
        , mEnablePerPixelLighting(true)
        , mEnableFog(false)
        , mEnableInstancing(false)
        , mDevice(device)
    {
        if (textureDescriptors)
            mTextureDescriptors = std::make_unique<DescriptorHeap>(textureDescriptors);
        if (samplerDescriptors)
            mSamplerDescriptors = std::make_unique<DescriptorHeap>(samplerDescriptors);
    }

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

    bool mSharing;
    bool mUseNormalMapEffect;
    bool mEnablePerPixelLighting;
    bool mEnableFog;
    bool mEnableInstancing;

private:
    ComPtr<ID3D12Device> mDevice;

    using EffectCache = std::map< std::wstring, std::shared_ptr<IEffect> >;

    EffectCache  mEffectCache;
    EffectCache  mEffectCacheSkinning;
    EffectCache  mEffectCacheDualTexture;
    EffectCache  mEffectCacheNormalMap;
    EffectCache  mEffectCacheNormalMapSkinned;

    std::mutex mutex;
};


std::shared_ptr<IEffect> EffectFactory::Impl::CreateEffect(
    const EffectInfo& info,
    const EffectPipelineStateDescription& opaquePipelineState,
    const EffectPipelineStateDescription& alphaPipelineState,
    const D3D12_INPUT_LAYOUT_DESC& inputLayoutDesc,
    int textureDescriptorOffset,
    int samplerDescriptorOffset)
{
    // If textures are required, make sure we have a descriptor heap
    if (!mTextureDescriptors && (info.diffuseTextureIndex != -1 || info.specularTextureIndex != -1 || info.normalTextureIndex != -1 || info.emissiveTextureIndex != -1))
    {
        DebugTrace("ERROR: EffectFactory created without texture descriptor heap with texture index set (diffuse %d, specular %d, normal %d, emissive %d)!\n",
            info.diffuseTextureIndex, info.specularTextureIndex, info.normalTextureIndex, info.emissiveTextureIndex);
        throw std::runtime_error("EffectFactory");
    }
    if (!mSamplerDescriptors && (info.samplerIndex != -1 || info.samplerIndex2 != -1))
    {
        DebugTrace("ERROR: EffectFactory created without sampler descriptor heap with sampler index set (samplerIndex %d, samplerIndex2 %d)!\n",
            info.samplerIndex, info.samplerIndex2);
        throw std::runtime_error("EffectFactory");
    }

    // If we have descriptors, make sure we have both texture and sampler descriptors
    if ((mTextureDescriptors == nullptr) != (mSamplerDescriptors == nullptr))
    {
        DebugTrace("ERROR: A texture or sampler descriptor heap was provided, but both are required.\n");
        throw std::runtime_error("EffectFactory");
    }

    // Validate the we have either both texture and sampler descriptors, or neither
    if ((info.diffuseTextureIndex == -1) != (info.samplerIndex == -1))
    {
        DebugTrace("ERROR: Material provides either a texture or sampler, but both are required.\n");
        throw std::runtime_error("EffectFactory");
    }

    int diffuseTextureIndex = (info.diffuseTextureIndex != -1 && mTextureDescriptors != nullptr) ? info.diffuseTextureIndex + textureDescriptorOffset : -1;
    int specularTextureIndex = (info.specularTextureIndex != -1 && mTextureDescriptors != nullptr) ? info.specularTextureIndex + textureDescriptorOffset : -1;
    int emissiveTextureIndex = (info.emissiveTextureIndex != -1 && mTextureDescriptors != nullptr) ? info.emissiveTextureIndex + textureDescriptorOffset : -1;
    int normalTextureIndex = (info.normalTextureIndex != -1 && mTextureDescriptors != nullptr) ? info.normalTextureIndex + textureDescriptorOffset : -1;
    int samplerIndex = (info.samplerIndex != -1 && mSamplerDescriptors != nullptr) ? info.samplerIndex + samplerDescriptorOffset : -1;
    int samplerIndex2 = (info.samplerIndex2 != -1 && mSamplerDescriptors != nullptr) ? info.samplerIndex2 + samplerDescriptorOffset : -1;

    // Modify base pipeline state
    EffectPipelineStateDescription derivedPSD = (info.alphaValue < 1.0f) ? alphaPipelineState : opaquePipelineState;
    derivedPSD.inputLayout = inputLayoutDesc;

    std::wstring cacheName;
    if (info.enableSkinning)
    {
        int effectflags = (mEnablePerPixelLighting) ? EffectFlags::PerPixelLighting : EffectFlags::Lighting;

        if (mEnableFog)
        {
            effectflags |= EffectFlags::Fog;
        }

        if (info.biasedVertexNormals)
        {
            effectflags |= EffectFlags::BiasedVertexNormals;
        }

        if (info.enableNormalMaps && mUseNormalMapEffect)
        {
            // SkinnedNormalMapEffect
            if (specularTextureIndex != -1)
            {
                effectflags |= EffectFlags::Specular;
            }

            if (mSharing && !info.name.empty())
            {
                uint32_t hash = derivedPSD.ComputeHash();
                cacheName = std::to_wstring(effectflags) + info.name + std::to_wstring(hash);

                auto it = mEffectCacheNormalMapSkinned.find(cacheName);
                if (mSharing && it != mEffectCacheNormalMapSkinned.end())
                {
                    return it->second;
                }
            }

            auto effect = std::make_shared<SkinnedNormalMapEffect>(mDevice.Get(), effectflags, derivedPSD);

            SetMaterialProperties(effect.get(), info);

            if (diffuseTextureIndex != -1)
            {
                effect->SetTexture(
                    mTextureDescriptors->GetGpuHandle(static_cast<size_t>(diffuseTextureIndex)),
                    mSamplerDescriptors->GetGpuHandle(static_cast<size_t>(samplerIndex)));
            }

            if (specularTextureIndex != -1)
            {
                effect->SetSpecularTexture(mTextureDescriptors->GetGpuHandle(static_cast<size_t>(specularTextureIndex)));
            }

            if (normalTextureIndex != -1)
            {
                effect->SetNormalTexture(mTextureDescriptors->GetGpuHandle(static_cast<size_t>(normalTextureIndex)));
            }

            if (mSharing && !info.name.empty())
            {
                std::lock_guard<std::mutex> lock(mutex);
                EffectCache::value_type v(cacheName, effect);
                mEffectCacheNormalMapSkinned.insert(v);
            }

            return std::move(effect);
        }
        else
        {
            // SkinnedEffect
            if (mSharing && !info.name.empty())
            {
                uint32_t hash = derivedPSD.ComputeHash();
                cacheName = std::to_wstring(effectflags) + info.name + std::to_wstring(hash);

                auto it = mEffectCacheSkinning.find(cacheName);
                if (mSharing && it != mEffectCacheSkinning.end())
                {
                    return it->second;
                }
            }

            auto effect = std::make_shared<SkinnedEffect>(mDevice.Get(), effectflags, derivedPSD);

            SetMaterialProperties(effect.get(), info);

            if (diffuseTextureIndex != -1)
            {
                effect->SetTexture(
                    mTextureDescriptors->GetGpuHandle(static_cast<size_t>(diffuseTextureIndex)),
                    mSamplerDescriptors->GetGpuHandle(static_cast<size_t>(samplerIndex)));
            }

            if (mSharing && !info.name.empty())
            {
                std::lock_guard<std::mutex> lock(mutex);
                EffectCache::value_type v(cacheName, effect);
                mEffectCacheSkinning.insert(v);
            }

            return std::move(effect);
        }
    }
    else if (info.enableDualTexture)
    {
        // DualTextureEffect
        int effectflags = EffectFlags::None;

        if (mEnableFog)
        {
            effectflags |= EffectFlags::Fog;
        }

        if (mSharing && !info.name.empty())
        {
            uint32_t hash = derivedPSD.ComputeHash();
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

        XMVECTOR color = XMLoadFloat3(&info.diffuseColor);
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
                throw std::runtime_error("EffectFactory");
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
                throw std::runtime_error("EffectFactory");
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
    else if (info.enableNormalMaps && mUseNormalMapEffect)
    {
        // NormalMapEffect
        int effectflags = EffectFlags::None;

        if (mEnableFog)
        {
            effectflags |= EffectFlags::Fog;
        }

        if (mEnableInstancing)
        {
            effectflags |= EffectFlags::Instancing;
        }

        if (info.perVertexColor)
        {
            effectflags |= EffectFlags::VertexColor;
        }

        if (info.biasedVertexNormals)
        {
            effectflags |= EffectFlags::BiasedVertexNormals;
        }

        if (specularTextureIndex != -1)
        {
            effectflags |= EffectFlags::Specular;
        }

        if (mSharing && !info.name.empty())
        {
            uint32_t hash = derivedPSD.ComputeHash();
            cacheName = std::to_wstring(effectflags) + info.name + std::to_wstring(hash);

            auto it = mEffectCacheNormalMap.find(cacheName);
            if (mSharing && it != mEffectCacheNormalMap.end())
            {
                return it->second;
            }
        }

        auto effect = std::make_shared<NormalMapEffect>(mDevice.Get(), effectflags, derivedPSD);

        SetMaterialProperties(effect.get(), info);

        if (diffuseTextureIndex != -1)
        {
            effect->SetTexture(
                mTextureDescriptors->GetGpuHandle(static_cast<size_t>(diffuseTextureIndex)),
                mSamplerDescriptors->GetGpuHandle(static_cast<size_t>(samplerIndex)));
        }

        if (specularTextureIndex != -1)
        {
            effect->SetSpecularTexture(mTextureDescriptors->GetGpuHandle(static_cast<size_t>(specularTextureIndex)));
        }

        if (normalTextureIndex != -1)
        {
            effect->SetNormalTexture(mTextureDescriptors->GetGpuHandle(static_cast<size_t>(normalTextureIndex)));
        }

        if (mSharing && !info.name.empty())
        {
            std::lock_guard<std::mutex> lock(mutex);
            EffectCache::value_type v(cacheName, effect);
            mEffectCacheNormalMap.insert(v);
        }

        return std::move(effect);
    }
    else
    {
        // set effect flags for creation
        int effectflags = (mEnablePerPixelLighting) ? EffectFlags::PerPixelLighting : EffectFlags::Lighting;

        if (mEnableFog)
        {
            effectflags |= EffectFlags::Fog;
        }

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

        // BasicEffect
        if (mSharing && !info.name.empty())
        {
            uint32_t hash = derivedPSD.ComputeHash();
            cacheName = std::to_wstring(effectflags) + info.name + std::to_wstring(hash);

            auto it = mEffectCache.find(cacheName);
            if (mSharing && it != mEffectCache.end())
            {
                return it->second;
            }
        }

        auto effect = std::make_shared<BasicEffect>(mDevice.Get(), effectflags, derivedPSD);

        SetMaterialProperties(effect.get(), info);

        if (diffuseTextureIndex != -1)
        {
            effect->SetTexture(
                mTextureDescriptors->GetGpuHandle(static_cast<size_t>(diffuseTextureIndex)),
                mSamplerDescriptors->GetGpuHandle(static_cast<size_t>(samplerIndex)));
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

void EffectFactory::Impl::ReleaseCache()
{
    std::lock_guard<std::mutex> lock(mutex);
    mEffectCache.clear();
    mEffectCacheSkinning.clear();
    mEffectCacheDualTexture.clear();
    mEffectCacheNormalMap.clear();
    mEffectCacheNormalMapSkinned.clear();
}



//--------------------------------------------------------------------------------------
// EffectFactory
//--------------------------------------------------------------------------------------

EffectFactory::EffectFactory(_In_ ID3D12Device* device) :
    pImpl(std::make_shared<Impl>(device, nullptr, nullptr))
{
}

EffectFactory::EffectFactory(_In_ ID3D12DescriptorHeap* textureDescriptors, _In_ ID3D12DescriptorHeap* samplerDescriptors)
{
    if (!textureDescriptors)
    {
        throw std::invalid_argument("Texture descriptor heap cannot be null if no device is provided. Use the alternative EffectFactory constructor instead.");
    }
    if (!samplerDescriptors)
    {
        throw std::invalid_argument("Descriptor heap cannot be null if no device is provided. Use the alternative EffectFactory constructor instead.");
    }

    if (textureDescriptors->GetDesc().Type != D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    {
        throw std::invalid_argument("EffectFactory::CreateEffect requires a CBV_SRV_UAV descriptor heap for textureDescriptors.");
    }
    if (samplerDescriptors->GetDesc().Type != D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
    {
        throw std::invalid_argument("EffectFactory::CreateEffect requires a SAMPLER descriptor heap for samplerDescriptors.");
    }

    ComPtr<ID3D12Device> device;
#if (defined(_XBOX_ONE) && defined(_TITLE)) || defined(_GAMING_XBOX)
    textureDescriptors->GetDevice(IID_GRAPHICS_PPV_ARGS(device.GetAddressOf()));
#else
    HRESULT hresult = textureDescriptors->GetDevice(IID_PPV_ARGS(device.GetAddressOf()));
    if (FAILED(hresult))
    {
        throw com_exception(hresult);
    }
#endif

    pImpl = std::make_shared<Impl>(device.Get(), textureDescriptors, samplerDescriptors);
}


EffectFactory::EffectFactory(EffectFactory&&) noexcept = default;
EffectFactory& EffectFactory::operator= (EffectFactory&&) noexcept = default;
EffectFactory::~EffectFactory() = default;


std::shared_ptr<IEffect> EffectFactory::CreateEffect(
    const EffectInfo& info,
    const EffectPipelineStateDescription& opaquePipelineState,
    const EffectPipelineStateDescription& alphaPipelineState,
    const D3D12_INPUT_LAYOUT_DESC& inputLayout,
    int textureDescriptorOffset,
    int samplerDescriptorOffset)
{
    return pImpl->CreateEffect(info, opaquePipelineState, alphaPipelineState, inputLayout, textureDescriptorOffset, samplerDescriptorOffset);
}


void EffectFactory::ReleaseCache()
{
    pImpl->ReleaseCache();
}


// Properties.
void EffectFactory::SetSharing(bool enabled) noexcept
{
    pImpl->mSharing = enabled;
}

void EffectFactory::EnablePerPixelLighting(bool enabled) noexcept
{
    pImpl->mEnablePerPixelLighting = enabled;
}

void EffectFactory::EnableFogging(bool enabled) noexcept
{
    pImpl->mEnableFog = enabled;
}

void EffectFactory::EnableInstancing(bool enabled) noexcept
{
    pImpl->mEnableInstancing = enabled;
}

void EffectFactory::EnableNormalMapEffect(bool enabled) noexcept
{
    pImpl->mUseNormalMapEffect = enabled;
}
