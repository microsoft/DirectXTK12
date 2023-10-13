//--------------------------------------------------------------------------------------
// File: Model.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Model.h"

#include "CommonStates.h"
#include "DescriptorHeap.h"
#include "DirectXHelpers.h"
#include "Effects.h"
#include "PlatformHelpers.h"
#include "ResourceUploadBatch.h"

using namespace DirectX;

#if !defined(_CPPRTTI) && !defined(__GXX_RTTI)
#error Model requires RTTI
#endif


//--------------------------------------------------------------------------------------
// ModelMeshPart
//--------------------------------------------------------------------------------------

ModelMeshPart::ModelMeshPart(uint32_t ipartIndex) noexcept :
    partIndex(ipartIndex),
    materialIndex(0),
    indexCount(0),
    startIndex(0),
    vertexOffset(0),
    vertexStride(0),
    vertexCount(0),
    indexBufferSize(0),
    vertexBufferSize(0),
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
    if (!indexBufferSize || !vertexBufferSize)
    {
        DebugTrace("ERROR: Model part missing values for vertex and/or index buffer size (indexBufferSize %u, vertexBufferSize %u)!\n", indexBufferSize, vertexBufferSize);
        throw std::runtime_error("ModelMeshPart");
    }

    if (!staticIndexBuffer && !indexBuffer)
    {
        DebugTrace("ERROR: Model part missing index buffer!\n");
        throw std::runtime_error("ModelMeshPart");
    }

    if (!staticVertexBuffer && !vertexBuffer)
    {
        DebugTrace("ERROR: Model part missing vertex buffer!\n");
        throw std::runtime_error("ModelMeshPart");
    }

    D3D12_VERTEX_BUFFER_VIEW vbv;
    vbv.BufferLocation = staticVertexBuffer ? staticVertexBuffer->GetGPUVirtualAddress() : vertexBuffer.GpuAddress();
    vbv.StrideInBytes = vertexStride;
    vbv.SizeInBytes = vertexBufferSize;
    commandList->IASetVertexBuffers(0, 1, &vbv);

    D3D12_INDEX_BUFFER_VIEW ibv;
    ibv.BufferLocation = staticIndexBuffer ? staticIndexBuffer->GetGPUVirtualAddress() : indexBuffer.GpuAddress();
    ibv.SizeInBytes = indexBufferSize;
    ibv.Format = indexFormat;
    commandList->IASetIndexBuffer(&ibv);

    commandList->IASetPrimitiveTopology(primitiveType);

    commandList->DrawIndexedInstanced(indexCount, 1, startIndex, vertexOffset, 0);
}


_Use_decl_annotations_
void ModelMeshPart::DrawInstanced(
    ID3D12GraphicsCommandList* commandList,
    uint32_t instanceCount,
    uint32_t startInstance) const
{
    if (!indexBufferSize || !vertexBufferSize)
    {
        DebugTrace("ERROR: Model part missing values for vertex and/or index buffer size (indexBufferSize %u, vertexBufferSize %u)!\n", indexBufferSize, vertexBufferSize);
        throw std::runtime_error("ModelMeshPart");
    }

    if (!staticIndexBuffer && !indexBuffer)
    {
        DebugTrace("ERROR: Model part missing index buffer!\n");
        throw std::runtime_error("ModelMeshPart");
    }

    if (!staticVertexBuffer && !vertexBuffer)
    {
        DebugTrace("ERROR: Model part missing vertex buffer!\n");
        throw std::runtime_error("ModelMeshPart");
    }

    D3D12_VERTEX_BUFFER_VIEW vbv;
    vbv.BufferLocation = staticVertexBuffer ? staticVertexBuffer->GetGPUVirtualAddress() : vertexBuffer.GpuAddress();
    vbv.StrideInBytes = vertexStride;
    vbv.SizeInBytes = vertexBufferSize;
    commandList->IASetVertexBuffers(0, 1, &vbv);

    D3D12_INDEX_BUFFER_VIEW ibv;
    ibv.BufferLocation = staticIndexBuffer ? staticIndexBuffer->GetGPUVirtualAddress() : indexBuffer.GpuAddress();
    ibv.SizeInBytes = indexBufferSize;
    ibv.Format = indexFormat;
    commandList->IASetIndexBuffer(&ibv);

    commandList->IASetPrimitiveTopology(primitiveType);

    commandList->DrawIndexedInstanced(indexCount, instanceCount, startIndex, vertexOffset, startInstance);
}


