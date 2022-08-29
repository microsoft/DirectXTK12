//--------------------------------------------------------------------------------------
// File: GeometricPrimitive.h
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#pragma once

#include "VertexTypes.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>


namespace DirectX
{
    class ResourceUploadBatch;

    inline namespace DX12
    {
        class IEffect;

        class GeometricPrimitive
        {
        public:
            GeometricPrimitive(GeometricPrimitive&&) = default;
            GeometricPrimitive& operator= (GeometricPrimitive&&) = default;

            GeometricPrimitive(GeometricPrimitive const&) = delete;
            GeometricPrimitive& operator= (GeometricPrimitive const&) = delete;

            virtual ~GeometricPrimitive();

            using VertexType = VertexPositionNormalTexture;
            using VertexCollection = std::vector<VertexType>;
            using IndexCollection = std::vector<uint16_t>;

            // Factory methods.
            static std::unique_ptr<GeometricPrimitive> __cdecl CreateCube(float size = 1, bool rhcoords = true, _In_opt_ ID3D12Device* device = nullptr);
            static std::unique_ptr<GeometricPrimitive> __cdecl CreateBox(const XMFLOAT3& size, bool rhcoords = true, bool invertn = false, _In_opt_ ID3D12Device* device = nullptr);
            static std::unique_ptr<GeometricPrimitive> __cdecl CreateSphere(float diameter = 1, size_t tessellation = 16, bool rhcoords = true, bool invertn = false, _In_opt_ ID3D12Device* device = nullptr);
            static std::unique_ptr<GeometricPrimitive> __cdecl CreateGeoSphere(float diameter = 1, size_t tessellation = 3, bool rhcoords = true, _In_opt_ ID3D12Device* device = nullptr);
            static std::unique_ptr<GeometricPrimitive> __cdecl CreateCylinder(float height = 1, float diameter = 1, size_t tessellation = 32, bool rhcoords = true, _In_opt_ ID3D12Device* device = nullptr);
            static std::unique_ptr<GeometricPrimitive> __cdecl CreateCone(float diameter = 1, float height = 1, size_t tessellation = 32, bool rhcoords = true, _In_opt_ ID3D12Device* device = nullptr);
            static std::unique_ptr<GeometricPrimitive> __cdecl CreateTorus(float diameter = 1, float thickness = 0.333f, size_t tessellation = 32, bool rhcoords = true, _In_opt_ ID3D12Device* device = nullptr);
            static std::unique_ptr<GeometricPrimitive> __cdecl CreateTetrahedron(float size = 1, bool rhcoords = true, _In_opt_ ID3D12Device* device = nullptr);
            static std::unique_ptr<GeometricPrimitive> __cdecl CreateOctahedron(float size = 1, bool rhcoords = true, _In_opt_ ID3D12Device* device = nullptr);
            static std::unique_ptr<GeometricPrimitive> __cdecl CreateDodecahedron(float size = 1, bool rhcoords = true, _In_opt_ ID3D12Device* device = nullptr);
            static std::unique_ptr<GeometricPrimitive> __cdecl CreateIcosahedron(float size = 1, bool rhcoords = true, _In_opt_ ID3D12Device* device = nullptr);
            static std::unique_ptr<GeometricPrimitive> __cdecl CreateTeapot(float size = 1, size_t tessellation = 8, bool rhcoords = true, _In_opt_ ID3D12Device* device = nullptr);
            static std::unique_ptr<GeometricPrimitive> __cdecl CreateCustom(const VertexCollection& vertices, const IndexCollection& indices, _In_opt_ ID3D12Device* device = nullptr);

            static void __cdecl CreateCube(VertexCollection& vertices, IndexCollection& indices, float size = 1, bool rhcoords = true);
            static void __cdecl CreateBox(VertexCollection& vertices, IndexCollection& indices, const XMFLOAT3& size, bool rhcoords = true, bool invertn = false);
            static void __cdecl CreateSphere(VertexCollection& vertices, IndexCollection& indices, float diameter = 1, size_t tessellation = 16, bool rhcoords = true, bool invertn = false);
            static void __cdecl CreateGeoSphere(VertexCollection& vertices, IndexCollection& indices, float diameter = 1, size_t tessellation = 3, bool rhcoords = true);
            static void __cdecl CreateCylinder(VertexCollection& vertices, IndexCollection& indices, float height = 1, float diameter = 1, size_t tessellation = 32, bool rhcoords = true);
            static void __cdecl CreateCone(VertexCollection& vertices, IndexCollection& indices, float diameter = 1, float height = 1, size_t tessellation = 32, bool rhcoords = true);
            static void __cdecl CreateTorus(VertexCollection& vertices, IndexCollection& indices, float diameter = 1, float thickness = 0.333f, size_t tessellation = 32, bool rhcoords = true);
            static void __cdecl CreateTetrahedron(VertexCollection& vertices, IndexCollection& indices, float size = 1, bool rhcoords = true);
            static void __cdecl CreateOctahedron(VertexCollection& vertices, IndexCollection& indices, float size = 1, bool rhcoords = true);
            static void __cdecl CreateDodecahedron(VertexCollection& vertices, IndexCollection& indices, float size = 1, bool rhcoords = true);
            static void __cdecl CreateIcosahedron(VertexCollection& vertices, IndexCollection& indices, float size = 1, bool rhcoords = true);
            static void __cdecl CreateTeapot(VertexCollection& vertices, IndexCollection& indices, float size = 1, size_t tessellation = 8, bool rhcoords = true);

            // Load VB/IB resources for static geometry.
            void __cdecl LoadStaticBuffers(
                _In_ ID3D12Device* device,
                ResourceUploadBatch& resourceUploadBatch);

            // Transition VB/IB resources for static geometry.
            void __cdecl Transition(
                _In_ ID3D12GraphicsCommandList* commandList,
                D3D12_RESOURCE_STATES stateBeforeVB,
                D3D12_RESOURCE_STATES stateAfterVB,
                D3D12_RESOURCE_STATES stateBeforeIB,
                D3D12_RESOURCE_STATES stateAfterIB);

            // Draw the primitive.
            void __cdecl Draw(_In_ ID3D12GraphicsCommandList* commandList) const;

            void __cdecl DrawInstanced(_In_ ID3D12GraphicsCommandList* commandList, uint32_t instanceCount, uint32_t startInstanceLocation = 0) const;

        private:
            GeometricPrimitive() noexcept(false);

            // Private implementation.
            class Impl;

            std::unique_ptr<Impl> pImpl;
        };
    }
}
