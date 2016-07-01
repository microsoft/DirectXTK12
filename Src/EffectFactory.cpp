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

#include <mutex>


using namespace DirectX;
using Microsoft::WRL::ComPtr;

// Internal EffectFactory implementation class. Only one of these helpers is allocated
// per D3D device, even if there are multiple public facing EffectFactory instances.
class EffectFactory::Impl
{
public:
    Impl(_In_ ID3D12Device* device, _In_ ID3D12DescriptorHeap* heap)
        : device(device)
        , mDescriptors(heap)
        , mSharing(true)
    { 
    }

    std::shared_ptr<IEffect> CreateEffect(
        _In_ const EffectInfo& info, 
        _In_ const EffectPipelineStateDescription& pipelineState,
        _In_ const D3D12_INPUT_LAYOUT_DESC& inputLayout, 
        _In_opt_ int baseDescriptorOffset);

    void ReleaseCache();
    void SetSharing( bool enabled ) { mSharing = enabled; }

    ComPtr<ID3D12DescriptorHeap> mDescriptors;

private:
    ID3D12Device*                  device;

    typedef std::map< std::wstring, std::shared_ptr<IEffect> > EffectCache;

    EffectCache  mEffectCache;
    EffectCache  mEffectCacheSkinning;
    EffectCache  mEffectCacheDualTexture;

    bool mSharing;

    std::mutex mutex;
};


