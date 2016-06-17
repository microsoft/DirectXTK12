//--------------------------------------------------------------------------------------
// File: Model.cpp
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
#include "Model.h"

#include "CommonStates.h"
#include "DirectXHelpers.h"
#include "Effects.h"
#include "PlatformHelpers.h"
#include "DescriptorHeap.h"

using namespace DirectX;

#ifndef _CPPRTTI 
#error Model requires RTTI
#endif

//--------------------------------------------------------------------------------------
// ModelMeshPart
//--------------------------------------------------------------------------------------

ModelMeshPart::ModelMeshPart() :
    indexCount(0),
    startIndex(0),
    vertexOffset(0),
    vertexStride(0),
    primitiveType(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST),
    indexFormat(DXGI_FORMAT_R16_UINT),
    isAlpha(false)
{
}


ModelMeshPart::~ModelMeshPart()
{
}


_Use_decl_annotations_
void ModelMeshPart::Draw(_In_ ID3D12GraphicsCommandList* commandList, IEffect* ieffect) const
{
    assert( ieffect != 0 );
    ieffect->Apply(commandList);

    D3D12_VERTEX_BUFFER_VIEW vbv;
    vbv.BufferLocation = vertexBuffer.GpuAddress();
    vbv.StrideInBytes = vertexStride;
    vbv.SizeInBytes = (UINT)vertexBuffer.Size();
    commandList->IASetVertexBuffers(0, 1, &vbv);

    D3D12_INDEX_BUFFER_VIEW ibv;
    ibv.BufferLocation = indexBuffer.GpuAddress();
    ibv.SizeInBytes = (UINT)indexBuffer.Size();
    ibv.Format = indexFormat;
    commandList->IASetIndexBuffer(&ibv);

    commandList->IASetPrimitiveTopology(primitiveType);

    commandList->DrawIndexedInstanced( indexCount, 1, startIndex, vertexOffset, 0 );
}

//--------------------------------------------------------------------------------------
// ModelMesh
//--------------------------------------------------------------------------------------

ModelMesh::ModelMesh() :
    ccw(true),
    pmalpha(true)
{
}


ModelMesh::~ModelMesh()
{
}


_Use_decl_annotations_
void XM_CALLCONV ModelMesh::Draw(_In_ ID3D12GraphicsCommandList* commandList,
                                  FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection,
                                  bool alpha ) const
{
    for ( auto it = meshParts.cbegin(); it != meshParts.cend(); ++it )
    {
        auto part = (*it).get();
        assert( part != 0 );

        if ( part->isAlpha != alpha )
        {
            // Skip alpha parts when drawing opaque or skip opaque parts if drawing alpha
            continue;
        }

        auto imatrices = dynamic_cast<IEffectMatrices*>( part->effect.get() );
        if ( imatrices )
        {
            imatrices->SetWorld( world );
            imatrices->SetView( view );
            imatrices->SetProjection( projection );
        }

        part->Draw(commandList, part->effect.get());
    }
}

//--------------------------------------------------------------------------------------
// Model
//--------------------------------------------------------------------------------------
Model::Model()
{
}


Model::~Model()
{
}


_Use_decl_annotations_
void XM_CALLCONV Model::Draw(_In_ ID3D12GraphicsCommandList* commandList,
    FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection) const
{
    // Draw opaque parts
    for( auto it = meshes.cbegin(); it != meshes.cend(); ++it )
    {
        auto mesh = it->get();
        assert( mesh != 0 );
        
        mesh->Draw(commandList, world, view, projection, false);
    }

    // Draw alpha parts
    for( auto it = meshes.cbegin(); it != meshes.cend(); ++it )
    {
        auto mesh = it->get();
        assert( mesh != 0 );

        mesh->Draw(commandList, world, view, projection, true);
    }
}

void Model::UpdateEffects( _In_ std::function<void(IEffect*)> setEffect )
{
    if ( mEffectCache.empty() )
    {
        // This cache ensures we only set each effect once (could be shared)
        for( auto mit  = meshes.cbegin(); mit != meshes.cend(); ++mit )
        {
            auto mesh = mit->get();
            assert( mesh != 0 );

            for ( auto it = mesh->meshParts.cbegin(); it != mesh->meshParts.cend(); ++it )
            {
                if ( (*it)->effect != 0 )
                    mEffectCache.insert( (*it)->effect.get() );
            }
        }
    }

    assert( setEffect != 0 );

    for( auto it = mEffectCache.begin(); it != mEffectCache.end(); ++it )
    {
        setEffect( *it );
    }
}
