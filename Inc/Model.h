//--------------------------------------------------------------------------------------
// File: Model.h
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
#include <directx/dxgiformat.h>
#include <dxguids/dxguids.h>
#else
#include <d3d12.h>
#include <dxgiformat.h>
#endif

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <malloc.h>

#include <wrl/client.h>

#include <DirectXMath.h>
#include <DirectXCollision.h>

#include "GraphicsMemory.h"
#include "Effects.h"


namespace DirectX
{
    inline namespace DX12
    {
        class IEffect;
        class IEffectFactory;
        class ModelMesh;

        //------------------------------------------------------------------------------
        // Model loading options
        enum ModelLoaderFlags : uint32_t
        {
            ModelLoader_Default = 0x0,
            ModelLoader_MaterialColorsSRGB = 0x1,
            ModelLoader_AllowLargeModels = 0x2,
            ModelLoader_IncludeBones = 0x4,
            ModelLoader_DisableSkinning = 0x8,
        };

        //------------------------------------------------------------------------------
        // Frame hierarchy for rigid body and skeletal animation
        struct ModelBone
        {
            ModelBone() noexcept :
                parentIndex(c_Invalid),
                childIndex(c_Invalid),
                siblingIndex(c_Invalid)
            {
            }

            ModelBone(uint32_t parent, uint32_t child, uint32_t sibling) noexcept :
                parentIndex(parent),
                childIndex(child),
                siblingIndex(sibling)
            {
            }

            uint32_t            parentIndex;
            uint32_t            childIndex;
            uint32_t            siblingIndex;
            std::wstring        name;

            using Collection = std::vector<ModelBone>;

            static constexpr uint32_t c_Invalid = uint32_t(-1);

            struct aligned_deleter { void operator()(void* p) noexcept { _aligned_free(p); } };

            using TransformArray = std::unique_ptr<XMMATRIX[], aligned_deleter>;

            static TransformArray MakeArray(size_t count)
            {
                void* temp = _aligned_malloc(sizeof(XMMATRIX) * count, 16);
                if (!temp)
                    throw std::bad_alloc();
                return TransformArray(static_cast<XMMATRIX*>(temp));
            }
        };

        //------------------------------------------------------------------------------
        // Each mesh part is a submesh with a single effect
        class ModelMeshPart
        {
        public:
            ModelMeshPart(uint32_t partIndex) noexcept;

            ModelMeshPart(ModelMeshPart&&) = default;
            ModelMeshPart& operator= (ModelMeshPart&&) = default;

            ModelMeshPart(ModelMeshPart const&) = default;
            ModelMeshPart& operator= (ModelMeshPart const&) = default;

            virtual ~ModelMeshPart();

            using Collection = std::vector<std::unique_ptr<ModelMeshPart>>;
            using DrawCallback = std::function<void(_In_ ID3D12GraphicsCommandList* commandList, const ModelMeshPart& part)>;
            using InputLayoutCollection = std::vector<D3D12_INPUT_ELEMENT_DESC>;

            uint32_t                                                partIndex;      // Unique index assigned per-part in a model.
            uint32_t                                                materialIndex;  // Index of the material spec to use
            uint32_t                                                indexCount;
            uint32_t                                                startIndex;
            int32_t                                                 vertexOffset;
            uint32_t                                                vertexStride;
            uint32_t                                                vertexCount;
            uint32_t                                                indexBufferSize;
            uint32_t                                                vertexBufferSize;
            D3D_PRIMITIVE_TOPOLOGY                                  primitiveType;
            DXGI_FORMAT                                             indexFormat;
            SharedGraphicsResource                                  indexBuffer;
            SharedGraphicsResource                                  vertexBuffer;
            Microsoft::WRL::ComPtr<ID3D12Resource>                  staticIndexBuffer;
            Microsoft::WRL::ComPtr<ID3D12Resource>                  staticVertexBuffer;
            std::shared_ptr<InputLayoutCollection>                  vbDecl;

            // Draw mesh part
            void __cdecl Draw(_In_ ID3D12GraphicsCommandList* commandList) const;

            void __cdecl DrawInstanced(_In_ ID3D12GraphicsCommandList* commandList, uint32_t instanceCount, uint32_t startInstance = 0) const;

            //
            // Utilities for drawing multiple mesh parts
            //

            // Draw the mesh
            static void __cdecl DrawMeshParts(_In_ ID3D12GraphicsCommandList* commandList, const Collection& meshParts);

            // Draw the mesh with an effect
            static void __cdecl DrawMeshParts(_In_ ID3D12GraphicsCommandList* commandList, const Collection& meshParts, _In_ IEffect* effect);

            // Draw the mesh with a callback for each mesh part
            static void __cdecl DrawMeshParts(_In_ ID3D12GraphicsCommandList* commandList, const Collection& meshParts, DrawCallback callback);

            // Draw the mesh with a range of effects that mesh parts will index into.
            // Effects can be any IEffect pointer type (including smart pointer). Value or reference types will not compile.
            // The iterator passed to this method should have random access capabilities for best performance.
            template<typename TEffectIterator, typename TEffectIteratorCategory = typename TEffectIterator::iterator_category>
            static void DrawMeshParts(
                _In_ ID3D12GraphicsCommandList* commandList,
                const Collection& meshParts,
                TEffectIterator partEffects)
            {
                // This assert is here to prevent accidental use of containers that would cause undesirable performance penalties.
                static_assert(
                    std::is_base_of<std::random_access_iterator_tag, TEffectIteratorCategory>::value,
                    "Providing an iterator without random access capabilities -- such as from std::list -- is not supported.");

                for (const auto& it : meshParts)
                {
                    auto part = it.get();
                    assert(part != nullptr);

                    // Get the effect at the location specified by the part's material
                    TEffectIterator effect_iterator = partEffects;
                    std::advance(effect_iterator, part->partIndex);

                    // Apply the effect and draw
                    (*effect_iterator)->Apply(commandList);
                    part->Draw(commandList);
                }
            }

            template<typename TEffectIterator, typename TEffectIteratorCategory = typename TEffectIterator::iterator_category>
            static void XM_CALLCONV DrawMeshParts(
                _In_ ID3D12GraphicsCommandList* commandList,
                const Collection& meshParts,
                FXMMATRIX world,
                TEffectIterator partEffects)
            {
                // This assert is here to prevent accidental use of containers that would cause undesirable performance penalties.
                static_assert(
                    std::is_base_of<std::random_access_iterator_tag, TEffectIteratorCategory>::value,
                    "Providing an iterator without random access capabilities -- such as from std::list -- is not supported.");

                for (const auto& it : meshParts)
                {
                    auto part = it.get();
                    assert(part != nullptr);

                    // Get the effect at the location specified by the part's material
                    TEffectIterator effect_iterator = partEffects;
                    std::advance(effect_iterator, part->partIndex);

                    auto imatrices = dynamic_cast<IEffectMatrices*>((*effect_iterator).get());
                    if (imatrices)
                    {
                        imatrices->SetWorld(world);
                    }

                    // Apply the effect and draw
                    (*effect_iterator)->Apply(commandList);
                    part->Draw(commandList);
                }
            }

            template<typename TEffectIterator, typename TEffectIteratorCategory = typename TEffectIterator::iterator_category>
            static void XM_CALLCONV DrawSkinnedMeshParts(
                _In_ ID3D12GraphicsCommandList* commandList,
                const ModelMesh& mesh,
                const Collection& meshParts,
                size_t nbones,
                _In_reads_(nbones) const XMMATRIX* boneTransforms,
                FXMMATRIX world,
                TEffectIterator partEffects);
        };


        //------------------------------------------------------------------------------
        // A mesh consists of one or more model mesh parts
        class ModelMesh
        {
        public:
            ModelMesh() noexcept;

            ModelMesh(ModelMesh&&) = default;
            ModelMesh& operator= (ModelMesh&&) = default;

            ModelMesh(ModelMesh const&) = default;
            ModelMesh& operator= (ModelMesh const&) = default;

            virtual ~ModelMesh();

            BoundingSphere              boundingSphere;
            BoundingBox                 boundingBox;
            ModelMeshPart::Collection   opaqueMeshParts;
            ModelMeshPart::Collection   alphaMeshParts;
            uint32_t                    boneIndex;
            std::vector<uint32_t>       boneInfluences;
            std::wstring                name;

            using Collection = std::vector<std::shared_ptr<ModelMesh>>;

            // Draw the mesh
            void __cdecl DrawOpaque(_In_ ID3D12GraphicsCommandList* commandList) const;
            void __cdecl DrawAlpha(_In_ ID3D12GraphicsCommandList* commandList) const;

            // Draw the mesh with an effect
            void __cdecl DrawOpaque(_In_ ID3D12GraphicsCommandList* commandList, _In_ IEffect* effect) const;
            void __cdecl DrawAlpha(_In_ ID3D12GraphicsCommandList* commandList, _In_ IEffect* effect) const;

            // Draw the mesh with a callback for each mesh part
            void __cdecl DrawOpaque(_In_ ID3D12GraphicsCommandList* commandList, ModelMeshPart::DrawCallback callback) const;
            void __cdecl DrawAlpha(_In_ ID3D12GraphicsCommandList* commandList, ModelMeshPart::DrawCallback callback) const;

            // Draw the mesh with a range of effects that mesh parts will index into.
            // TEffectPtr can be any IEffect pointer type (including smart pointer). Value or reference types will not compile.
            template<typename TEffectIterator, typename TEffectIteratorCategory = typename TEffectIterator::iterator_category>
            void DrawOpaque(_In_ ID3D12GraphicsCommandList* commandList, TEffectIterator effects) const
            {
                ModelMeshPart::DrawMeshParts<TEffectIterator, TEffectIteratorCategory>(commandList, opaqueMeshParts, effects);
            }
            template<typename TEffectIterator, typename TEffectIteratorCategory = typename TEffectIterator::iterator_category>
            void DrawAlpha(_In_ ID3D12GraphicsCommandList* commandList, TEffectIterator effects) const
            {
                ModelMeshPart::DrawMeshParts<TEffectIterator, TEffectIteratorCategory>(commandList, alphaMeshParts, effects);
            }

            // Draw rigid-body with bones.
            template<typename TEffectIterator, typename TEffectIteratorCategory = typename TEffectIterator::iterator_category>
            void XM_CALLCONV DrawOpaque(
                _In_ ID3D12GraphicsCommandList* commandList,
                size_t nbones,
                _In_reads_(nbones) const XMMATRIX* boneTransforms,
                FXMMATRIX world,
                TEffectIterator effects) const
            {
                assert(nbones > 0 && boneTransforms != nullptr);
                XMMATRIX local;
                if (boneIndex != ModelBone::c_Invalid && boneIndex < nbones)
                {
                    local = XMMatrixMultiply(boneTransforms[boneIndex], world);
                }
                else
                {
                    local = world;
                }

                ModelMeshPart::DrawMeshParts<TEffectIterator, TEffectIteratorCategory>(commandList, opaqueMeshParts, local, effects);
            }

            template<typename TEffectIterator, typename TEffectIteratorCategory = typename TEffectIterator::iterator_category>
            void XM_CALLCONV DrawAlpha(
                _In_ ID3D12GraphicsCommandList* commandList,
                size_t nbones,
                _In_reads_(nbones) const XMMATRIX* boneTransforms,
                FXMMATRIX world,
                TEffectIterator effects) const
            {
                assert(nbones > 0 && boneTransforms != nullptr);
                XMMATRIX local;
                if (boneIndex != ModelBone::c_Invalid && boneIndex < nbones)
                {
                    local = XMMatrixMultiply(boneTransforms[boneIndex], world);
                }
                else
                {
                    local = world;
                }

                ModelMeshPart::DrawMeshParts<TEffectIterator, TEffectIteratorCategory>(commandList, alphaMeshParts, local, effects);
            }

            // Draw using skinning given bone transform array.
            template<typename TEffectIterator, typename TEffectIteratorCategory = typename TEffectIterator::iterator_category>
            void XM_CALLCONV DrawSkinnedOpaque(
                _In_ ID3D12GraphicsCommandList* commandList,
                size_t nbones,
                _In_reads_(nbones) const XMMATRIX* boneTransforms,
                FXMMATRIX world,
                TEffectIterator effects) const
            {
                ModelMeshPart::DrawSkinnedMeshParts<TEffectIterator, TEffectIteratorCategory>(commandList, *this, opaqueMeshParts,
                    nbones, boneTransforms, world, effects);
            }

            template<typename TEffectIterator, typename TEffectIteratorCategory = typename TEffectIterator::iterator_category>
            void XM_CALLCONV DrawSkinnedAlpha(
                _In_ ID3D12GraphicsCommandList* commandList,
                size_t nbones,
                _In_reads_(nbones) const XMMATRIX* boneTransforms,
                FXMMATRIX world,
                TEffectIterator effects) const
            {
                ModelMeshPart::DrawSkinnedMeshParts<TEffectIterator, TEffectIteratorCategory>(commandList, *this, alphaMeshParts,
                    nbones, boneTransforms, world, effects);
            }
        };


        //------------------------------------------------------------------------------
        // A model consists of one or more meshes
        class Model
        {
        public:
            Model() noexcept;

            Model(Model&&) = default;
            Model& operator= (Model&&) = default;

            Model(Model const& other);
            Model& operator= (Model const& rhs);

            virtual ~Model();

            using EffectCollection = std::vector<std::shared_ptr<IEffect>>;
            using ModelMaterialInfo = IEffectFactory::EffectInfo;
            using ModelMaterialInfoCollection = std::vector<ModelMaterialInfo>;
            using TextureCollection = std::vector<std::wstring>;

            // The Model::Draw* functions use variadic templates and perfect-forwarding in order to support future
            // overloads to the ModelMesh::Draw* family of functions. This means that a new ModelMesh overload can be
            // added, removed or altered, but the Model routines will still remain compatible. The correct ModelMesh
            // overload will be selected by the compiler depending on the arguments you provide to the Model method.

            // Draw all the meshes in the model.
            template<typename... TForwardArgs> void DrawOpaque(_In_ ID3D12GraphicsCommandList* commandList, TForwardArgs&&... args) const
            {
                // Draw opaque parts
                for (const auto& it : meshes)
                {
                    auto mesh = it.get();
                    assert(mesh != nullptr);

                    mesh->DrawOpaque(commandList, std::forward<TForwardArgs>(args)...);
                }
            }

            template<typename... TForwardArgs> void DrawAlpha(_In_ ID3D12GraphicsCommandList* commandList, TForwardArgs&&... args) const
            {
                // Draw alpha parts
                for (const auto& it : meshes)
                {
                    auto mesh = it.get();
                    assert(mesh != nullptr);

                    mesh->DrawAlpha(commandList, std::forward<TForwardArgs>(args)...);
                }
            }

            template<typename... TForwardArgs> void Draw(_In_ ID3D12GraphicsCommandList* commandList, TForwardArgs&&... args) const
            {
                DrawOpaque(commandList, args...);
                DrawAlpha(commandList, std::forward<TForwardArgs>(args)...);
            }

            // Draw mesh using skinning given bone transform array.
            template<typename... TForwardArgs> void DrawSkinnedOpaque(_In_ ID3D12GraphicsCommandList* commandList, TForwardArgs&&... args) const
            {
                // Draw opaque parts
                for (const auto& it : meshes)
                {
                    auto mesh = it.get();
                    assert(mesh != nullptr);

                    mesh->DrawSkinnedOpaque(commandList, std::forward<TForwardArgs>(args)...);
                }
            }

            template<typename... TForwardArgs> void DrawSkinnedAlpha(_In_ ID3D12GraphicsCommandList* commandList, TForwardArgs&&... args) const
            {
                // Draw alpha parts
                for (const auto& it : meshes)
                {
                    auto mesh = it.get();
                    assert(mesh != nullptr);

                    mesh->DrawSkinnedAlpha(commandList, std::forward<TForwardArgs>(args)...);
                }
            }

            template<typename... TForwardArgs> void DrawSkinned(_In_ ID3D12GraphicsCommandList* commandList, TForwardArgs&&... args) const
            {
                DrawSkinnedOpaque(commandList, args...);
                DrawSkinnedAlpha(commandList, std::forward<TForwardArgs>(args)...);
            }

            // Load texture resources into an existing Effect Texture Factory
            int __cdecl LoadTextures(IEffectTextureFactory& texFactory, int destinationDescriptorOffset = 0) const;

            // Load texture resources into a new Effect Texture Factory
            std::unique_ptr<EffectTextureFactory> __cdecl LoadTextures(
                _In_ ID3D12Device* device,
                ResourceUploadBatch& resourceUploadBatch,
                _In_opt_z_ const wchar_t* texturesPath = nullptr,
                D3D12_DESCRIPTOR_HEAP_FLAGS flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) const;

            // Load VB/IB resources for static geometry
            void __cdecl LoadStaticBuffers(
                _In_ ID3D12Device* device,
                ResourceUploadBatch& resourceUploadBatch,
                bool keepMemory = false);

            // Create effects using the default effect factory
            EffectCollection __cdecl CreateEffects(
                const EffectPipelineStateDescription& opaquePipelineState,
                const EffectPipelineStateDescription& alphaPipelineState,
                _In_ ID3D12DescriptorHeap* textureDescriptorHeap,
                _In_ ID3D12DescriptorHeap* samplerDescriptorHeap,
                int textureDescriptorOffset = 0,
                int samplerDescriptorOffset = 0) const;

            // Create effects using a custom effect factory
            EffectCollection __cdecl CreateEffects(
                IEffectFactory& fxFactory,
                const EffectPipelineStateDescription& opaquePipelineState,
                const EffectPipelineStateDescription& alphaPipelineState,
                int textureDescriptorOffset = 0,
                int samplerDescriptorOffset = 0) const;

            // Compute bone positions based on heirarchy and transform matrices
            void __cdecl CopyAbsoluteBoneTransformsTo(
                size_t nbones,
                _Out_writes_(nbones) XMMATRIX* boneTransforms) const;

            void __cdecl CopyAbsoluteBoneTransforms(
                size_t nbones,
                _In_reads_(nbones) const XMMATRIX* inBoneTransforms,
                _Out_writes_(nbones) XMMATRIX* outBoneTransforms) const;

            // Set bone matrices to a set of relative tansforms
            void __cdecl CopyBoneTransformsFrom(
                size_t nbones,
                _In_reads_(nbones) const XMMATRIX* boneTransforms);

            // Copies the relative bone matrices to a transform array
            void __cdecl CopyBoneTransformsTo(
                size_t nbones,
                _Out_writes_(nbones) XMMATRIX* boneTransforms) const;

            // Loads a model from a Visual Studio Starter Kit .CMO file
            static std::unique_ptr<Model> __cdecl CreateFromCMO(
                _In_opt_ ID3D12Device* device,
                _In_reads_bytes_(dataSize) const uint8_t* meshData, _In_ size_t dataSize,
                ModelLoaderFlags flags = ModelLoader_Default,
                _Out_opt_ size_t* animsOffset = nullptr);
            static std::unique_ptr<Model> __cdecl CreateFromCMO(
                _In_opt_ ID3D12Device* device,
                _In_z_ const wchar_t* szFileName,
                ModelLoaderFlags flags = ModelLoader_Default,
                _Out_opt_ size_t* animsOffset = nullptr);

            // Loads a model from a DirectX SDK .SDKMESH file
            static std::unique_ptr<Model> __cdecl CreateFromSDKMESH(
                _In_opt_ ID3D12Device* device,
                _In_reads_bytes_(dataSize) const uint8_t* meshData, _In_ size_t dataSize,
                ModelLoaderFlags flags = ModelLoader_Default);
            static std::unique_ptr<Model> __cdecl CreateFromSDKMESH(
                _In_opt_ ID3D12Device* device,
                _In_z_ const wchar_t* szFileName,
                ModelLoaderFlags flags = ModelLoader_Default);

            // Loads a model from a .VBO file
            static std::unique_ptr<Model> __cdecl CreateFromVBO(
                _In_opt_ ID3D12Device* device,
                _In_reads_bytes_(dataSize) const uint8_t* meshData, _In_ size_t dataSize,
                ModelLoaderFlags flags = ModelLoader_Default);
            static std::unique_ptr<Model> __cdecl CreateFromVBO(
                _In_opt_ ID3D12Device* device,
                _In_z_ const wchar_t* szFileName,
                ModelLoaderFlags flags = ModelLoader_Default);

            // Utility function for getting a GPU descriptor for a mesh part/material index. If there is no texture the
            // descriptor will be zero.
            D3D12_GPU_DESCRIPTOR_HANDLE __cdecl GetGpuTextureHandleForMaterialIndex(uint32_t materialIndex, _In_ ID3D12DescriptorHeap* heap, _In_ size_t descriptorSize, _In_ size_t descriptorOffset) const
            {
                D3D12_GPU_DESCRIPTOR_HANDLE handle = {};

                if (materialIndex >= materials.size())
                    return handle;

                const int textureIndex = materials[materialIndex].diffuseTextureIndex;
                if (textureIndex == -1)
                    return handle;

            #if defined(_MSC_VER) || !defined(_WIN32)
                handle = heap->GetGPUDescriptorHandleForHeapStart();
            #else
                std::ignore = heap->GetGPUDescriptorHandleForHeapStart(&handle);
            #endif
                handle.ptr += static_cast<UINT64>(descriptorSize * (UINT64(textureIndex) + UINT64(descriptorOffset)));

                return handle;
            }

            // Utility function for updating the matrices in a list of effects. This will SetWorld, SetView and SetProjection
            // on any effect in the list that derives from IEffectMatrices.
            static void XM_CALLCONV UpdateEffectMatrices(
                EffectCollection& effects,
                FXMMATRIX world,
                CXMMATRIX view,
                CXMMATRIX proj);

            // Utility function to transition VB/IB resources for static geometry.
            void __cdecl Transition(
                _In_ ID3D12GraphicsCommandList* commandList,
                D3D12_RESOURCE_STATES stateBeforeVB,
                D3D12_RESOURCE_STATES stateAfterVB,
                D3D12_RESOURCE_STATES stateBeforeIB,
                D3D12_RESOURCE_STATES stateAfterIB);

            ModelMesh::Collection           meshes;
            ModelMaterialInfoCollection     materials;
            TextureCollection               textureNames;
            ModelBone::Collection           bones;
            ModelBone::TransformArray       boneMatrices;
            ModelBone::TransformArray       invBindPoseMatrices;
            std::wstring                    name;

        private:
            std::shared_ptr<IEffect> __cdecl CreateEffectForMeshPart(
                IEffectFactory& fxFactory,
                const EffectPipelineStateDescription& opaquePipelineState,
                const EffectPipelineStateDescription& alphaPipelineState,
                int textureDescriptorOffset,
                int samplerDescriptorOffset,
                _In_ const ModelMeshPart* part) const;

            void __cdecl ComputeAbsolute(uint32_t index,
                CXMMATRIX local, size_t nbones,
                _In_reads_(nbones) const XMMATRIX* inBoneTransforms,
                _Inout_updates_(nbones) XMMATRIX* outBoneTransforms,
                size_t& visited) const;
        };


        template<typename TEffectIterator, typename TEffectIteratorCategory>
        void XM_CALLCONV ModelMeshPart::DrawSkinnedMeshParts(
            _In_ ID3D12GraphicsCommandList* commandList,
            const ModelMesh& mesh,
            const ModelMeshPart::Collection& meshParts,
            size_t nbones,
            _In_reads_(nbones) const XMMATRIX* boneTransforms,
            FXMMATRIX world,
            TEffectIterator partEffects)
        {
            // This assert is here to prevent accidental use of containers that would cause undesirable performance penalties.
            static_assert(
                std::is_base_of<std::random_access_iterator_tag, TEffectIteratorCategory>::value,
                "Providing an iterator without random access capabilities -- such as from std::list -- is not supported.");

            assert(nbones > 0 && boneTransforms != nullptr);

            ModelBone::TransformArray temp;

            for (const auto& mit : meshParts)
            {
                auto part = mit.get();
                assert(part != nullptr);

                // Get the effect at the location specified by the part's material
                TEffectIterator effect_iterator = partEffects;
                std::advance(effect_iterator, part->partIndex);

                auto imatrices = dynamic_cast<IEffectMatrices*>((*effect_iterator).get());
                if (imatrices)
                {
                    imatrices->SetWorld(world);
                }

                auto iskinning = dynamic_cast<IEffectSkinning*>((*effect_iterator).get());
                if (iskinning)
                {
                    if (mesh.boneInfluences.empty())
                    {
                        // Direct-mapping of vertex bone indices to our master bone array
                        iskinning->SetBoneTransforms(boneTransforms, nbones);
                    }
                    else
                    {
                        if (!temp)
                        {
                            // Create the influence mapped bones on-demand.
                            temp = ModelBone::MakeArray(IEffectSkinning::MaxBones);

                            size_t count = 0;
                            for (auto it : mesh.boneInfluences)
                            {
                                ++count;
                                if (count > IEffectSkinning::MaxBones)
                                {
                                    throw std::runtime_error("Too many bones for skinning");
                                }

                                if (it >= nbones)
                                {
                                    throw std::runtime_error("Invalid bone influence index");
                                }

                                temp[count - 1] = boneTransforms[it];
                            }

                            assert(count == mesh.boneInfluences.size());
                        }

                        iskinning->SetBoneTransforms(temp.get(), mesh.boneInfluences.size());
                    }
                }
                else if (imatrices)
                {
                    // Fallback for if we encounter a non-skinning effect in the model
                    XMMATRIX bm = (mesh.boneIndex != ModelBone::c_Invalid && mesh.boneIndex < nbones)
                        ? boneTransforms[mesh.boneIndex] : XMMatrixIdentity();

                    imatrices->SetWorld(XMMatrixMultiply(bm, world));
                }

                // Apply the effect and draw
                (*effect_iterator)->Apply(commandList);
                part->Draw(commandList);
            }
        }

    #ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdeprecated-dynamic-exception-spec"
    #endif

        DEFINE_ENUM_FLAG_OPERATORS(ModelLoaderFlags);

    #ifdef __clang__
    #pragma clang diagnostic pop
    #endif
    }
}