_Use_decl_annotations_
void ModelMeshPart::DrawMeshParts(
    ID3D12GraphicsCommandList* commandList,
    const ModelMeshPart::Collection& meshParts)
{
    for (const auto& it : meshParts)
    {
        auto part = it.get();
        assert(part != nullptr);

        part->Draw(commandList);
    }
}


_Use_decl_annotations_
void ModelMeshPart::DrawMeshParts(
    ID3D12GraphicsCommandList* commandList,
    const ModelMeshPart::Collection& meshParts,
    ModelMeshPart::DrawCallback callback)
{
    for (const auto& it : meshParts)
    {
        auto part = it.get();
        assert(part != nullptr);

        callback(commandList, *part);
        part->Draw(commandList);
    }
}


_Use_decl_annotations_
void ModelMeshPart::DrawMeshParts(
    ID3D12GraphicsCommandList* commandList,
    const ModelMeshPart::Collection& meshParts,
    IEffect* effect)
{
    effect->Apply(commandList);
    DrawMeshParts(commandList, meshParts);
}


//--------------------------------------------------------------------------------------
// ModelMesh
//--------------------------------------------------------------------------------------

ModelMesh::ModelMesh() noexcept :
    boneIndex(ModelBone::c_Invalid)
{
}


ModelMesh::~ModelMesh()
{
}

// Draw the mesh
void ModelMesh::DrawOpaque(_In_ ID3D12GraphicsCommandList* commandList) const
{
    ModelMeshPart::DrawMeshParts(commandList, opaqueMeshParts);
}

void ModelMesh::DrawAlpha(_In_ ID3D12GraphicsCommandList* commandList) const
{
    ModelMeshPart::DrawMeshParts(commandList, alphaMeshParts);
}


// Draw the mesh with an effect
void ModelMesh::DrawOpaque(_In_ ID3D12GraphicsCommandList* commandList, _In_ IEffect* effect) const
{
    ModelMeshPart::DrawMeshParts(commandList, opaqueMeshParts, effect);
}

void ModelMesh::DrawAlpha(_In_ ID3D12GraphicsCommandList* commandList, _In_ IEffect* effect) const
{
    ModelMeshPart::DrawMeshParts(commandList, alphaMeshParts, effect);
}


// Draw the mesh with a callback for each mesh part
void ModelMesh::DrawOpaque(_In_ ID3D12GraphicsCommandList* commandList, ModelMeshPart::DrawCallback callback) const
{
    ModelMeshPart::DrawMeshParts(commandList, opaqueMeshParts, callback);
}

void ModelMesh::DrawAlpha(_In_ ID3D12GraphicsCommandList* commandList, ModelMeshPart::DrawCallback callback) const
{
    ModelMeshPart::DrawMeshParts(commandList, alphaMeshParts, callback);
}


//--------------------------------------------------------------------------------------
// Model
//--------------------------------------------------------------------------------------

Model::Model() noexcept
{
}

Model::~Model()
{
}

Model::Model(Model const& other) :
    meshes(other.meshes),
    materials(other.materials),
    textureNames(other.textureNames),
    bones(other.bones),
    name(other.name)
{
    const size_t nbones = other.bones.size();
    if (nbones > 0)
    {
        if (other.boneMatrices)
        {
            boneMatrices = ModelBone::MakeArray(nbones);
            memcpy(boneMatrices.get(), other.boneMatrices.get(), sizeof(XMMATRIX) * nbones);
        }
        if (other.invBindPoseMatrices)
        {
            invBindPoseMatrices = ModelBone::MakeArray(nbones);
            memcpy(invBindPoseMatrices.get(), other.invBindPoseMatrices.get(), sizeof(XMMATRIX) * nbones);
        }
    }
}

Model& Model::operator= (Model const& rhs)
{
    if (this != &rhs)
    {
        Model tmp(rhs);
        std::swap(meshes, tmp.meshes);
        std::swap(materials, tmp.materials);
        std::swap(textureNames, tmp.textureNames);
        std::swap(bones, tmp.bones);
        std::swap(boneMatrices, tmp.boneMatrices);
        std::swap(invBindPoseMatrices, tmp.invBindPoseMatrices);
        std::swap(name, tmp.name);
    }
    return *this;
}


// Load texture resources.
int Model::LoadTextures(IEffectTextureFactory& texFactory, int destinationDescriptorOffset) const
{
    for (size_t i = 0; i < textureNames.size(); ++i)
    {
        texFactory.CreateTexture(textureNames[i].c_str(), destinationDescriptorOffset + static_cast<int>(i));
    }

    return static_cast<int>(textureNames.size());
}


// Load texture resources (helper function).
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


