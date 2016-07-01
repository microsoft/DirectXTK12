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
// http://go.microsoft.com/fwlink/?LinkID=615561
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
void ModelMeshPart::Draw(_In_ ID3D12GraphicsCommandList* commandList) const
{
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

_Use_decl_annotations_
void ModelMeshPart::DrawMeshParts(ID3D12GraphicsCommandList* commandList, const ModelMeshPart::Collection& meshParts)
{
    for ( auto it = meshParts.cbegin(); it != meshParts.cend(); ++it )
    {
        auto part = (*it).get();
        assert( part != 0 );

        part->Draw(commandList);
    }
}


_Use_decl_annotations_
void ModelMeshPart::DrawMeshParts(ID3D12GraphicsCommandList* commandList, const ModelMeshPart::Collection& meshParts, ModelMeshPart::DrawCallback callback)
{
    for ( auto it = meshParts.cbegin(); it != meshParts.cend(); ++it )
    {
        auto part = (*it).get();
        assert( part != 0 );

        callback(commandList, *part);
        part->Draw(commandList);
    }
}


_Use_decl_annotations_
void ModelMeshPart::DrawMeshParts(ID3D12GraphicsCommandList* commandList, const ModelMeshPart::Collection& meshParts, IEffect* effect)
{
    effect->Apply(commandList);
    DrawMeshParts(commandList, meshParts);
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

// Draw the mesh
void __cdecl ModelMesh::DrawOpaque(_In_ ID3D12GraphicsCommandList* commandList) const
{
    ModelMeshPart::DrawMeshParts(commandList, opaqueMeshParts);
}
void __cdecl ModelMesh::DrawAlpha(_In_ ID3D12GraphicsCommandList* commandList) const
{
    ModelMeshPart::DrawMeshParts(commandList, alphaMeshParts);
}

// Draw the mesh with an effect
void __cdecl ModelMesh::DrawOpaque(_In_ ID3D12GraphicsCommandList* commandList, _In_ IEffect* effect) const
{
    ModelMeshPart::DrawMeshParts(commandList, opaqueMeshParts, effect);
}
void __cdecl ModelMesh::DrawAlpha(_In_ ID3D12GraphicsCommandList* commandList, _In_ IEffect* effect) const
{
    ModelMeshPart::DrawMeshParts(commandList, alphaMeshParts, effect);
}

// Draw the mesh with a callback for each mesh part
void __cdecl ModelMesh::DrawOpaque(_In_ ID3D12GraphicsCommandList* commandList, ModelMeshPart::DrawCallback callback) const
{
    ModelMeshPart::DrawMeshParts(commandList, opaqueMeshParts, callback);
}
void __cdecl ModelMesh::DrawAlpha(_In_ ID3D12GraphicsCommandList* commandList, ModelMeshPart::DrawCallback callback) const
{
    ModelMeshPart::DrawMeshParts(commandList, alphaMeshParts, callback);
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


// Load texture resources
void Model::LoadTextures(_In_ IEffectTextureFactory& texFactory, _In_opt_ int destinationDescriptorOffset)
{
    for (size_t i = 0; i < textureNames.size(); ++i)
    {
        texFactory.CreateTexture(textureNames[i].c_str(), destinationDescriptorOffset + (int) i);
    }
}


// Load texture resources (helper function)
std::unique_ptr<EffectTextureFactory> Model::LoadTextures(_In_ ID3D12Device* device, _Inout_ ResourceUploadBatch& resourceUploadBatch, _In_opt_z_ const wchar_t* texturesPath, _In_opt_ D3D12_DESCRIPTOR_HEAP_FLAGS flags)
{
    if (textureNames.size() == 0)
        return nullptr;

    std::unique_ptr<EffectTextureFactory> texFactory = std::make_unique<EffectTextureFactory>(
        device, 
        resourceUploadBatch, 
        textureNames.size(),
        flags);
    if (texturesPath != nullptr && *texturesPath != 0)
    {
        texFactory->SetDirectory(texturesPath);
    }

    LoadTextures(*texFactory);

    return std::move(texFactory);
}


// Create effects for each mesh piece
std::vector<std::shared_ptr<IEffect>> Model::CreateEffects(
    _In_ IEffectFactory& fxFactory, 
    _In_ const EffectPipelineStateDescription& pipelineState,
    _In_opt_ int descriptorOffset)
{
    std::vector<std::shared_ptr<IEffect>> effects;
    effects.reserve(materials.size());

    for (const auto& mesh : meshes)
    {
        assert(mesh != nullptr);

        for (const auto& part : mesh->opaqueMeshParts)
        {
            assert(part != nullptr);

            if (part->materialIndex == ~0ull)
                continue;

            effects.push_back(CreateEffectForMeshPart(fxFactory, pipelineState, descriptorOffset, part.get()));
        }

        for (const auto& part : mesh->alphaMeshParts)
        {
            assert(part != nullptr);

            if (part->materialIndex == ~0ull)
                continue;

            effects.push_back(CreateEffectForMeshPart(fxFactory, pipelineState, descriptorOffset, part.get()));
        }
    }

    return std::move(effects);
}

// Creates an effect for a mesh part
std::shared_ptr<IEffect> Model::CreateEffectForMeshPart(
    _In_ IEffectFactory& fxFactory, 
    _In_ const EffectPipelineStateDescription& pipelineState,
    _In_opt_ int descriptorOffset,
    _In_ const ModelMeshPart* part) const
{
    assert(part->materialIndex < materials.size());

    const auto& m = materials[part->materialIndex];

    D3D12_INPUT_LAYOUT_DESC il = {};
    il.NumElements = (uint32_t) part->vbDecl->size();
    il.pInputElementDescs = part->vbDecl->data();

    return fxFactory.CreateEffect(m, pipelineState, il, descriptorOffset);
}

// Create effects for each mesh piece with the default factory
std::vector<std::shared_ptr<IEffect>> Model::CreateEffects(
    _In_ const EffectPipelineStateDescription& pipelineState,
    _In_ ID3D12DescriptorHeap* gpuVisibleTextureDescriptorHeap, 
    _In_opt_ int descriptorOffset)
{
    EffectFactory fxFactory(gpuVisibleTextureDescriptorHeap);
    return CreateEffects(fxFactory, pipelineState, descriptorOffset);
}

// Updates effect matrices (if applicable)
void XM_CALLCONV Model::UpdateEffectMatrices(
    _In_ std::vector<std::shared_ptr<IEffect>>& effectList,
    DirectX::FXMMATRIX world,
    DirectX::CXMMATRIX view,
    DirectX::CXMMATRIX proj)
{
    for (auto& fx : effectList)
    {
        IEffectMatrices* matFx = dynamic_cast<IEffectMatrices*>(fx.get());
        if (matFx != nullptr)
        {
            matFx->SetWorld(world);
            matFx->SetView(view);
            matFx->SetProjection(proj);
        }
    }
}
