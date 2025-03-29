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

#ifndef DIRECTX_TOOLKIT_API
#ifdef DIRECTX_TOOLKIT_EXPORT
#ifdef __GNUC__
#define DIRECTX_TOOLKIT_API __attribute__ ((dllexport))
#else
#define DIRECTX_TOOLKIT_API __declspec(dllexport)
#endif
#elif defined(DIRECTX_TOOLKIT_IMPORT)
#ifdef __GNUC__
#define DIRECTX_TOOLKIT_API __attribute__ ((dllimport))
#else
#define DIRECTX_TOOLKIT_API __declspec(dllimport)
#endif
#else
#define DIRECTX_TOOLKIT_API
#endif
#endif


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

            DIRECTX_TOOLKIT_API virtual ~GeometricPrimitive();

            using VertexType = VertexPositionNormalTexture;
            using VertexCollection = std::vector<VertexType>;
            using IndexCollection = std::vector<uint16_t>;

            // Factory methods.
            DIRECTX_TOOLKIT_API static std::unique_ptr<GeometricPrimitive> __cdecl CreateCube(float size = 1, bool rhcoords = true, _In_opt_ ID3D12Device* device = nullptr);
            DIRECTX_TOOLKIT_API static std::unique_ptr<GeometricPrimitive> __cdecl CreateBox(const XMFLOAT3& size, bool rhcoords = true, bool invertn = false, _In_opt_ ID3D12Device* device = nullptr);
            DIRECTX_TOOLKIT_API static std::unique_ptr<GeometricPrimitive> __cdecl CreateSphere(float diameter = 1, size_t tessellation = 16, bool rhcoords = true, bool invertn = false, _In_opt_ ID3D12Device* device = nullptr);
            DIRECTX_TOOLKIT_API static std::unique_ptr<GeometricPrimitive> __cdecl CreateGeoSphere(float diameter = 1, size_t tessellation = 3, bool rhcoords = true, _In_opt_ ID3D12Device* device = nullptr);
            DIRECTX_TOOLKIT_API static std::unique_ptr<GeometricPrimitive> __cdecl CreateCylinder(float height = 1, float diameter = 1, size_t tessellation = 32, bool rhcoords = true, _In_opt_ ID3D12Device* device = nullptr);
            DIRECTX_TOOLKIT_API static std::unique_ptr<GeometricPrimitive> __cdecl CreateCone(float diameter = 1, float height = 1, size_t tessellation = 32, bool rhcoords = true, _In_opt_ ID3D12Device* device = nullptr);
            DIRECTX_TOOLKIT_API static std::unique_ptr<GeometricPrimitive> __cdecl CreateTorus(float diameter = 1, float thickness = 0.333f, size_t tessellation = 32, bool rhcoords = true, _In_opt_ ID3D12Device* device = nullptr);
            DIRECTX_TOOLKIT_API static std::unique_ptr<GeometricPrimitive> __cdecl CreateTetrahedron(float size = 1, bool rhcoords = true, _In_opt_ ID3D12Device* device = nullptr);
            DIRECTX_TOOLKIT_API static std::unique_ptr<GeometricPrimitive> __cdecl CreateOctahedron(float size = 1, bool rhcoords = true, _In_opt_ ID3D12Device* device = nullptr);
            DIRECTX_TOOLKIT_API static std::unique_ptr<GeometricPrimitive> __cdecl CreateDodecahedron(float size = 1, bool rhcoords = true, _In_opt_ ID3D12Device* device = nullptr);
            DIRECTX_TOOLKIT_API static std::unique_ptr<GeometricPrimitive> __cdecl CreateIcosahedron(float size = 1, bool rhcoords = true, _In_opt_ ID3D12Device* device = nullptr);
            DIRECTX_TOOLKIT_API static std::unique_ptr<GeometricPrimitive> __cdecl CreateTeapot(float size = 1, size_t tessellation = 8, bool rhcoords = true, _In_opt_ ID3D12Device* device = nullptr);
            DIRECTX_TOOLKIT_API static std::unique_ptr<GeometricPrimitive> __cdecl CreateCustom(const VertexCollection& vertices, const IndexCollection& indices, _In_opt_ ID3D12Device* device = nullptr);

            DIRECTX_TOOLKIT_API static void __cdecl CreateCube(VertexCollection& vertices, IndexCollection& indices, float size = 1, bool rhcoords = true);
            DIRECTX_TOOLKIT_API static void __cdecl CreateBox(VertexCollection& vertices, IndexCollection& indices, const XMFLOAT3& size, bool rhcoords = true, bool invertn = false);
            DIRECTX_TOOLKIT_API static void __cdecl CreateSphere(VertexCollection& vertices, IndexCollection& indices, float diameter = 1, size_t tessellation = 16, bool rhcoords = true, bool invertn = false);
            DIRECTX_TOOLKIT_API static void __cdecl CreateGeoSphere(VertexCollection& vertices, IndexCollection& indices, float diameter = 1, size_t tessellation = 3, bool rhcoords = true);
            DIRECTX_TOOLKIT_API static void __cdecl CreateCylinder(VertexCollection& vertices, IndexCollection& indices, float height = 1, float diameter = 1, size_t tessellation = 32, bool rhcoords = true);
            DIRECTX_TOOLKIT_API static void __cdecl CreateCone(VertexCollection& vertices, IndexCollection& indices, float diameter = 1, float height = 1, size_t tessellation = 32, bool rhcoords = true);
            DIRECTX_TOOLKIT_API static void __cdecl CreateTorus(VertexCollection& vertices, IndexCollection& indices, float diameter = 1, float thickness = 0.333f, size_t tessellation = 32, bool rhcoords = true);
            DIRECTX_TOOLKIT_API static void __cdecl CreateTetrahedron(VertexCollection& vertices, IndexCollection& indices, float size = 1, bool rhcoords = true);
            DIRECTX_TOOLKIT_API static void __cdecl CreateOctahedron(VertexCollection& vertices, IndexCollection& indices, float size = 1, bool rhcoords = true);
            DIRECTX_TOOLKIT_API static void __cdecl CreateDodecahedron(VertexCollection& vertices, IndexCollection& indices, float size = 1, bool rhcoords = true);
            DIRECTX_TOOLKIT_API static void __cdecl CreateIcosahedron(VertexCollection& vertices, IndexCollection& indices, float size = 1, bool rhcoords = true);
            DIRECTX_TOOLKIT_API static void __cdecl CreateTeapot(VertexCollection& vertices, IndexCollection& indices, float size = 1, size_t tessellation = 8, bool rhcoords = true);

            // Load VB/IB resources for static geometry.
            DIRECTX_TOOLKIT_API void __cdecl LoadStaticBuffers(
                _In_ ID3D12Device* device,
                ResourceUploadBatch& resourceUploadBatch);

            // Transition VB/IB resources for static geometry.
            DIRECTX_TOOLKIT_API void __cdecl Transition(
                _In_ ID3D12GraphicsCommandList* commandList,
                D3D12_RESOURCE_STATES stateBeforeVB,
                D3D12_RESOURCE_STATES stateAfterVB,
                D3D12_RESOURCE_STATES stateBeforeIB,
                D3D12_RESOURCE_STATES stateAfterIB);

            // Draw the primitive.
            DIRECTX_TOOLKIT_API void __cdecl Draw(_In_ ID3D12GraphicsCommandList* commandList) const;

            DIRECTX_TOOLKIT_API void __cdecl DrawInstanced(
                _In_ ID3D12GraphicsCommandList* commandList,
                uint32_t instanceCount,
                uint32_t startInstanceLocation = 0) const;

        private:
            DIRECTX_TOOLKIT_API GeometricPrimitive() noexcept(false);

            // Private implementation.
            class Impl;

            std::unique_ptr<Impl> pImpl;
        };
    }
}
