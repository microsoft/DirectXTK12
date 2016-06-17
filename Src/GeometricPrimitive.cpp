//--------------------------------------------------------------------------------------
// File: GeometricPrimitive.cpp
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
#include "GeometricPrimitive.h"
#include "Effects.h"
#include "CommonStates.h"
#include "DirectXHelpers.h"
#include "SharedResourcePool.h"
#include "Geometry.h"
#include "PrimitiveBatch.h"
#include "GraphicsMemory.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

// Internal GeometricPrimitive implementation class.
class GeometricPrimitive::Impl
{
public:
    void Initialize(_In_ ID3D12Device* device, const VertexCollection& vertices, const IndexCollection& indices);

    void Draw(_In_ ID3D12GraphicsCommandList* commandList, _In_ IEffect* effect);

    void XM_CALLCONV Draw(_In_ ID3D12GraphicsCommandList* commandList, FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection, FXMVECTOR color, _In_opt_ D3D12_GPU_DESCRIPTOR_HANDLE* texture = nullptr);
    
    UINT					 mIndexCount;
    GraphicsResource		 mIndexBuffer;
    GraphicsResource		 mVertexBuffer;

    D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
    D3D12_INDEX_BUFFER_VIEW  mIndexBufferView;

private:
    
    // Only one of these helpers is allocated per D3D device context, even if there are multiple GeometricPrimitive instances.
    class SharedResources
    {
    public:
        SharedResources(_In_ ID3D12Device* device);

        ComPtr<ID3D12Device>  m_device;
        std::unique_ptr<BasicEffect> effect;
    };


    // Per-device-context data.
    std::shared_ptr<SharedResources> mResources;

    static SharedResourcePool<ID3D12Device*, SharedResources> sharedResourcesPool;
};

// Global pool of per-device GeometricPrimitive resources.
SharedResourcePool<ID3D12Device*, GeometricPrimitive::Impl::SharedResources> GeometricPrimitive::Impl::sharedResourcesPool;


// Per-device-context constructor.
GeometricPrimitive::Impl::SharedResources::SharedResources(_In_ ID3D12Device* device)
    : m_device(device)
{
    // We assume this RT. 
    RenderTargetState rtState(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_D32_FLOAT);

    EffectPipelineStateDescription pd(
        &VertexPositionNormalColorTexture::InputLayout,
        &CommonStates::Opaque,
        &CommonStates::DepthDefault,
        &CommonStates::CullNone,
        &rtState
        );

    // Create the BasicEffect.
    effect = std::make_unique<BasicEffect>(device, EffectFlags::PerPixelLighting, pd);
    effect->EnableDefaultLighting();
}

// Initializes a geometric primitive instance that will draw the specified vertex and index data.
_Use_decl_annotations_
void GeometricPrimitive::Impl::Initialize(ID3D12Device* device, const VertexCollection& vertices, const IndexCollection& indices)
{
    if (vertices.size() >= USHRT_MAX)
        throw std::exception("Too many vertices for 16-bit index buffer");

    mResources = sharedResourcesPool.DemandCreate(device);

    // Vertex data
    auto verts = reinterpret_cast<const uint8_t*>(vertices.data());
    size_t vertSizeBytes = vertices.size() * sizeof(vertices[0]);
    
    mVertexBuffer = GraphicsMemory::Get().Allocate(vertSizeBytes);
    memcpy(mVertexBuffer.Memory(), verts, vertSizeBytes);

    // Index data
    auto ind = reinterpret_cast<const uint8_t*>(indices.data());
    size_t indSizeBytes = indices.size() * sizeof(indices[0]);

    mIndexBuffer = GraphicsMemory::Get().Allocate(indSizeBytes);
    memcpy(mIndexBuffer.Memory(), ind, indSizeBytes);

    // Record index count for draw
    mIndexCount = static_cast<UINT>(indices.size());

    // Create views
    mVertexBufferView.BufferLocation = mVertexBuffer.GpuAddress();
    mVertexBufferView.StrideInBytes = static_cast<UINT>(sizeof(VertexCollection::value_type));
    mVertexBufferView.SizeInBytes = (UINT)mVertexBuffer.Size();

    mIndexBufferView.BufferLocation = mIndexBuffer.GpuAddress();
    mIndexBufferView.SizeInBytes = (UINT)mIndexBuffer.Size();
    mIndexBufferView.Format = DXGI_FORMAT_R16_UINT;
}


