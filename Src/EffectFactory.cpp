//--------------------------------------------------------------------------------------
// File: EffectFactory.cpp
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
#include "Effects.h"
#include "CommonStates.h"
#include "DirectXHelpers.h"
#include "PlatformHelpers.h"
#include "DescriptorHeap.h"

#include <mutex>


using namespace DirectX;
using Microsoft::WRL::ComPtr;

// Internal EffectFactory implementation class. Only one of these helpers is allocated
// per D3D device, even if there are multiple public facing EffectFactory instances.
class EffectFactory::Impl
{
public:
    Impl(_In_ ID3D12Device* device, _In_ ID3D12DescriptorHeap* textureDescriptors, _In_ ID3D12DescriptorHeap* samplerDescriptors)
        : device(device)
        , mTextureDescriptors(nullptr)
        , mSamplerDescriptors(nullptr)
        , mEnablePerPixelLighting(false)
        , mEnableFog(false)
        , mSharing(true)
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
    void SetSharing( bool enabled ) { mSharing = enabled; }

    std::unique_ptr<DescriptorHeap> mTextureDescriptors;
    std::unique_ptr<DescriptorHeap> mSamplerDescriptors;

    bool mEnablePerPixelLighting;
    bool mEnableFog;

private:
    ID3D12Device*                  device;

    typedef std::map< std::wstring, std::shared_ptr<IEffect> > EffectCache;

    EffectCache  mEffectCache;
    EffectCache  mEffectCacheSkinning;
    EffectCache  mEffectCacheDualTexture;

    bool mSharing;

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
    if (mTextureDescriptors == nullptr && (info.textureIndex != -1 || info.textureIndex2 != -1))
    {
        DebugTrace("ERROR: EffectFactory created without texture descriptor heap with texture index set (textureIndex %d, textureIndex2 %d)!\n", info.textureIndex, info.textureIndex2);
        throw std::exception("EffectFactory");
    }
    if (mSamplerDescriptors == nullptr && (info.samplerIndex != -1 || info.samplerIndex2 != -1))
    {
        DebugTrace("ERROR: EffectFactory created without sampler descriptor heap with sampler index set (samplerIndex %d, samplerIndex2 %d)!\n", info.samplerIndex, info.samplerIndex2);
        throw std::exception("EffectFactory");
    }

    // If we have descriptors, make sure we have both texture and sampler descriptors
    if ((mTextureDescriptors == nullptr) != (mSamplerDescriptors == nullptr))
    {
        throw std::exception("A texture or sampler descriptor heap was provided, but both are required.");
    }

    // Validate the we have either both texture and sampler descriptors, or neither
    if ((info.textureIndex == -1) != (info.samplerIndex == -1))
    {
        throw std::exception("Material provides either a texture or sampler, but both are required.");
    }
    if ((info.textureIndex2 == -1) != (info.samplerIndex2 == -1))
    {
        throw std::exception("Material provides either a secondary texture or sampler, but both are required.");
    }

    int textureIndex = (info.textureIndex != -1 && mTextureDescriptors != nullptr) ? info.textureIndex + textureDescriptorOffset : -1;
    int textureIndex2 = (info.textureIndex2 != -1 && mTextureDescriptors != nullptr) ? info.textureIndex2 + textureDescriptorOffset : -1;
    int samplerIndex = (info.samplerIndex != -1 && mSamplerDescriptors != nullptr) ? info.samplerIndex + samplerDescriptorOffset : -1;
    int samplerIndex2 = (info.samplerIndex2 != -1 && mSamplerDescriptors != nullptr) ? info.samplerIndex2 + samplerDescriptorOffset : -1;

    // Modify base pipeline state
    EffectPipelineStateDescription derivedPSD = (info.alphaValue < 1.0f) ? alphaPipelineState : opaquePipelineState;
    derivedPSD.inputLayout = inputLayoutDesc;

