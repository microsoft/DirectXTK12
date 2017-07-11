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

ModelMeshPart::ModelMeshPart(uint32_t partIndex) :
    partIndex(partIndex),
    materialIndex(0),
    indexCount(0),
    startIndex(0),
    vertexOffset(0),
    vertexStride(0),
    vertexCount(0),
    primitiveType(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST),
    indexFormat(DXGI_FORMAT_R16_UINT)
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
    vbv.SizeInBytes = static_cast<UINT>(vertexBuffer.Size());
    commandList->IASetVertexBuffers(0, 1, &vbv);

    D3D12_INDEX_BUFFER_VIEW ibv;
    ibv.BufferLocation = indexBuffer.GpuAddress();
    ibv.SizeInBytes = static_cast<UINT>(indexBuffer.Size());
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
void ModelMeshPart::DrawMeshParts(
    ID3D12GraphicsCommandList* commandList,
    const ModelMeshPart::Collection& meshParts,
    ModelMeshPart::DrawCallback callback)
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
void ModelMeshPart::DrawMeshParts(ID3D12GraphicsCommandList* commandList,
    const ModelMeshPart::Collection& meshParts,
    IEffect* effect)
{
    effect->Apply(commandList);
    DrawMeshParts(commandList, meshParts);
}


//--------------------------------------------------------------------------------------
// ModelMesh
//--------------------------------------------------------------------------------------

ModelMesh::ModelMesh()
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
int Model::LoadTextures(IEffectTextureFactory& texFactory, int destinationDescriptorOffset) const
{
    for (size_t i = 0; i < textureNames.size(); ++i)
    {
        texFactory.CreateTexture(textureNames[i].c_str(), destinationDescriptorOffset + static_cast<int>(i));
    }

    return static_cast<int>(textureNames.size());
}


// Load texture resources (helper function)
_Use_decl_annotations_
std::unique_ptr<EffectTextureFactory> Model::LoadTextures(
    ID3D12Device* device,
    ResourceUploadBatch& resourceUploadBatch,
    const wchar_t* texturesPath,
    D3D12_DESCRIPTOR_HEAP_FLAGS flags) const
{
    if (textureNames.empty())
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

    return texFactory;
}


// Create effects for each mesh piece
std::vector<std::shared_ptr<IEffect>> Model::CreateEffects(
    IEffectFactory& fxFactory, 
    const EffectPipelineStateDescription& opaquePipelineState,
    const EffectPipelineStateDescription& alphaPipelineState, 
    int textureDescriptorOffset,
    int samplerDescriptorOffset) const
{
    if (materials.empty())
    {
        DebugTrace("ERROR: Model has no material information to create effects!\n");
        throw std::exception("CreateEffects");
    }

    std::vector<std::shared_ptr<IEffect>> effects;

    // Count the number of parts
    uint32_t partCount = 0;
    for (const auto& mesh : meshes)
    {
        for (const auto& part : mesh->opaqueMeshParts)
            partCount = std::max(part->partIndex + 1, partCount);
        for (const auto& part : mesh->alphaMeshParts)
            partCount = std::max(part->partIndex + 1, partCount);
    }

    if (partCount == 0)
        return effects;

    // Create an array of effects for each part. We need to have an effect per part because the part's vertex layout
    // combines with the material spec to create a unique effect. We rely on the EffectFactory to de-duplicate if it
    // wants to.
    effects.resize(partCount);

    for (const auto& mesh : meshes)
    {
        assert(mesh != nullptr);

        for (const auto& part : mesh->opaqueMeshParts)
        {
            assert(part != nullptr);

            if (part->materialIndex == uint32_t(-1))
                continue;

            // If this fires, you have multiple parts with the same unique ID
            assert(effects[part->partIndex] == nullptr);

            effects[part->partIndex] = CreateEffectForMeshPart(fxFactory, opaquePipelineState, alphaPipelineState, textureDescriptorOffset, samplerDescriptorOffset, part.get());
        }

        for (const auto& part : mesh->alphaMeshParts)
        {
            assert(part != nullptr);

            if (part->materialIndex == uint32_t(-1))
                continue;

            // If this fires, you have multiple parts with the same unique ID
            assert(effects[part->partIndex] == nullptr);

            effects[part->partIndex] = CreateEffectForMeshPart(fxFactory, opaquePipelineState, alphaPipelineState, textureDescriptorOffset, samplerDescriptorOffset, part.get());
        }
    }

    return effects;
}

// Creates an effect for a mesh part
_Use_decl_annotations_
std::shared_ptr<IEffect> Model::CreateEffectForMeshPart(
    IEffectFactory& fxFactory, 
    const EffectPipelineStateDescription& opaquePipelineState,
    const EffectPipelineStateDescription& alphaPipelineState, 
    int textureDescriptorOffset,
    int samplerDescriptorOffset,
    const ModelMeshPart* part) const
{
    assert(part->materialIndex < materials.size());

    const auto& m = materials[part->materialIndex];

    D3D12_INPUT_LAYOUT_DESC il = {};
    il.NumElements = (uint32_t) part->vbDecl->size();
    il.pInputElementDescs = part->vbDecl->data();

    return fxFactory.CreateEffect(m, opaquePipelineState, alphaPipelineState, il, textureDescriptorOffset, samplerDescriptorOffset);
}

// Create effects for each mesh piece with the default factory
_Use_decl_annotations_
std::vector<std::shared_ptr<IEffect>> Model::CreateEffects(
    const EffectPipelineStateDescription& opaquePipelineState,
    const EffectPipelineStateDescription& alphaPipelineState,
    ID3D12DescriptorHeap* textureDescriptorHeap, 
    ID3D12DescriptorHeap* samplerDescriptorHeap,
    int textureDescriptorOffset,
    int samplerDescriptorOffset) const
{
    EffectFactory fxFactory(textureDescriptorHeap, samplerDescriptorHeap);
    return CreateEffects(fxFactory, opaquePipelineState, alphaPipelineState, textureDescriptorOffset, samplerDescriptorOffset);
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
        if (matFx)
        {
            matFx->SetMatrices(world, view, proj);
        }
    }
}
