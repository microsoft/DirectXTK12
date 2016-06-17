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
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Effects.h"
#include "DemandCreate.h"
#include "SharedResourcePool.h"
#include "CommonStates.h"
#include "DirectXHelpers.h"
#include "DDSTextureLoader.h"
#include "DescriptorHeap.h"
#include "ResourceUploadBatch.h"
#include "WICTextureLoader.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;


// Internal EffectFactory implementation class. Only one of these helpers is allocated
// per D3D device, even if there are multiple public facing EffectFactory instances.
class EffectFactory::Impl
{
public:
    Impl(_In_ ID3D12Device*                          device,
         _In_ ID3D12CommandQueue*                    uploadCommandQueue,
         _In_ const EffectPipelineStateDescription&  basePipelineState,
         _Inout_ ID3D12DescriptorHeap*               textureDescriptorHeap,
         _In_ size_t                                 textureDescriptorHeapStartIndex,
        _Inout_  ResourceUploadBatch&                resourceUploadBatch)
        : device(device)
        , mUploadCommandQueue(uploadCommandQueue)
        , mBasePipelineStateDesc(basePipelineState)
        , mTextureDescriptorHeap(textureDescriptorHeap)
        , mTextureDescriptorHeapSlot((int)textureDescriptorHeapStartIndex)
        , mTextureDescriptorHeapIncrement(device->GetDescriptorHandleIncrementSize(textureDescriptorHeap->GetDesc().Type))
        , mTextureDescriptorHeapGpuHandle(textureDescriptorHeap->GetGPUDescriptorHandleForHeapStart())
        , mResourceUploadBatch(resourceUploadBatch)
        , mSharing(true)
    { 
        *mPath = 0; 
    }

    std::shared_ptr<IEffect> CreateEffect(_In_ const IEffectFactory::EffectInfo&   info,
                                          _In_ const D3D12_INPUT_LAYOUT_DESC&      inputLayoutDesc);

    int CreateTexture(_In_z_ const wchar_t* name);

    void ReleaseCache();
    void SetSharing( bool enabled ) { mSharing = enabled; }

    static SharedResourcePool<ID3D12Device*, Impl> instancePool;

    wchar_t mPath[MAX_PATH];

private:
    ID3D12Device*                  device;
    ID3D12CommandQueue*            mUploadCommandQueue;
    EffectPipelineStateDescription mBasePipelineStateDesc;
    ID3D12DescriptorHeap*          mTextureDescriptorHeap;
    D3D12_GPU_DESCRIPTOR_HANDLE    mTextureDescriptorHeapGpuHandle;
    int                            mTextureDescriptorHeapIncrement;
    int                            mTextureDescriptorHeapSlot;
    ResourceUploadBatch&           mResourceUploadBatch;

    struct TextureCacheEntry
    {
        ComPtr<ID3D12Resource> mResource;
        int                    mDescriptorHeapIndex;
    };

    typedef std::map< std::wstring, std::shared_ptr<IEffect> > EffectCache;
    typedef std::map< std::wstring, TextureCacheEntry > TextureCache;

    EffectCache  mEffectCache;
    EffectCache  mEffectCacheSkinning;
    EffectCache  mEffectCacheDualTexture;
    TextureCache mTextureCache;

    bool mSharing;

    std::mutex mutex;
};


// Global instance pool.
SharedResourcePool<ID3D12Device*, EffectFactory::Impl> EffectFactory::Impl::instancePool;


_Use_decl_annotations_
std::shared_ptr<IEffect> EffectFactory::Impl::CreateEffect(_In_ const IEffectFactory::EffectInfo&   info,
                                                           _In_ const D3D12_INPUT_LAYOUT_DESC&      inputLayoutDesc)
{
    // Modify base pipeline state
    EffectPipelineStateDescription derivedPSD = mBasePipelineStateDesc;
    
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
        if ( mSharing && info.name && *info.name )
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

        if ( info.texture && *info.texture )
        {
            int newDescriptorSlot = CreateTexture(info.texture);

            CD3DX12_GPU_DESCRIPTOR_HANDLE handle(
                mTextureDescriptorHeapGpuHandle,
                mTextureDescriptorHeapIncrement * newDescriptorSlot);

            effect->SetTexture(handle);
        }

        if ( mSharing && info.name && *info.name )
        {
            std::lock_guard<std::mutex> lock(mutex);
            mEffectCacheSkinning.insert( EffectCache::value_type( info.name, effect ) );
        }

        return effect;
    }
    else if ( info.enableDualTexture )
    {
        // DualTextureEffect
        if ( mSharing && info.name && *info.name )
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

        if ( info.texture && *info.texture )
        {
            int newDescriptorSlot = CreateTexture(info.texture);
            effect->SetTexture(CD3DX12_GPU_DESCRIPTOR_HANDLE(
                mTextureDescriptorHeapGpuHandle,
                mTextureDescriptorHeapIncrement * newDescriptorSlot));
        }

        if ( info.texture2 && *info.texture2 )
        {
            int newDescriptorSlot = CreateTexture(info.texture2);
            effect->SetTexture2(CD3DX12_GPU_DESCRIPTOR_HANDLE(
                mTextureDescriptorHeapGpuHandle,
                mTextureDescriptorHeapIncrement * newDescriptorSlot));
        }

        if ( mSharing && info.name && *info.name )
        {
            std::lock_guard<std::mutex> lock(mutex);
            mEffectCacheDualTexture.insert( EffectCache::value_type( info.name, effect ) );
        }

        return effect;
    }
    else
    {
        // BasicEffect
        if ( mSharing && info.name && *info.name )
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
        
        if (info.texture && *info.texture)
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

        if ( info.texture && *info.texture )
        {
            int newDescriptorSlot = CreateTexture( info.texture );
            effect->SetTexture(CD3DX12_GPU_DESCRIPTOR_HANDLE(
                mTextureDescriptorHeapGpuHandle,
                mTextureDescriptorHeapIncrement * newDescriptorSlot));
        }

        if ( mSharing && info.name && *info.name )
        {
            std::lock_guard<std::mutex> lock(mutex);
            mEffectCache.insert( EffectCache::value_type( info.name, effect ) );
        }

        return effect;
    }
}