    std::wstring cacheName;
    if ( info.enableSkinning )
    {
        // SkinnedEffect
        int effectflags = (mEnablePerPixelLighting) ? EffectFlags::PerPixelLighting : EffectFlags::Lighting;

        if (mEnableFog)
        {
            effectflags |= EffectFlags::Fog;
        }

        if ( mSharing && !info.name.empty() )
        {
            uint32_t hash = derivedPSD.ComputeHash();
            cacheName = std::to_wstring(effectflags) + info.name + std::to_wstring(hash);

            auto it = mEffectCacheSkinning.find( cacheName );
            if ( mSharing && it != mEffectCacheSkinning.end() )
            {
                return it->second;
            }
        }

        std::shared_ptr<SkinnedEffect> effect = std::make_shared<SkinnedEffect>( device, effectflags, derivedPSD );

        effect->EnableDefaultLighting();

        effect->SetAlpha( info.alphaValue );

        // Skinned Effect does not have an ambient material color, or per-vertex color support

        XMVECTOR color = XMLoadFloat3( &info.diffuseColor );
        effect->SetDiffuseColor( color );

        if ( info.specularColor.x != 0 || info.specularColor.y != 0 || info.specularColor.z != 0 )
        {
            color = XMLoadFloat3( &info.specularColor );
            effect->SetSpecularColor( color );
            effect->SetSpecularPower( info.specularPower );
        }
        else
        {
            effect->DisableSpecular();
        }

        if ( info.emissiveColor.x != 0 || info.emissiveColor.y != 0 || info.emissiveColor.z != 0 )
        {
            color = XMLoadFloat3( &info.emissiveColor );
            effect->SetEmissiveColor( color );
        }

        if ( textureIndex != -1 )
        {
            effect->SetTexture(
                mTextureDescriptors->GetGpuHandle(textureIndex),
                mSamplerDescriptors->GetGpuHandle(samplerIndex));
        }

        if ( mSharing && !info.name.empty() )
        {
            std::lock_guard<std::mutex> lock(mutex);
            mEffectCacheSkinning.insert( EffectCache::value_type( cacheName, effect ) );
        }

        return effect;
    }
    else if ( info.enableDualTexture )
    {
        // DualTextureEffect
        int effectflags = EffectFlags::None;

        if (mEnableFog)
        {
            effectflags |= EffectFlags::Fog;
        }

        if ( mSharing && !info.name.empty() )
        {
            uint32_t hash = derivedPSD.ComputeHash();
            cacheName = std::to_wstring(effectflags) + info.name + std::to_wstring(hash);

            auto it = mEffectCacheDualTexture.find( cacheName );
            if ( mSharing && it != mEffectCacheDualTexture.end() )
            {
                return it->second;
            }
        }

        if (info.perVertexColor)
        {
            effectflags |= EffectFlags::VertexColor;
        }

        std::shared_ptr<DualTextureEffect> effect = std::make_shared<DualTextureEffect>(device, effectflags, derivedPSD );

        // Dual texture effect doesn't support lighting (usually it's lightmaps)
        effect->SetAlpha( info.alphaValue );

        XMVECTOR color = XMLoadFloat3( &info.diffuseColor );
        effect->SetDiffuseColor( color );

        if ( textureIndex != -1 )
        {
            effect->SetTexture(
                mTextureDescriptors->GetGpuHandle(textureIndex),
                mSamplerDescriptors->GetGpuHandle(samplerIndex));
        }

        if ( textureIndex2 != -1 )
        {
            effect->SetTexture2(
                mTextureDescriptors->GetGpuHandle(textureIndex2),
                mSamplerDescriptors->GetGpuHandle(samplerIndex2));
        }

        if ( mSharing && !info.name.empty() )
        {
            std::lock_guard<std::mutex> lock(mutex);
            mEffectCacheDualTexture.insert( EffectCache::value_type( cacheName, effect ) );
        }

        return effect;
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

        if (info.textureIndex != -1)
        {
            effectflags |= EffectFlags::Texture;
        }

        // BasicEffect
        if ( mSharing && !info.name.empty() )
        {
            uint32_t hash = derivedPSD.ComputeHash();
            cacheName = std::to_wstring(effectflags) + info.name + std::to_wstring(hash);

            auto it = mEffectCache.find( cacheName );
            if ( mSharing && it != mEffectCache.end() )
            {
                return it->second;
            }
        }

        std::shared_ptr<BasicEffect> effect = std::make_shared<BasicEffect>( device, effectflags, derivedPSD );

        effect->EnableDefaultLighting();
        
        effect->SetAlpha( info.alphaValue );

        // Basic Effect does not have an ambient material color
        XMVECTOR color = XMLoadFloat3( &info.diffuseColor );
        effect->SetDiffuseColor( color );

        if ( info.specularColor.x != 0 || info.specularColor.y != 0 || info.specularColor.z != 0 )
        {
            color = XMLoadFloat3( &info.specularColor );
            effect->SetSpecularColor( color );
            effect->SetSpecularPower( info.specularPower );
        }
        else
        {
            effect->DisableSpecular();
        }

        if ( info.emissiveColor.x != 0 || info.emissiveColor.y != 0 || info.emissiveColor.z != 0 )
        {
            color = XMLoadFloat3( &info.emissiveColor );
            effect->SetEmissiveColor( color );
        }

        if ( textureIndex != -1 )
        {
            effect->SetTexture(
                mTextureDescriptors->GetGpuHandle(textureIndex),
                mSamplerDescriptors->GetGpuHandle(samplerIndex));
        }

        if ( mSharing && !info.name.empty() )
        {
            std::lock_guard<std::mutex> lock(mutex);
            mEffectCache.insert( EffectCache::value_type( cacheName, effect ) );
        }

        return effect;
    }
}