_Use_decl_annotations_
std::shared_ptr<IEffect> EffectFactory::Impl::CreateEffect(
    const EffectInfo& info, 
    const EffectPipelineStateDescription& pipelineState,
    const D3D12_INPUT_LAYOUT_DESC& inputLayoutDesc, 
    int baseDescriptorOffset)
{
    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
    CD3DX12_GPU_DESCRIPTOR_HANDLE textureDescriptorHeapGpuHandle = {};
    CD3DX12_GPU_DESCRIPTOR_HANDLE endDescriptor = {};
    int textureDescriptorHeapIncrement = 0;

    // If we have descriptors, get some information about that
    if (mDescriptors != nullptr)
    {
        descriptorHeapDesc = mDescriptors->GetDesc();

        // Get the texture offsets and descriptor handles
        textureDescriptorHeapIncrement = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        textureDescriptorHeapGpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
            mDescriptors->GetGPUDescriptorHandleForHeapStart(),
            baseDescriptorOffset, 
            textureDescriptorHeapIncrement);

        // For validation, get the last descriptor
        endDescriptor = CD3DX12_GPU_DESCRIPTOR_HANDLE(
            mDescriptors->GetGPUDescriptorHandleForHeapStart(), 
            descriptorHeapDesc.NumDescriptors, 
            textureDescriptorHeapIncrement);
    }

    auto checkDescriptor = [endDescriptor] (D3D12_GPU_DESCRIPTOR_HANDLE handle)
    {
        if (handle.ptr >= endDescriptor.ptr)
        {
            throw std::exception("Out of descriptor heap space.");
        }
    };

    // Modify base pipeline state
    EffectPipelineStateDescription derivedPSD = pipelineState;

    // Input layout
    derivedPSD.inputLayout = &inputLayoutDesc;

    // Alpha state modification
    if ( info.alphaValue < 1.0f )
    {
        derivedPSD.depthStencilDesc = &CommonStates::DepthRead;

        if ( info.isPremultipliedAlpha )
        {
            derivedPSD.blendDesc = &CommonStates::AlphaBlend;
        }
        else
        {
            derivedPSD.blendDesc = &CommonStates::NonPremultiplied;
        }
    }

    if ( info.enableSkinning )
    {
        // SkinnedEffect
        if ( mSharing && !info.name.empty() )
        {
            auto it = mEffectCacheSkinning.find( info.name );
            if ( mSharing && it != mEffectCacheSkinning.end() )
            {
                return it->second;
            }
        }

        std::shared_ptr<SkinnedEffect> effect = std::make_shared<SkinnedEffect>( device, EffectFlags::None, derivedPSD );

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

        if ( mDescriptors != nullptr && info.textureIndex != -1 )
        {
            CD3DX12_GPU_DESCRIPTOR_HANDLE handle(
                textureDescriptorHeapGpuHandle,
                textureDescriptorHeapIncrement * info.textureIndex);

            checkDescriptor(handle);
            effect->SetTexture(handle);
        }

        if ( mSharing && !info.name.empty() )
        {
            std::lock_guard<std::mutex> lock(mutex);
            mEffectCacheSkinning.insert( EffectCache::value_type( info.name, effect ) );
        }

        return effect;
    }
    else if ( info.enableDualTexture )
    {
        // DualTextureEffect
        if ( mSharing && !info.name.empty() )
        {
            auto it = mEffectCacheDualTexture.find( info.name );
            if ( mSharing && it != mEffectCacheDualTexture.end() )
            {
                return it->second;
            }
        }

        // set effect flags for creation
        int flags = EffectFlags::Lighting;

        if (info.perVertexColor)
        {
            flags |= EffectFlags::VertexColor;
        }

        std::shared_ptr<DualTextureEffect> effect = std::make_shared<DualTextureEffect>(device, flags, derivedPSD );

        // Dual texture effect doesn't support lighting (usually it's lightmaps)
        effect->SetAlpha( info.alphaValue );

        XMVECTOR color = XMLoadFloat3( &info.diffuseColor );
        effect->SetDiffuseColor( color );

        if ( mDescriptors != nullptr && info.textureIndex != -1 )
        {
            CD3DX12_GPU_DESCRIPTOR_HANDLE handle(
                textureDescriptorHeapGpuHandle,
                textureDescriptorHeapIncrement * info.textureIndex);

            checkDescriptor(handle);
            effect->SetTexture(handle);
        }

        if ( mDescriptors != nullptr && info.textureIndex2 != -1 )
        {
            CD3DX12_GPU_DESCRIPTOR_HANDLE handle(
                textureDescriptorHeapGpuHandle,
                textureDescriptorHeapIncrement * info.textureIndex2);

            checkDescriptor(handle);
            effect->SetTexture2(handle);
        }

        if ( mSharing && !info.name.empty() )
        {
            std::lock_guard<std::mutex> lock(mutex);
            mEffectCacheDualTexture.insert( EffectCache::value_type( info.name, effect ) );
        }

        return effect;
    }
    else
    {
        // BasicEffect
        if ( mSharing && !info.name.empty() )
        {
            auto it = mEffectCache.find( info.name );
            if ( mSharing && it != mEffectCache.end() )
            {
                return it->second;
            }
        }
        
        // set effect flags for creation
        int flags = EffectFlags::Lighting;

        if (info.perVertexColor)
        {
            flags |= EffectFlags::VertexColor;
        }
        
        if (info.textureIndex != -1)
        {
            flags |= EffectFlags::Texture;
        }

        std::shared_ptr<BasicEffect> effect = std::make_shared<BasicEffect>( device, flags, derivedPSD );

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

        if ( mDescriptors != nullptr && info.textureIndex != -1 )
        {
            CD3DX12_GPU_DESCRIPTOR_HANDLE handle(
                textureDescriptorHeapGpuHandle,
                textureDescriptorHeapIncrement * info.textureIndex);

            checkDescriptor(handle);
            effect->SetTexture(handle);
        }

        if ( mSharing && !info.name.empty() )
        {
            std::lock_guard<std::mutex> lock(mutex);
            mEffectCache.insert( EffectCache::value_type( info.name, effect ) );
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
    pImpl = std::make_shared<Impl>(device, nullptr);
}

EffectFactory::EffectFactory(_In_ ID3D12DescriptorHeap* descriptors)
{
    if (descriptors == nullptr)
    {
        throw std::exception("Descriptor heap cannot be null of no device is provided. Use the alternative EffectFactory constructor instead.");
    }

    if (descriptors->GetDesc().Type != D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    {
        throw std::exception("EffectFactory::CreateEffect requires a CBV_SRV_UAV descriptor heap as input.");
    }

    ComPtr<ID3D12Device> device;
#if defined(_XBOX_ONE) && defined(_TITLE)
    descriptors->GetDevice(IID_GRAPHICS_PPV_ARGS(device.GetAddressOf()));
#else
    HRESULT hresult = descriptors->GetDevice(IID_GRAPHICS_PPV_ARGS(device.GetAddressOf()));
    if (FAILED(hresult))
    {
        throw com_exception(hresult);
    }
#endif

    pImpl = std::make_shared<Impl>(device.Get(), descriptors);
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

_Use_decl_annotations_
std::shared_ptr<IEffect> EffectFactory::CreateEffect(
    _In_ const EffectInfo& info, 
    _In_ const EffectPipelineStateDescription& pipelineState,
    _In_ const D3D12_INPUT_LAYOUT_DESC& inputLayout, 
    _In_opt_ int descriptorOffset)
{
    return pImpl->CreateEffect(info, pipelineState, inputLayout, descriptorOffset);
}

void EffectFactory::ReleaseCache()
{
    pImpl->ReleaseCache();
}

void EffectFactory::SetSharing( bool enabled )
{
    pImpl->SetSharing( enabled );
}