_Use_decl_annotations_
int EffectFactory::Impl::CreateTexture(_In_z_  const wchar_t* name)
{
   if ( !name )
        throw std::exception("invalid arguments");

    int newDescriptorSlot = -1;

    auto it = mTextureCache.find( name );

    if ( mSharing && it != mTextureCache.end() )
    {
        newDescriptorSlot = it->second.mDescriptorHeapIndex;
    }
    else
    {
        ComPtr<ID3D12Resource> newTextureResource;

        wchar_t fullName[MAX_PATH] = {};
        wcscpy_s( fullName, mPath );
        wcscat_s( fullName, name );

        wchar_t ext[_MAX_EXT];
        _wsplitpath_s( name, nullptr, 0, nullptr, 0, nullptr, 0, ext, _MAX_EXT );

        if ( _wcsicmp( ext, L".dds" ) == 0 )
        {
            // load resource
            HRESULT hr = CreateDDSTextureFromFile( device, &mResourceUploadBatch, fullName, newTextureResource.ReleaseAndGetAddressOf());
            if ( FAILED(hr) )
            {
                DebugTrace( "CreateDDSTextureFromFile failed (%08X) for '%ls'\n", hr, fullName );
                throw std::exception( "CreateDDSTextureFromFile" );
            }

            // bind a new descriptor in slot 
            CD3DX12_CPU_DESCRIPTOR_HANDLE newTextureDescriptor(
                mTextureDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
                mTextureDescriptorHeapIncrement * mTextureDescriptorHeapSlot);
            DirectX::CreateShaderResourceView(device, newTextureResource.Get(), newTextureDescriptor, false);

            // return slot index, then increment
            newDescriptorSlot = mTextureDescriptorHeapSlot++;
        }
        else
        {
            std::lock_guard<std::mutex> lock(mutex);
            HRESULT hr = CreateWICTextureFromFile( device, &mResourceUploadBatch, fullName, newTextureResource.ReleaseAndGetAddressOf() );
            if ( FAILED(hr) )
            {
                DebugTrace( "CreateWICTextureFromFile failed (%08X) for '%ls'\n", hr, fullName );
                throw std::exception( "CreateWICTextureFromFile" );
            }
        }

        if ( mSharing )
        {   
            std::lock_guard<std::mutex> lock(mutex);
            TextureCacheEntry newEntry = { newTextureResource, newDescriptorSlot };
            mTextureCache.insert( TextureCache::value_type( name, newEntry ) );
        }
    }

    return newDescriptorSlot;
}

void EffectFactory::Impl::ReleaseCache()
{
    std::lock_guard<std::mutex> lock(mutex);
    mEffectCache.clear();
    mEffectCacheSkinning.clear();
    mEffectCacheDualTexture.clear();
    mTextureCache.clear();
}



//--------------------------------------------------------------------------------------
// EffectFactory
//--------------------------------------------------------------------------------------

EffectFactory::EffectFactory(_In_ ID3D12Device*                          device,
                             _In_ ID3D12CommandQueue*                    uploadCommandQueue,
                             _In_ const EffectPipelineStateDescription&  basePipelineState,
                             _Inout_ ID3D12DescriptorHeap*               textureDescriptorHeap,
                             _In_    size_t                              textureDescriptorHeapStartIndex,
                            _Inout_  ResourceUploadBatch&                resourceUploadBatch)
{
    pImpl = std::make_shared<Impl>(device, uploadCommandQueue, basePipelineState, textureDescriptorHeap, textureDescriptorHeapStartIndex, resourceUploadBatch);
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
std::shared_ptr<IEffect> EffectFactory::CreateEffect(_In_ const EffectInfo& info,
                                                     _In_ const D3D12_INPUT_LAYOUT_DESC& inputLayout)
{
    return pImpl->CreateEffect(info, inputLayout );
}

_Use_decl_annotations_
size_t EffectFactory::CreateTexture(_In_z_  const wchar_t* name)
{
    return pImpl->CreateTexture(name);
}

void EffectFactory::ReleaseCache()
{
    pImpl->ReleaseCache();
}

void EffectFactory::SetSharing( bool enabled )
{
    pImpl->SetSharing( enabled );
}

void EffectFactory::SetDirectory( _In_opt_z_ const wchar_t* path )
{
    if ( path && *path != 0 )
    {
        wcscpy_s( pImpl->mPath, path );
        size_t len = wcsnlen( pImpl->mPath, MAX_PATH );
        if ( len > 0 && len < (MAX_PATH-1) )
        {
            // Ensure it has a trailing slash
            if ( pImpl->mPath[len-1] != L'\\' )
            {
                pImpl->mPath[len] = L'\\';
                pImpl->mPath[len+1] = 0;
            }
        }
    }
    else
        *pImpl->mPath = 0;
}