void EffectFactory::Impl::ReleaseCache()
{
    std::lock_guard<std::mutex> lock(mutex);
    mEffectCache.clear();
    mEffectCacheSkinning.clear();
    mEffectCacheDualTexture.clear();
}



//--------------------------------------------------------------------------------------
// EffectFactory
//--------------------------------------------------------------------------------------

EffectFactory::EffectFactory(_In_ ID3D12Device* device)
{
    pImpl = std::make_shared<Impl>(device, nullptr, nullptr);
}

EffectFactory::EffectFactory(_In_ ID3D12DescriptorHeap* textureDescriptors, _In_ ID3D12DescriptorHeap* samplerDescriptors)
{
    if (textureDescriptors == nullptr)
    {
        throw std::exception("Texture descriptor heap cannot be null if no device is provided. Use the alternative EffectFactory constructor instead.");
    }
    if (samplerDescriptors == nullptr)
    {
        throw std::exception("Descriptor heap cannot be null if no device is provided. Use the alternative EffectFactory constructor instead.");
    }

    if (textureDescriptors->GetDesc().Type != D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    {
        throw std::exception("EffectFactory::CreateEffect requires a CBV_SRV_UAV descriptor heap for textureDescriptors.");
    }
    if (samplerDescriptors->GetDesc().Type != D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
    {
        throw std::exception("EffectFactory::CreateEffect requires a SAMPLER descriptor heap for samplerDescriptors.");
    }

    ComPtr<ID3D12Device> device;
#if defined(_XBOX_ONE) && defined(_TITLE)
    textureDescriptors->GetDevice(IID_GRAPHICS_PPV_ARGS(device.GetAddressOf()));
#else
    HRESULT hresult = textureDescriptors->GetDevice(IID_GRAPHICS_PPV_ARGS(device.GetAddressOf()));
    if (FAILED(hresult))
    {
        throw com_exception(hresult);
    }
#endif

    pImpl = std::make_shared<Impl>(device.Get(), textureDescriptors, samplerDescriptors);
}

EffectFactory::~EffectFactory()
{
}


EffectFactory::EffectFactory(EffectFactory&& moveFrom)
    : pImpl(std::move(moveFrom.pImpl))
{
}

EffectFactory& EffectFactory::operator= (EffectFactory&& moveFrom)
{
    pImpl = std::move(moveFrom.pImpl);
    return *this;
}

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

void EffectFactory::SetSharing( bool enabled )
{
    pImpl->SetSharing( enabled );
}

void EffectFactory::EnablePerPixelLighting(bool enabled)
{
    pImpl->mEnablePerPixelLighting = enabled;
}

void EffectFactory::EnableFogging(bool enabled)
{
    pImpl->mEnableFog = enabled;
}