// Load VB/IB resources for static geometry.
_Use_decl_annotations_
void Model::LoadStaticBuffers(
    ID3D12Device* device,
    ResourceUploadBatch& resourceUploadBatch,
    bool keepMemory)
{
    // Gather all unique parts
    std::set<ModelMeshPart*> uniqueParts;
    for (const auto& mesh : meshes)
    {
        for (const auto& part : mesh->opaqueMeshParts)
        {
            uniqueParts.insert(part.get());
        }
        for (const auto& part : mesh->alphaMeshParts)
        {
            uniqueParts.insert(part.get());
        }
    }

    const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);

    for (auto it = uniqueParts.cbegin(); it != uniqueParts.cend(); ++it)
    {
        auto part = *it;

        // Convert dynamic VB to static VB
        if (!part->staticVertexBuffer)
        {
            if (!part->vertexBuffer)
            {
                DebugTrace("ERROR: Model part missing vertex buffer!\n");
                throw std::runtime_error("ModelMeshPart");
            }

            part->vertexBufferSize = static_cast<uint32_t>(part->vertexBuffer.Size());

            auto const desc = CD3DX12_RESOURCE_DESC::Buffer(part->vertexBuffer.Size());

            ThrowIfFailed(device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &desc,
                c_initialCopyTargetState,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(part->staticVertexBuffer.GetAddressOf())
            ));

            SetDebugObjectName(part->staticVertexBuffer.Get(), L"ModelMeshPart");

            resourceUploadBatch.Upload(part->staticVertexBuffer.Get(), part->vertexBuffer);

            resourceUploadBatch.Transition(part->staticVertexBuffer.Get(),
                D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

            // Scan for any other part with the same vertex buffer for sharing
            for (auto sit = std::next(it); sit != uniqueParts.cend(); ++sit)
            {
                auto sharePart = *sit;
                assert(sharePart != part);

                if (sharePart->staticVertexBuffer)
                    continue;

                if (sharePart->vertexBuffer == part->vertexBuffer)
                {
                    sharePart->vertexBufferSize = part->vertexBufferSize;
                    sharePart->staticVertexBuffer = part->staticVertexBuffer;

                    if (!keepMemory)
                    {
                        sharePart->vertexBuffer.Reset();
                    }
                }
            }

            if (!keepMemory)
            {
                part->vertexBuffer.Reset();
            }
        }

        // Convert dynamic IB to static IB
        if (!part->staticIndexBuffer)
        {
            if (!part->indexBuffer)
            {
                DebugTrace("ERROR: Model part missing index buffer!\n");
                throw std::runtime_error("ModelMeshPart");
            }

            part->indexBufferSize = static_cast<uint32_t>(part->indexBuffer.Size());

            auto const desc = CD3DX12_RESOURCE_DESC::Buffer(part->indexBuffer.Size());

            ThrowIfFailed(device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &desc,
                c_initialCopyTargetState,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(part->staticIndexBuffer.GetAddressOf())
            ));

            SetDebugObjectName(part->staticIndexBuffer.Get(), L"ModelMeshPart");

            resourceUploadBatch.Upload(part->staticIndexBuffer.Get(), part->indexBuffer);

            resourceUploadBatch.Transition(part->staticIndexBuffer.Get(),
                D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);

            // Scan for any other part with the same index buffer for sharing
            for (auto sit = std::next(it); sit != uniqueParts.cend(); ++sit)
            {
                auto sharePart = *sit;
                assert(sharePart != part);

                if (sharePart->staticIndexBuffer)
                    continue;

                if (sharePart->indexBuffer == part->indexBuffer)
                {
                    sharePart->indexBufferSize = part->indexBufferSize;
                    sharePart->staticIndexBuffer = part->staticIndexBuffer;

                    if (!keepMemory)
                    {
                        sharePart->indexBuffer.Reset();
                    }
                }
            }

            if (!keepMemory)
            {
                part->indexBuffer.Reset();
            }
        }
    }
}


// Create effects for each mesh piece.
Model::EffectCollection Model::CreateEffects(
    IEffectFactory& fxFactory,
    const EffectPipelineStateDescription& opaquePipelineState,
    const EffectPipelineStateDescription& alphaPipelineState,
    int textureDescriptorOffset,
    int samplerDescriptorOffset) const
{
    if (materials.empty())
    {
        DebugTrace("ERROR: Model has no material information to create effects!\n");
        throw std::runtime_error("CreateEffects");
    }

    EffectCollection effects;

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


// Private helper for creating an effect for a mesh part.
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

    if (!part->vbDecl || part->vbDecl->empty())
        throw std::runtime_error("Model mesh part missing vertex buffer input elements data");

    if (part->vbDecl->size() > D3D12_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT)
        throw std::runtime_error("Model mesh part input layout size is too large for DirectX 12");

    D3D12_INPUT_LAYOUT_DESC il = {};
    il.NumElements = static_cast<UINT>(part->vbDecl->size());
    il.pInputElementDescs = part->vbDecl->data();

    return fxFactory.CreateEffect(m, opaquePipelineState, alphaPipelineState, il, textureDescriptorOffset, samplerDescriptorOffset);
}