// Draws the primitive.
_Use_decl_annotations_
void XM_CALLCONV GeometricPrimitive::Impl::Draw(ID3D12GraphicsCommandList* commandList, 
                                                FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection, FXMVECTOR color,
                                                D3D12_GPU_DESCRIPTOR_HANDLE* textureGpuHandle)
{
    assert(mResources != 0);
    auto effect = mResources->effect.get();
    assert(effect != 0);

    if (textureGpuHandle)
    {
        effect->SetTexture(*textureGpuHandle);
    }

    float alpha = XMVectorGetW(color);

    // Set effect parameters.
    effect->SetWorld(world);
    effect->SetView(view);
    effect->SetProjection(projection);

    effect->SetDiffuseColor(color);
    effect->SetAlpha(alpha);
    effect->EnableDefaultLighting();

    Draw(commandList, effect);
}

// Draw the primitive using a custom effect.
_Use_decl_annotations_
void GeometricPrimitive::Impl::Draw(ID3D12GraphicsCommandList* commandList, IEffect* effect)
{
    effect->Apply(commandList);

    commandList->IASetVertexBuffers(0, 1, &mVertexBufferView);
    commandList->IASetIndexBuffer(&mIndexBufferView);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    commandList->DrawIndexedInstanced(mIndexCount, 1, 0, 0, 0);
}

//--------------------------------------------------------------------------------------
// GeometricPrimitive
//--------------------------------------------------------------------------------------

// Constructor.
GeometricPrimitive::GeometricPrimitive()
    : pImpl(new Impl())
{
}


// Destructor.
GeometricPrimitive::~GeometricPrimitive()
{
}


// Public entrypoints.
_Use_decl_annotations_
void XM_CALLCONV GeometricPrimitive::Draw(ID3D12GraphicsCommandList* commandList, FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection, FXMVECTOR color)
{
    pImpl->Draw(commandList, world, view, projection, color);
}

_Use_decl_annotations_
void GeometricPrimitive::Draw(ID3D12GraphicsCommandList* commandList, IEffect* effect)
{
    pImpl->Draw(commandList, effect);
}


//--------------------------------------------------------------------------------------
// Cube (aka a Hexahedron) or Box
//--------------------------------------------------------------------------------------

// Creates a cube primitive.
std::unique_ptr<GeometricPrimitive> GeometricPrimitive::CreateCube(_In_ ID3D12Device* device, float size, bool rhcoords)
{
    VertexCollection vertices;
    IndexCollection indices;
    ComputeBox(vertices, indices, XMFLOAT3(size, size, size), rhcoords, false);

    // Create the primitive object.
    std::unique_ptr<GeometricPrimitive> primitive(new GeometricPrimitive());

    primitive->pImpl->Initialize(device, vertices, indices);

    return primitive;
}

void GeometricPrimitive::CreateCube(std::vector<VertexPositionNormalTexture>& vertices, std::vector<uint16_t>& indices, float size, bool rhcoords)
{
    ComputeBox(vertices, indices, XMFLOAT3(size, size, size), rhcoords, false);
}


// Creates a box primitive.
std::unique_ptr<GeometricPrimitive> GeometricPrimitive::CreateBox(_In_ ID3D12Device* device, const XMFLOAT3& size, bool rhcoords, bool invertn)
{
    VertexCollection vertices;
    IndexCollection indices;
    ComputeBox(vertices, indices, size, rhcoords, invertn);

    // Create the primitive object.
    std::unique_ptr<GeometricPrimitive> primitive(new GeometricPrimitive());

    primitive->pImpl->Initialize(device, vertices, indices);

    return primitive;
}

void GeometricPrimitive::CreateBox(std::vector<VertexPositionNormalTexture>& vertices, std::vector<uint16_t>& indices, const XMFLOAT3& size, bool rhcoords, bool invertn)
{
    ComputeBox(vertices, indices, size, rhcoords, invertn);
}


//--------------------------------------------------------------------------------------
// Sphere
//--------------------------------------------------------------------------------------

// Creates a sphere primitive.
std::unique_ptr<GeometricPrimitive> GeometricPrimitive::CreateSphere(_In_ ID3D12Device* device, float diameter, size_t tessellation, bool rhcoords, bool invertn)
{
    // Create the primitive object.
    std::unique_ptr<GeometricPrimitive> primitive(new GeometricPrimitive());

    VertexCollection vertices;
    IndexCollection indices;

    ComputeSphere(vertices, indices, diameter, tessellation, rhcoords, invertn);

    primitive->pImpl->Initialize(device, vertices, indices);

    return primitive;
}

void GeometricPrimitive::CreateSphere(std::vector<VertexPositionNormalTexture>& vertices, std::vector<uint16_t>& indices, float diameter, size_t tessellation, bool rhcoords, bool invertn)
{
    ComputeSphere(vertices, indices, diameter, tessellation, rhcoords, invertn);
}


