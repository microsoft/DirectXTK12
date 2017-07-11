//--------------------------------------------------------------------------------------
// File: PrimitiveBatch.cpp
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
#include "PrimitiveBatch.h"
#include "DirectXHelpers.h"
#include "PlatformHelpers.h"
#include "GraphicsMemory.h"

using namespace DirectX;
using namespace DirectX::Internal;
using Microsoft::WRL::ComPtr;


// Internal PrimitiveBatch implementation class.
class PrimitiveBatchBase::Impl
{
public:
    Impl(_In_ ID3D12Device* device, size_t maxIndices, size_t maxVertices, size_t vertexSize);

    void Begin( _In_ ID3D12GraphicsCommandList* cmdList );
    void End();

    void Draw(D3D_PRIMITIVE_TOPOLOGY topology, bool isIndexed, _In_opt_count_(indexCount) uint16_t const* indices, size_t indexCount, size_t vertexCount, _Outptr_ void** pMappedVertices);

private:
    void FlushBatch();

    GraphicsResource mVertexSegment;
    GraphicsResource mIndexSegment;

    ComPtr<ID3D12Device> mDevice;
    ComPtr<ID3D12GraphicsCommandList> mCommandList;
    
    size_t mMaxIndices;
    size_t mMaxVertices;
    size_t mVertexSize;
    size_t mVertexPageSize;
    size_t mIndexPageSize;
   
    D3D_PRIMITIVE_TOPOLOGY mCurrentTopology;
    bool mInBeginEndPair;
    bool mCurrentlyIndexed;
    
    size_t mIndexCount;
    size_t mVertexCount;
    
    size_t mBaseIndex;
    size_t mBaseVertex;
};


// Constructor.
PrimitiveBatchBase::Impl::Impl(_In_ ID3D12Device* device, size_t maxIndices, size_t maxVertices, size_t vertexSize)
  : mDevice(device),
    mCommandList(nullptr),
    mMaxIndices(maxIndices),
    mMaxVertices(maxVertices),
    mVertexSize(vertexSize),
    mVertexPageSize(maxVertices * vertexSize),
    mIndexPageSize(maxIndices * sizeof(uint16_t)),
    mCurrentTopology(D3D_PRIMITIVE_TOPOLOGY_UNDEFINED),
    mInBeginEndPair(false),
    mCurrentlyIndexed(false),
    mIndexCount(0),
    mVertexCount(0)
{
    if (maxVertices == 0)
        throw std::exception("maxVertices must be greater than 0");
}


// Begins a batch of primitive drawing operations.

void PrimitiveBatchBase::Impl::Begin( _In_ ID3D12GraphicsCommandList* cmdList )
{
    if (mInBeginEndPair)
        throw std::exception("Cannot nest Begin calls");

    mCommandList = cmdList;
    mInBeginEndPair = true;
}


// Ends a batch of primitive drawing operations.
void PrimitiveBatchBase::Impl::End()
{
    if (!mInBeginEndPair)
        throw std::exception("Begin must be called before End");

    FlushBatch();

    // Release our smart pointers and end the block
    mIndexSegment.Reset();
    mVertexSegment.Reset();
    mCommandList.Reset();
    mInBeginEndPair = false;
}


// Can we combine adjacent primitives using this topology into a single draw call?
static bool CanBatchPrimitives(D3D_PRIMITIVE_TOPOLOGY topology)
{
    switch (topology)
    {
        case D3D_PRIMITIVE_TOPOLOGY_POINTLIST:
        case D3D_PRIMITIVE_TOPOLOGY_LINELIST:
        case D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST:
            // Lists can easily be merged.
            return true;

        default:
            // Strips cannot.
            return false;
    }

    // We could also merge indexed strips by inserting degenerates,
    // but that's not always a perf win, so let's keep things simple.
}


