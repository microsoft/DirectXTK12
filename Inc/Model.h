//--------------------------------------------------------------------------------------
// File: Model.h
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

#pragma once

#if defined(_XBOX_ONE) && defined(_TITLE)
#include <d3d12_x.h>
#else
#include <d3d12.h>
#endif

#include <DirectXMath.h>
#include <DirectXCollision.h>

#include <memory>
#include <functional>
#include <set>
#include <string>
#include <vector>
#include <stdint.h>

#include <wrl\client.h>

#include "GraphicsMemory.h"


namespace DirectX
{
    class IEffect;
    class IEffectFactory;
    class CommonStates;
    class ModelMesh;
    class DescriptorHeap;

    //----------------------------------------------------------------------------------
    // Each mesh part is a submesh with a single effect
    class ModelMeshPart
    {
    public:
        ModelMeshPart();
        virtual ~ModelMeshPart();

        uint32_t                                                indexCount;
        uint32_t                                                startIndex;
        uint32_t                                                vertexOffset;
        uint32_t                                                vertexStride;
        D3D_PRIMITIVE_TOPOLOGY                                  primitiveType;
        DXGI_FORMAT                                             indexFormat;
        GraphicsResource                                        indexBuffer;
        GraphicsResource                                        vertexBuffer;
        std::shared_ptr<IEffect>                                effect;
        std::shared_ptr<std::vector<D3D12_INPUT_ELEMENT_DESC>>  vbDecl;
        bool                                                    isAlpha;

        typedef std::vector<std::unique_ptr<ModelMeshPart>> Collection;

        // Draw mesh part with custom effect
        void __cdecl Draw(_In_ ID3D12GraphicsCommandList* commandList, IEffect* ieffect) const;
    };


    //----------------------------------------------------------------------------------
    // A mesh consists of one or more model mesh parts
    class ModelMesh
    {
    public:
        ModelMesh();
        virtual ~ModelMesh();

        BoundingSphere              boundingSphere;
        BoundingBox                 boundingBox;
        ModelMeshPart::Collection   meshParts;
        std::wstring                name;
        bool                        ccw;
        bool                        pmalpha;

        typedef std::vector<std::shared_ptr<ModelMesh>> Collection;

        // Draw the mesh
        void XM_CALLCONV Draw(_In_ ID3D12GraphicsCommandList* commandList, FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection,
                              bool alpha = false) const;
    };


    //----------------------------------------------------------------------------------
    // A model consists of one or more meshes
    class Model
    {
    public:
        Model();
        virtual ~Model();

        // Draw all the meshes in the model
        void XM_CALLCONV Draw(_In_ ID3D12GraphicsCommandList* commandList, FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection) const;

        // Notify model that effects, parts list, or mesh list has changed
        void __cdecl Modified() { mEffectCache.clear(); }

        // Update all effects used by the model
        void __cdecl UpdateEffects( _In_ std::function<void __cdecl(IEffect*)> setEffect );

        // Loads a model from a DirectX SDK .SDKMESH file
        static std::unique_ptr<Model> __cdecl CreateFromSDKMESH( _In_ ID3D12Device* d3dDevice, _In_reads_bytes_(dataSize) const uint8_t* meshData, _In_ size_t dataSize,
                                                                 _In_ IEffectFactory& fxFactory, bool ccw = false, bool pmalpha = false );
        static std::unique_ptr<Model> __cdecl CreateFromSDKMESH( _In_ ID3D12Device* d3dDevice, _In_z_ const wchar_t* szFileName,
                                                                 _In_ IEffectFactory& fxFactory, bool ccw = false, bool pmalpha = false );

        // Loads a model from a .VBO file
        static std::unique_ptr<Model> __cdecl CreateFromVBO( _In_ ID3D12Device* d3dDevice, _In_reads_bytes_(dataSize) const uint8_t* meshData, _In_ size_t dataSize,
                                                             _In_opt_ std::shared_ptr<IEffect> ieffect = nullptr, bool ccw = false, bool pmalpha = false );
        static std::unique_ptr<Model> __cdecl CreateFromVBO( _In_ ID3D12Device* d3dDevice, _In_z_ const wchar_t* szFileName, 
                                                             _In_opt_ std::shared_ptr<IEffect> ieffect = nullptr, bool ccw = false, bool pmalpha = false );

        ModelMesh::Collection   meshes;
        std::wstring            name;
 
    private:
        std::set<IEffect*>  mEffectCache;
    };
 }