//--------------------------------------------------------------------------------------
// Geodesic sphere
//--------------------------------------------------------------------------------------

// Creates a geosphere primitive.
std::unique_ptr<GeometricPrimitive> GeometricPrimitive::CreateGeoSphere(_In_ ID3D12Device* device, float diameter, size_t tessellation, bool rhcoords)
{
    // Create the primitive object.
    std::unique_ptr<GeometricPrimitive> primitive(new GeometricPrimitive());

    VertexCollection vertices;
    IndexCollection indices;
    ComputeGeoSphere(vertices, indices, diameter, tessellation, rhcoords);

    primitive->pImpl->Initialize(device, vertices, indices);

    return primitive;
}

void GeometricPrimitive::CreateGeoSphere(std::vector<VertexPositionNormalTexture>& vertices, std::vector<uint16_t>& indices, float diameter, size_t tessellation, bool rhcoords)
{
    ComputeGeoSphere(vertices, indices, diameter, tessellation, rhcoords);
}


//--------------------------------------------------------------------------------------
// Cylinder / Cone
//--------------------------------------------------------------------------------------

// Creates a cylinder primitive.
std::unique_ptr<GeometricPrimitive> GeometricPrimitive::CreateCylinder(_In_ ID3D12Device* device, float height, float diameter, size_t tessellation, bool rhcoords)
{
    // Create the primitive object.
    std::unique_ptr<GeometricPrimitive> primitive(new GeometricPrimitive());

    VertexCollection vertices;
    IndexCollection indices;
    ComputeCylinder(vertices, indices, height, diameter, tessellation, rhcoords);

    primitive->pImpl->Initialize(device, vertices, indices);

    return primitive;
}

void GeometricPrimitive::CreateCylinder(std::vector<VertexPositionNormalTexture>& vertices, std::vector<uint16_t>& indices, float height, float diameter, size_t tessellation, bool rhcoords)
{
    ComputeCylinder(vertices, indices, height, diameter, tessellation, rhcoords);
}


// Creates a cone primitive.
std::unique_ptr<GeometricPrimitive> GeometricPrimitive::CreateCone(_In_ ID3D12Device* device, float diameter, float height, size_t tessellation, bool rhcoords)
{
    // Create the primitive object.
    std::unique_ptr<GeometricPrimitive> primitive(new GeometricPrimitive());

    VertexCollection vertices;
    IndexCollection indices;
    ComputeCone(vertices, indices, diameter, height, tessellation, rhcoords);

    primitive->pImpl->Initialize(device, vertices, indices);

    return primitive;
}

void GeometricPrimitive::CreateCone(std::vector<VertexPositionNormalTexture>& vertices, std::vector<uint16_t>& indices, float diameter, float height, size_t tessellation, bool rhcoords)
{
    ComputeCone(vertices, indices, diameter, height, tessellation, rhcoords);
}


//--------------------------------------------------------------------------------------
// Torus
//--------------------------------------------------------------------------------------

// Creates a torus primitive.
std::unique_ptr<GeometricPrimitive> GeometricPrimitive::CreateTorus(_In_ ID3D12Device* device, float diameter, float thickness, size_t tessellation, bool rhcoords)
{	
    // Create the primitive object.
    std::unique_ptr<GeometricPrimitive> primitive(new GeometricPrimitive());

    VertexCollection vertices;
    IndexCollection indices;
    ComputeTorus(vertices, indices, diameter, thickness, tessellation, rhcoords);

    primitive->pImpl->Initialize(device, vertices, indices);

    return primitive;
}

void GeometricPrimitive::CreateTorus(std::vector<VertexPositionNormalTexture>& vertices, std::vector<uint16_t>& indices, float diameter, float thickness, size_t tessellation, bool rhcoords)
{
    ComputeTorus(vertices, indices, diameter, thickness, tessellation, rhcoords);
}


//--------------------------------------------------------------------------------------
// Tetrahedron
//--------------------------------------------------------------------------------------

std::unique_ptr<GeometricPrimitive> GeometricPrimitive::CreateTetrahedron(_In_ ID3D12Device* device, float size, bool rhcoords)
{
    // Create the primitive object.
    std::unique_ptr<GeometricPrimitive> primitive(new GeometricPrimitive());

    VertexCollection vertices;
    IndexCollection indices;
    ComputeTetrahedron(vertices, indices, size, rhcoords);

    primitive->pImpl->Initialize(device, vertices, indices);

    return primitive;
}

void GeometricPrimitive::CreateTetrahedron(std::vector<VertexPositionNormalTexture>& vertices, std::vector<uint16_t>& indices, float size, bool rhcoords)
{
    ComputeTetrahedron(vertices, indices, size, rhcoords);
}