// Adds new geometry to the batch.
_Use_decl_annotations_
void PrimitiveBatchBase::Impl::Draw(D3D_PRIMITIVE_TOPOLOGY topology, bool isIndexed, uint16_t const* indices, size_t indexCount, size_t vertexCount, void** pMappedVertices)
{
    if (isIndexed && !indices)
        throw std::exception("Indices cannot be null");

    if (indexCount >= mMaxIndices)
        throw std::exception("Too many indices");

    if (vertexCount >= mMaxVertices)
        throw std::exception("Too many vertices");

    if (!mInBeginEndPair)
        throw std::exception("Begin must be called before Draw");

    assert(pMappedVertices != nullptr);

    // Can we merge this primitive in with an existing batch, or must we flush first?
    bool wrapIndexBuffer = (mIndexCount + indexCount > mMaxIndices);
    bool wrapVertexBuffer = (mVertexCount + vertexCount > mMaxVertices);

    if ((topology != mCurrentTopology) ||
        (isIndexed != mCurrentlyIndexed) ||
        !CanBatchPrimitives(topology) ||
        wrapIndexBuffer || wrapVertexBuffer)
    {
        FlushBatch();
    }
    
    // If we are not already in a batch, lock the buffers.
    if (mCurrentTopology == D3D_PRIMITIVE_TOPOLOGY_UNDEFINED)
    {
        mIndexCount = 0;
        mVertexCount = 0;
        mBaseIndex = 0;
        mBaseVertex = 0;
        mCurrentTopology = topology;
        mCurrentlyIndexed = isIndexed;

        // Allocate a page for the primitive data
        if (isIndexed)
            mIndexSegment = GraphicsMemory::Get().Allocate(mIndexPageSize);

        mVertexSegment = GraphicsMemory::Get().Allocate(mVertexPageSize);
    }
    
    // Copy over the index data.
    if (isIndexed)
    {
        uint16_t* outputIndices = (uint16_t*)mIndexSegment.Memory() + mIndexCount;
        
        for (size_t i = 0; i < indexCount; i++)
        {
            outputIndices[i] = (uint16_t)(indices[i] + mVertexCount - mBaseIndex);
        }
 
        mIndexCount += indexCount;
    }

    // Return the output vertex data location.
    *pMappedVertices = (uint8_t*)mVertexSegment.Memory() + mVertexSize * mVertexCount;

    mVertexCount += vertexCount;
}


// Sends queued primitives to the graphics device.
void PrimitiveBatchBase::Impl::FlushBatch()
{
    // Early out if there is nothing to flush.
    if (mCurrentTopology == D3D_PRIMITIVE_TOPOLOGY_UNDEFINED)
        return;
  
    mCommandList->IASetPrimitiveTopology(mCurrentTopology);

    // Set the vertex buffer view
    D3D12_VERTEX_BUFFER_VIEW vbv;
    vbv.BufferLocation = mVertexSegment.GpuAddress();
    vbv.SizeInBytes = static_cast<UINT>( mVertexSize * (mVertexCount - mBaseVertex) );
    vbv.StrideInBytes = static_cast<UINT>(mVertexSize);
    mCommandList->IASetVertexBuffers(0, 1, &vbv);

    if (mCurrentlyIndexed)
    {
        // Set the index buffer view
        D3D12_INDEX_BUFFER_VIEW ibv;
        ibv.BufferLocation = mIndexSegment.GpuAddress();
        ibv.Format = DXGI_FORMAT_R16_UINT;
        ibv.SizeInBytes = static_cast<UINT>(mIndexCount - mBaseIndex) * sizeof(uint16_t);
        mCommandList->IASetIndexBuffer(&ibv);

        // Draw indexed geometry.
        mCommandList->DrawIndexedInstanced(static_cast<UINT>(mIndexCount - mBaseIndex), 1, 0, 0, 0);
    }
    else
    {
        // Draw non-indexed geometry.
        mCommandList->DrawInstanced(static_cast<UINT>(mVertexCount - mBaseVertex), 1, 0, 0);
    }

    mCurrentTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
}


// Public constructor.
PrimitiveBatchBase::PrimitiveBatchBase(_In_ ID3D12Device* device, size_t maxIndices, size_t maxVertices, size_t vertexSize)
  : pImpl(new Impl(device, maxIndices, maxVertices, vertexSize))
{
}


// Move constructor.
PrimitiveBatchBase::PrimitiveBatchBase(PrimitiveBatchBase&& moveFrom)
  : pImpl(std::move(moveFrom.pImpl))
{
}


// Move assignment.
PrimitiveBatchBase& PrimitiveBatchBase::operator= (PrimitiveBatchBase&& moveFrom)
{
    pImpl = std::move(moveFrom.pImpl);
    return *this;
}


// Public destructor.
PrimitiveBatchBase::~PrimitiveBatchBase()
{
}


void PrimitiveBatchBase::Begin( _In_ ID3D12GraphicsCommandList* cmdList )
{
    pImpl->Begin( cmdList );
}


void PrimitiveBatchBase::End()
{
    pImpl->End();
}


_Use_decl_annotations_
void PrimitiveBatchBase::Draw(D3D12_PRIMITIVE_TOPOLOGY topology, bool isIndexed, uint16_t const* indices, size_t indexCount, size_t vertexCount, void** pMappedVertices)
{
    pImpl->Draw(topology, isIndexed, indices, indexCount, vertexCount, pMappedVertices);
}
