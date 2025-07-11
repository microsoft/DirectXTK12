//--------------------------------------------------------------------------------------
// File: PrimitiveBatch.h
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#pragma once

#ifdef _GAMING_XBOX_SCARLETT
#include <d3d12_xs.h>
#elif (defined(_XBOX_ONE) && defined(_TITLE)) || defined(_GAMING_XBOX)
#include <d3d12_x.h>
#elif defined(USING_DIRECTX_HEADERS)
#include <directx/d3d12.h>
#include <dxguids/dxguids.h>
#else
#include <d3d12.h>
#endif

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <utility>

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
    inline namespace DX12
    {
        namespace Private
        {
            // Base class, not to be used directly: clients should access this via the derived PrimitiveBatch<T>.
            class PrimitiveBatchBase
            {
            protected:
                DIRECTX_TOOLKIT_API PrimitiveBatchBase(_In_ ID3D12Device* device, size_t maxIndices, size_t maxVertices, size_t vertexSize);

                DIRECTX_TOOLKIT_API PrimitiveBatchBase(PrimitiveBatchBase&&) noexcept;
                DIRECTX_TOOLKIT_API PrimitiveBatchBase& operator= (PrimitiveBatchBase&&) noexcept;

                PrimitiveBatchBase(PrimitiveBatchBase const&) = delete;
                PrimitiveBatchBase& operator= (PrimitiveBatchBase const&) = delete;

                DIRECTX_TOOLKIT_API virtual ~PrimitiveBatchBase();

            public:
                // Begin/End a batch of primitive drawing operations.
                DIRECTX_TOOLKIT_API void __cdecl Begin(_In_ ID3D12GraphicsCommandList* cmdList);
                DIRECTX_TOOLKIT_API void __cdecl End();

            protected:
                // Internal, untyped drawing method.
                DIRECTX_TOOLKIT_API void __cdecl Draw(
                    D3D_PRIMITIVE_TOPOLOGY topology,
                    bool isIndexed, _In_opt_count_(indexCount) uint16_t const* indices, size_t indexCount,
                    size_t vertexCount,
                    _Outptr_ void** pMappedVertices);

            private:
                // Private implementation.
                class Impl;

                std::unique_ptr<Impl> pImpl;
            };
        }

        // Template makes the API typesafe, eg. PrimitiveBatch<VertexPositionColor>.
        template<typename TVertex>
        class PrimitiveBatch : public Private::PrimitiveBatchBase
        {
            static constexpr size_t DefaultBatchSize = 4096;

        public:
            explicit PrimitiveBatch(
                _In_ ID3D12Device* device,
                size_t maxIndices = DefaultBatchSize * 3,
                size_t maxVertices = DefaultBatchSize)
                : PrimitiveBatchBase(device, maxIndices, maxVertices, sizeof(TVertex))
            {}

            PrimitiveBatch(PrimitiveBatch&&) = default;
            PrimitiveBatch& operator= (PrimitiveBatch&&) = default;

            PrimitiveBatch(PrimitiveBatch const&) = delete;
            PrimitiveBatch& operator= (PrimitiveBatch const&) = delete;

            // Similar to the D3D9 API DrawPrimitiveUP.
            void Draw(
                D3D_PRIMITIVE_TOPOLOGY topology,
                _In_reads_(vertexCount) TVertex const* vertices, size_t vertexCount)
            {
                void* mappedVertices;

                PrimitiveBatchBase::Draw(topology, false, nullptr, 0, vertexCount, &mappedVertices);

                memcpy(mappedVertices, vertices, vertexCount * sizeof(TVertex));
            }


            // Similar to the D3D9 API DrawIndexedPrimitiveUP.
            void DrawIndexed(
                D3D_PRIMITIVE_TOPOLOGY topology,
                _In_reads_(indexCount) uint16_t const* indices, size_t indexCount,
                _In_reads_(vertexCount) TVertex const* vertices, size_t vertexCount)
            {
                void* mappedVertices;

                PrimitiveBatchBase::Draw(topology, true, indices, indexCount, vertexCount, &mappedVertices);

                memcpy(mappedVertices, vertices, vertexCount * sizeof(TVertex));
            }


            void DrawLine(
                TVertex const& v1,
                TVertex const& v2)
            {
                TVertex* mappedVertices;

                PrimitiveBatchBase::Draw(D3D_PRIMITIVE_TOPOLOGY_LINELIST, false, nullptr, 0, 2, reinterpret_cast<void**>(&mappedVertices));

                mappedVertices[0] = v1;
                mappedVertices[1] = v2;
            }


            void DrawTriangle(
                TVertex const& v1,
                TVertex const& v2,
                TVertex const& v3)
            {
                TVertex* mappedVertices;

                PrimitiveBatchBase::Draw(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, false, nullptr, 0, 3, reinterpret_cast<void**>(&mappedVertices));

                mappedVertices[0] = v1;
                mappedVertices[1] = v2;
                mappedVertices[2] = v3;
            }


            void DrawQuad(
                TVertex const& v1,
                TVertex const& v2,
                TVertex const& v3,
                TVertex const& v4)
            {
                static const uint16_t quadIndices[] = { 0, 1, 2, 0, 2, 3 };

                TVertex* mappedVertices;

                PrimitiveBatchBase::Draw(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, true, quadIndices, 6, 4, reinterpret_cast<void**>(&mappedVertices));

                mappedVertices[0] = v1;
                mappedVertices[1] = v2;
                mappedVertices[2] = v3;
                mappedVertices[3] = v4;
            }
        };
    }
}