// Create effects for each mesh piece with the default factory.
_Use_decl_annotations_
Model::EffectCollection Model::CreateEffects(
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


// Compute using bone hierarchy from model bone matrices to an array.
_Use_decl_annotations_
void Model::CopyAbsoluteBoneTransformsTo(
    size_t nbones,
    XMMATRIX* boneTransforms) const
{
    if (!nbones || !boneTransforms)
    {
        throw std::invalid_argument("Bone transforms array required");
    }

    if (nbones < bones.size())
    {
        throw std::invalid_argument("Bone transforms array is too small");
    }

    if (bones.empty() || !boneMatrices)
    {
        throw std::runtime_error("Model is missing bones");
    }

    memset(boneTransforms, 0, sizeof(XMMATRIX) * nbones);

    const XMMATRIX id = XMMatrixIdentity();
    size_t visited = 0;
    ComputeAbsolute(0, id, bones.size(), boneMatrices.get(), boneTransforms, visited);
}


// Compute using bone hierarchy from one array to another array.
_Use_decl_annotations_
void Model::CopyAbsoluteBoneTransforms(
    size_t nbones,
    const XMMATRIX* inBoneTransforms,
    XMMATRIX* outBoneTransforms) const
{
    if (!nbones || !inBoneTransforms || !outBoneTransforms)
    {
        throw std::invalid_argument("Bone transforms arrays required");
    }

    if (nbones < bones.size())
    {
        throw std::invalid_argument("Bone transforms arrays are too small");
    }

    if (bones.empty())
    {
        throw std::runtime_error("Model is missing bones");
    }

    memset(outBoneTransforms, 0, sizeof(XMMATRIX) * nbones);

    const XMMATRIX id = XMMatrixIdentity();
    size_t visited = 0;
    ComputeAbsolute(0, id, bones.size(), inBoneTransforms, outBoneTransforms, visited);
}


// Private helper for computing hierarchical transforms using bones via recursion.
_Use_decl_annotations_
void Model::ComputeAbsolute(
    uint32_t index,
    CXMMATRIX parent,
    size_t nbones,
    const XMMATRIX* inBoneTransforms,
    XMMATRIX* outBoneTransforms,
    size_t& visited) const
{
    if (index == ModelBone::c_Invalid || index >= nbones)
        return;

    assert(inBoneTransforms != nullptr && outBoneTransforms != nullptr);

    ++visited; // Cycle detection safety!
    if (visited > bones.size())
    {
        DebugTrace("ERROR: Model::CopyAbsoluteBoneTransformsTo encountered a cycle in the bones!\n");
        throw std::runtime_error("Model bones form an invalid graph");
    }

    XMMATRIX local = inBoneTransforms[index];
    local = XMMatrixMultiply(local, parent);
    outBoneTransforms[index] = local;

    if (bones[index].siblingIndex != ModelBone::c_Invalid)
    {
        ComputeAbsolute(bones[index].siblingIndex, parent, nbones,
            inBoneTransforms, outBoneTransforms, visited);
    }

    if (bones[index].childIndex != ModelBone::c_Invalid)
    {
        ComputeAbsolute(bones[index].childIndex, local, nbones,
            inBoneTransforms, outBoneTransforms, visited);
    }
}


// Copy the model bone matrices from an array.
_Use_decl_annotations_
void Model::CopyBoneTransformsFrom(size_t nbones, const XMMATRIX* boneTransforms)
{
    if (!nbones || !boneTransforms)
    {
        throw std::invalid_argument("Bone transforms array required");
    }

    if (nbones < bones.size())
    {
        throw std::invalid_argument("Bone transforms array is too small");
    }

    if (bones.empty())
    {
        throw std::runtime_error("Model is missing bones");
    }

    if (!boneMatrices)
    {
        boneMatrices = ModelBone::MakeArray(bones.size());
    }

    memcpy(boneMatrices.get(), boneTransforms, bones.size() * sizeof(XMMATRIX));
}


// Copy the model bone matrices to an array.
_Use_decl_annotations_
void Model::CopyBoneTransformsTo(size_t nbones, XMMATRIX* boneTransforms) const
{
    if (!nbones || !boneTransforms)
    {
        throw std::invalid_argument("Bone transforms array required");
    }

    if (nbones < bones.size())
    {
        throw std::invalid_argument("Bone transforms array is too small");
    }

    if (bones.empty())
    {
        throw std::runtime_error("Model is missing bones");
    }

    memcpy(boneTransforms, boneMatrices.get(), bones.size() * sizeof(XMMATRIX));
}


// Updates effect matrices (if applicable).
void XM_CALLCONV Model::UpdateEffectMatrices(
    EffectCollection& effects,
    FXMMATRIX world,
    CXMMATRIX view,
    CXMMATRIX proj)
{
    for (auto& fx : effects)
    {
        auto imatrices = dynamic_cast<IEffectMatrices*>(fx.get());
        if (imatrices)
        {
            imatrices->SetMatrices(world, view, proj);
        }
    }
}


// Transition static VB/IB resources (if applicable).
void Model::Transition(
    _In_ ID3D12GraphicsCommandList* commandList,
    D3D12_RESOURCE_STATES stateBeforeVB,
    D3D12_RESOURCE_STATES stateAfterVB,
    D3D12_RESOURCE_STATES stateBeforeIB,
    D3D12_RESOURCE_STATES stateAfterIB)
{
    UINT count = 0;
    D3D12_RESOURCE_BARRIER barrier[64] = {};

    for (auto& mit : meshes)
    {
        for (auto& pit : mit->opaqueMeshParts)
        {
            assert(count < std::size(barrier));
            _Analysis_assume_(count < std::size(barrier));

            if (stateBeforeIB != stateAfterIB && pit->staticIndexBuffer)
            {
                barrier[count].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier[count].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                barrier[count].Transition.pResource = pit->staticIndexBuffer.Get();
                barrier[count].Transition.StateBefore = stateBeforeIB;
                barrier[count].Transition.StateAfter = stateAfterIB;
                ++count;

                if (count >= std::size(barrier))
                {
                    commandList->ResourceBarrier(count, barrier);
                    count = 0;
                }
            }

            if (stateBeforeVB != stateAfterVB && pit->staticVertexBuffer)
            {
                barrier[count].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier[count].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                barrier[count].Transition.pResource = pit->staticVertexBuffer.Get();
                barrier[count].Transition.StateBefore = stateBeforeVB;
                barrier[count].Transition.StateAfter = stateAfterVB;
                ++count;

                if (count >= std::size(barrier))
                {
                    commandList->ResourceBarrier(count, barrier);
                    count = 0;
                }
            }
        }

        for (auto& pit : mit->alphaMeshParts)
        {
            assert(count < std::size(barrier));
            _Analysis_assume_(count < std::size(barrier));

            if (stateBeforeIB != stateAfterIB && pit->staticIndexBuffer)
            {
                barrier[count].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier[count].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                barrier[count].Transition.pResource = pit->staticIndexBuffer.Get();
                barrier[count].Transition.StateBefore = stateBeforeIB;
                barrier[count].Transition.StateAfter = stateAfterIB;
                ++count;

                if (count >= std::size(barrier))
                {
                    commandList->ResourceBarrier(count, barrier);
                    count = 0;
                }
            }

            if (stateBeforeVB != stateAfterVB && pit->staticVertexBuffer)
            {
                barrier[count].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier[count].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                barrier[count].Transition.pResource = pit->staticVertexBuffer.Get();
                barrier[count].Transition.StateBefore = stateBeforeVB;
                barrier[count].Transition.StateAfter = stateAfterVB;
                ++count;

                if (count >= std::size(barrier))
                {
                    commandList->ResourceBarrier(count, barrier);
                    count = 0;
                }
            }
        }
    }

    if (count > 0)
    {
        commandList->ResourceBarrier(count, barrier);
    }
}


//--------------------------------------------------------------------------------------
// Adapters for /Zc:wchar_t- clients

#if defined(_MSC_VER) && !defined(_NATIVE_WCHAR_T_DEFINED)

_Use_decl_annotations_
std::unique_ptr<EffectTextureFactory> Model::LoadTextures(
    ID3D12Device* device,
    ResourceUploadBatch& resourceUploadBatch,
    const __wchar_t* texturesPath,
    D3D12_DESCRIPTOR_HEAP_FLAGS flags) const
{
    return LoadTextures(device, resourceUploadBatch, reinterpret_cast<const unsigned short*>(texturesPath), flags);
}

#endif