//--------------------------------------------------------------------------------------
// Octahedron
//--------------------------------------------------------------------------------------

std::unique_ptr<GeometricPrimitive> GeometricPrimitive::CreateOctahedron(_In_ ID3D12Device* device, float size, bool rhcoords)
{
    // Create the primitive object.
    std::unique_ptr<GeometricPrimitive> primitive(new GeometricPrimitive());

    VertexCollection vertices;
    IndexCollection indices;
    ComputeOctahedron(vertices, indices, size, rhcoords);

    primitive->pImpl->Initialize(device, vertices, indices);

    return primitive;
}

void GeometricPrimitive::CreateOctahedron(std::vector<VertexPositionNormalTexture>& vertices, std::vector<uint16_t>& indices, float size, bool rhcoords)
{
    ComputeOctahedron(vertices, indices, size, rhcoords);
}


//--------------------------------------------------------------------------------------
// Dodecahedron
//--------------------------------------------------------------------------------------

std::unique_ptr<GeometricPrimitive> GeometricPrimitive::CreateDodecahedron(_In_ ID3D12Device* device, float size, bool rhcoords)
{
    // Create the primitive object.
    std::unique_ptr<GeometricPrimitive> primitive(new GeometricPrimitive());

    VertexCollection vertices;
    IndexCollection indices;
    ComputeDodecahedron(vertices, indices, size, rhcoords);

    primitive->pImpl->Initialize(device, vertices, indices);

    return primitive;
}

void GeometricPrimitive::CreateDodecahedron(std::vector<VertexPositionNormalTexture>& vertices, std::vector<uint16_t>& indices, float size, bool rhcoords)
{
    ComputeDodecahedron(vertices, indices, size, rhcoords);
}


//--------------------------------------------------------------------------------------
// Icosahedron
//--------------------------------------------------------------------------------------

std::unique_ptr<GeometricPrimitive> GeometricPrimitive::CreateIcosahedron(_In_ ID3D12Device* device, float size, bool rhcoords)
{
    // Create the primitive object.
    std::unique_ptr<GeometricPrimitive> primitive(new GeometricPrimitive());

    VertexCollection vertices;
    IndexCollection indices;
    ComputeIcosahedron(vertices, indices, size, rhcoords);

    primitive->pImpl->Initialize(device, vertices, indices);

    return primitive;
}

void GeometricPrimitive::CreateIcosahedron(std::vector<VertexPositionNormalTexture>& vertices, std::vector<uint16_t>& indices, float size, bool rhcoords)
{
    ComputeIcosahedron(vertices, indices, size, rhcoords);
}


//--------------------------------------------------------------------------------------
// Teapot
//--------------------------------------------------------------------------------------

// Creates a teapot primitive.
std::unique_ptr<GeometricPrimitive> GeometricPrimitive::CreateTeapot(_In_ ID3D12Device* device, float size, size_t tessellation, bool rhcoords)
{
    // Create the primitive object.
    std::unique_ptr<GeometricPrimitive> primitive(new GeometricPrimitive());

    VertexCollection vertices;
    IndexCollection indices;
    ComputeTeapot(vertices, indices, size, tessellation, rhcoords);

    primitive->pImpl->Initialize(device, vertices, indices);

    return primitive;
}

void GeometricPrimitive::CreateTeapot(std::vector<VertexPositionNormalTexture>& vertices, std::vector<uint16_t>& indices, float size, size_t tessellation, bool rhcoords)
{
    ComputeTeapot(vertices, indices, size, tessellation, rhcoords);
}


//--------------------------------------------------------------------------------------
// Custom
//--------------------------------------------------------------------------------------

std::unique_ptr<GeometricPrimitive> GeometricPrimitive::CreateCustom(_In_ ID3D12Device* device, const std::vector<VertexPositionNormalTexture>& vertices, const std::vector<uint16_t>& indices)
{
    // Extra validation
    if (vertices.empty() || indices.empty())
        throw std::exception("Requires both vertices and indices");

    if (indices.size() % 3)
        throw std::exception("Expected triangular faces");

    size_t nVerts = vertices.size();
    if (nVerts >= USHRT_MAX)
        throw std::exception("Too many vertices for 16-bit index buffer");

    for (auto it = indices.cbegin(); it != indices.cend(); ++it)
    {
        if (*it >= nVerts)
        {
            throw std::exception("Index not in vertices list");
        }
    }
    // Create the primitive object.
    std::unique_ptr<GeometricPrimitive> primitive(new GeometricPrimitive());

    // copy geometry
    primitive->pImpl->Initialize(device, vertices, indices);

    return primitive;
}
