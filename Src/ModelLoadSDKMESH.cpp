//--------------------------------------------------------------------------------------
// File: ModelLoadSDKMESH.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Model.h"

#include "Effects.h"
#include "VertexTypes.h"

#include "DirectXHelpers.h"
#include "PlatformHelpers.h"
#include "BinaryReader.h"
#include "DescriptorHeap.h"
#include "CommonStates.h"

#include "SDKMesh.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace
{
    enum : unsigned int
    {
        PER_VERTEX_COLOR = 0x1,
        SKINNING = 0x2,
        DUAL_TEXTURE = 0x4,
        NORMAL_MAPS = 0x8,
        BIASED_VERTEX_NORMALS = 0x10,
        USES_OBSOLETE_DEC3N = 0x20,
    };

    int GetUniqueTextureIndex(const wchar_t* textureName, std::map<std::wstring, int>& textureDictionary)
    {
        if (textureName == nullptr || !textureName[0])
            return -1;

        auto i = textureDictionary.find(textureName);
        if (i == std::cend(textureDictionary))
        {
            int index = static_cast<int>(textureDictionary.size());
            textureDictionary[textureName] = index;
            return index;
        }
        else
        {
            return i->second;
        }
    }

    inline XMFLOAT3 GetMaterialColor(float r, float g, float b, bool srgb) noexcept
    {
        if (srgb)
        {
            XMVECTOR v = XMVectorSet(r, g, b, 1.f);
            v = XMColorSRGBToRGB(v);

            XMFLOAT3 result;
            XMStoreFloat3(&result, v);
            return result;
        }
        else
        {
            return XMFLOAT3(r, g, b);
        }
    }

    template<size_t sizeOfBuffer>
    inline void ASCIIToWChar(wchar_t(&buffer)[sizeOfBuffer], const char *ascii)
    {
        MultiByteToWideChar(CP_UTF8, 0, ascii, -1, buffer, sizeOfBuffer);
    }

    void InitMaterial(
        const DXUT::SDKMESH_MATERIAL& mh,
        unsigned int flags,
        _Out_ Model::ModelMaterialInfo& m,
        _Inout_ std::map<std::wstring, int32_t>& textureDictionary,
        bool srgb)
    {
        wchar_t matName[DXUT::MAX_MATERIAL_NAME] = {};
        ASCIIToWChar(matName, mh.Name);

        wchar_t diffuseName[DXUT::MAX_TEXTURE_NAME] = {};
        ASCIIToWChar(diffuseName, mh.DiffuseTexture);

        wchar_t specularName[DXUT::MAX_TEXTURE_NAME] = {};
        ASCIIToWChar(specularName, mh.SpecularTexture);

        wchar_t normalName[DXUT::MAX_TEXTURE_NAME] = {};
        ASCIIToWChar(normalName, mh.NormalTexture);

        if ((flags & DUAL_TEXTURE) && !mh.SpecularTexture[0])
        {
            DebugTrace("WARNING: Material '%s' has multiple texture coords but not multiple textures\n", mh.Name);
            flags &= ~static_cast<unsigned int>(DUAL_TEXTURE);
        }

        if (mh.NormalTexture[0])
        {
            flags |= NORMAL_MAPS;
        }

        m = {};
        m.name = matName;
        m.perVertexColor = (flags & PER_VERTEX_COLOR) != 0;
        m.enableSkinning = (flags & SKINNING) != 0;
        m.enableDualTexture = (flags & DUAL_TEXTURE) != 0;
        m.enableNormalMaps = (flags & NORMAL_MAPS) != 0;
        m.biasedVertexNormals = (flags & BIASED_VERTEX_NORMALS) != 0;

        if (mh.Ambient.x == 0 && mh.Ambient.y == 0 && mh.Ambient.z == 0 && mh.Ambient.w == 0
            && mh.Diffuse.x == 0 && mh.Diffuse.y == 0 && mh.Diffuse.z == 0 && mh.Diffuse.w == 0)
        {
            // SDKMESH material color block is uninitalized; assume defaults
            m.diffuseColor = XMFLOAT3(1.f, 1.f, 1.f);
            m.alphaValue = 1.f;
        }
        else
        {
            m.ambientColor = GetMaterialColor(mh.Ambient.x, mh.Ambient.y, mh.Ambient.z, srgb);
            m.diffuseColor = GetMaterialColor(mh.Diffuse.x, mh.Diffuse.y, mh.Diffuse.z, srgb);
            m.emissiveColor = GetMaterialColor(mh.Emissive.x, mh.Emissive.y, mh.Emissive.z, srgb);

            if (mh.Diffuse.w != 1.f && mh.Diffuse.w != 0.f)
            {
                m.alphaValue = mh.Diffuse.w;
            }
            else
                m.alphaValue = 1.f;

            if (mh.Power > 0)
            {
                m.specularPower = mh.Power;
                m.specularColor = XMFLOAT3(mh.Specular.x, mh.Specular.y, mh.Specular.z);
            }
        }

        m.diffuseTextureIndex = GetUniqueTextureIndex(diffuseName, textureDictionary);
        m.specularTextureIndex = GetUniqueTextureIndex(specularName, textureDictionary);
        m.normalTextureIndex = GetUniqueTextureIndex(normalName, textureDictionary);

        m.samplerIndex = (m.diffuseTextureIndex == -1) ? -1 : static_cast<int>(CommonStates::SamplerIndex::AnisotropicWrap);
        m.samplerIndex2 = (flags & DUAL_TEXTURE) ? static_cast<int>(CommonStates::SamplerIndex::AnisotropicWrap) : -1;
    }

    void InitMaterial(
        const DXUT::SDKMESH_MATERIAL_V2& mh,
        unsigned int flags,
        _Out_ Model::ModelMaterialInfo& m,
        _Inout_ std::map<std::wstring, int>& textureDictionary)
    {
        wchar_t matName[DXUT::MAX_MATERIAL_NAME] = {};
        ASCIIToWChar(matName, mh.Name);

        wchar_t albedoTexture[DXUT::MAX_TEXTURE_NAME] = {};
        ASCIIToWChar(albedoTexture, mh.AlbedoTexture);

        wchar_t normalName[DXUT::MAX_TEXTURE_NAME] = {};
        ASCIIToWChar(normalName, mh.NormalTexture);

        wchar_t rmaName[DXUT::MAX_TEXTURE_NAME] = {};
        ASCIIToWChar(rmaName, mh.RMATexture);

        wchar_t emissiveName[DXUT::MAX_TEXTURE_NAME] = {};
        ASCIIToWChar(emissiveName, mh.EmissiveTexture);

        m = {};
        m.name = matName;
        m.perVertexColor = false;
        m.enableSkinning = (flags & SKINNING) != 0;
        m.enableDualTexture = false;
        m.enableNormalMaps = true;
        m.biasedVertexNormals = (flags & BIASED_VERTEX_NORMALS) != 0;
        m.alphaValue = (mh.Alpha == 0.f) ? 1.f : mh.Alpha;

        m.diffuseTextureIndex = GetUniqueTextureIndex(albedoTexture, textureDictionary);
        m.specularTextureIndex = GetUniqueTextureIndex(rmaName, textureDictionary);
        m.normalTextureIndex = GetUniqueTextureIndex(normalName, textureDictionary);
        m.emissiveTextureIndex = GetUniqueTextureIndex(emissiveName, textureDictionary);

        m.samplerIndex = m.samplerIndex2 = static_cast<int>(CommonStates::SamplerIndex::AnisotropicWrap);
    }


    //--------------------------------------------------------------------------------------
    // Direct3D 9 Vertex Declaration to Direct3D 12 Input Layout mapping

    static_assert(D3D12_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT >= 32, "SDKMESH supports decls up to 32 entries");

    unsigned int GetInputLayoutDesc(
        _In_reads_(32) const DXUT::D3DVERTEXELEMENT9 decl[],
        ModelMeshPart::InputLayoutCollection& inputDesc)
    {
        static const D3D12_INPUT_ELEMENT_DESC s_elements[] =
        {
           { "SV_Position",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
           { "NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
           { "COLOR",        0, DXGI_FORMAT_B8G8R8A8_UNORM,  0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
           { "TANGENT",      0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
           { "BINORMAL",     0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
           { "TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
           { "BLENDINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT,   0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
           { "BLENDWEIGHT",  0, DXGI_FORMAT_R8G8B8A8_UNORM,  0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        using namespace DXUT;

        uint32_t offset = 0;
        uint32_t texcoords = 0;
        unsigned int flags = 0;

        bool posfound = false;

        for (size_t index = 0; index < DXUT::MAX_VERTEX_ELEMENTS; ++index)
        {
            if (decl[index].Usage == 0xFF)
                break;

            if (decl[index].Type == D3DDECLTYPE_UNUSED)
                break;

            if (decl[index].Offset != offset)
                break;

            if (decl[index].Usage == D3DDECLUSAGE_POSITION)
            {
                if (decl[index].Type == D3DDECLTYPE_FLOAT3)
                {
                    inputDesc.push_back(s_elements[0]);
                    offset += 12;
                    posfound = true;
                }
                else
                    break;
            }
            else if (decl[index].Usage == D3DDECLUSAGE_NORMAL
                || decl[index].Usage == D3DDECLUSAGE_TANGENT
                || decl[index].Usage == D3DDECLUSAGE_BINORMAL)
            {
                size_t base = 1;
                if (decl[index].Usage == D3DDECLUSAGE_TANGENT)
                    base = 3;
                else if (decl[index].Usage == D3DDECLUSAGE_BINORMAL)
                    base = 4;

                D3D12_INPUT_ELEMENT_DESC desc = s_elements[base];

                bool unk = false;
                switch (decl[index].Type)
                {
                case D3DDECLTYPE_FLOAT3:                 assert(desc.Format == DXGI_FORMAT_R32G32B32_FLOAT); offset += 12; break;
                case D3DDECLTYPE_UBYTE4N:                desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; flags |= BIASED_VERTEX_NORMALS; offset += 4; break;
                case D3DDECLTYPE_SHORT4N:                desc.Format = DXGI_FORMAT_R16G16B16A16_SNORM; offset += 8; break;
                case D3DDECLTYPE_FLOAT16_4:              desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT; offset += 8; break;
                case D3DDECLTYPE_DXGI_R10G10B10A2_UNORM: desc.Format = DXGI_FORMAT_R10G10B10A2_UNORM; flags |= BIASED_VERTEX_NORMALS; offset += 4; break;
                case D3DDECLTYPE_DXGI_R11G11B10_FLOAT:   desc.Format = DXGI_FORMAT_R11G11B10_FLOAT; flags |= BIASED_VERTEX_NORMALS; offset += 4; break;
                case D3DDECLTYPE_DXGI_R8G8B8A8_SNORM:    desc.Format = DXGI_FORMAT_R8G8B8A8_SNORM; offset += 4; break;

                #if (defined(_XBOX_ONE) && defined(_TITLE)) || defined(_GAMING_XBOX)
                case D3DDECLTYPE_DEC3N:                  desc.Format = DXGI_FORMAT_R10G10B10_SNORM_A2_UNORM; offset += 4; break;
                case (32 + DXGI_FORMAT_R10G10B10_SNORM_A2_UNORM): desc.Format = DXGI_FORMAT_R10G10B10_SNORM_A2_UNORM; offset += 4; break;
                #else
                case D3DDECLTYPE_DEC3N:                  desc.Format = DXGI_FORMAT_R10G10B10A2_UNORM; flags |= USES_OBSOLETE_DEC3N; offset += 4; break;
                #endif

                default:
                    unk = true;
                    break;
                }

                if (unk)
                    break;

                inputDesc.push_back(desc);
            }
            else if (decl[index].Usage == D3DDECLUSAGE_COLOR)
            {
                D3D12_INPUT_ELEMENT_DESC desc = s_elements[2];

                bool unk = false;
                switch (decl[index].Type)
                {
                case D3DDECLTYPE_FLOAT4:                 desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; offset += 16; break;
                case D3DDECLTYPE_D3DCOLOR:               assert(desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM); offset += 4; break;
                case D3DDECLTYPE_UBYTE4N:                desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; offset += 4; break;
                case D3DDECLTYPE_FLOAT16_4:              desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT; offset += 8; break;
                case D3DDECLTYPE_DXGI_R10G10B10A2_UNORM: desc.Format = DXGI_FORMAT_R10G10B10A2_UNORM; offset += 4; break;
                case D3DDECLTYPE_DXGI_R11G11B10_FLOAT:   desc.Format = DXGI_FORMAT_R11G11B10_FLOAT; offset += 4; break;

                default:
                    unk = true;
                    break;
                }

                if (unk)
                    break;

                flags |= PER_VERTEX_COLOR;

                inputDesc.push_back(desc);
            }
            else if (decl[index].Usage == D3DDECLUSAGE_TEXCOORD)
            {
                D3D12_INPUT_ELEMENT_DESC desc = s_elements[5];
                desc.SemanticIndex = decl[index].UsageIndex;

                bool unk = false;
                switch (decl[index].Type)
                {
                case D3DDECLTYPE_FLOAT1:    desc.Format = DXGI_FORMAT_R32_FLOAT; offset += 4; break;
                case D3DDECLTYPE_FLOAT2:    assert(desc.Format == DXGI_FORMAT_R32G32_FLOAT); offset += 8; break;
                case D3DDECLTYPE_FLOAT3:    desc.Format = DXGI_FORMAT_R32G32B32_FLOAT; offset += 12; break;
                case D3DDECLTYPE_FLOAT4:    desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; offset += 16; break;
                case D3DDECLTYPE_FLOAT16_2: desc.Format = DXGI_FORMAT_R16G16_FLOAT; offset += 4; break;
                case D3DDECLTYPE_FLOAT16_4: desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT; offset += 8; break;

                default:
                    unk = true;
                    break;
                }

                if (unk)
                    break;

                ++texcoords;

                inputDesc.push_back(desc);
            }
            else if (decl[index].Usage == D3DDECLUSAGE_BLENDINDICES)
            {
                if (decl[index].Type == D3DDECLTYPE_UBYTE4)
                {
                    flags |= SKINNING;
                    inputDesc.push_back(s_elements[6]);
                    offset += 4;
                }
                else
                    break;
            }
            else if (decl[index].Usage == D3DDECLUSAGE_BLENDWEIGHT)
            {
                if (decl[index].Type == D3DDECLTYPE_UBYTE4N)
                {
                    flags |= SKINNING;
                    inputDesc.push_back(s_elements[7]);
                    offset += 4;
                }
                else
                    break;
            }
            else
                break;
        }

        if (!posfound)
            throw std::runtime_error("SV_Position is required");

        if (texcoords == 2)
        {
            flags |= DUAL_TEXTURE;
        }

        return flags;
    }
}

//======================================================================================
// Model Loader
//======================================================================================

_Use_decl_annotations_
std::unique_ptr<Model> DirectX::Model::CreateFromSDKMESH(
    ID3D12Device* device,
    const uint8_t* meshData,
    size_t idataSize,
    ModelLoaderFlags flags)
{
    if (!meshData)
        throw std::invalid_argument("meshData cannot be null");

    const uint64_t dataSize = idataSize;

    // File Headers
    if (dataSize < sizeof(DXUT::SDKMESH_HEADER))
        throw std::runtime_error("End of file");
    auto header = reinterpret_cast<const DXUT::SDKMESH_HEADER*>(meshData);

    const size_t headerSize = sizeof(DXUT::SDKMESH_HEADER)
        + header->NumVertexBuffers * sizeof(DXUT::SDKMESH_VERTEX_BUFFER_HEADER)
        + header->NumIndexBuffers * sizeof(DXUT::SDKMESH_INDEX_BUFFER_HEADER);
    if (header->HeaderSize != headerSize)
        throw std::runtime_error("Not a valid SDKMESH file");

    if (dataSize < header->HeaderSize)
        throw std::runtime_error("End of file");

    if (header->Version != DXUT::SDKMESH_FILE_VERSION && header->Version != DXUT::SDKMESH_FILE_VERSION_V2)
        throw std::runtime_error("Not a supported SDKMESH version");

    if (header->IsBigEndian)
        throw std::runtime_error("Loading BigEndian SDKMESH files not supported");

    if (!header->NumMeshes)
        throw std::runtime_error("No meshes found");

    if (!header->NumVertexBuffers)
        throw std::runtime_error("No vertex buffers found");

    if (!header->NumIndexBuffers)
        throw std::runtime_error("No index buffers found");

    if (!header->NumTotalSubsets)
        throw std::runtime_error("No subsets found");

    if (!header->NumMaterials)
        throw std::runtime_error("No materials found");

    // Sub-headers
    if (dataSize < header->VertexStreamHeadersOffset
        || (dataSize < (header->VertexStreamHeadersOffset + uint64_t(header->NumVertexBuffers) * sizeof(DXUT::SDKMESH_VERTEX_BUFFER_HEADER))))
        throw std::runtime_error("End of file");
    auto vbArray = reinterpret_cast<const DXUT::SDKMESH_VERTEX_BUFFER_HEADER*>(meshData + header->VertexStreamHeadersOffset);

    if (dataSize < header->IndexStreamHeadersOffset
        || (dataSize < (header->IndexStreamHeadersOffset + uint64_t(header->NumIndexBuffers) * sizeof(DXUT::SDKMESH_INDEX_BUFFER_HEADER))))
        throw std::runtime_error("End of file");
    auto ibArray = reinterpret_cast<const DXUT::SDKMESH_INDEX_BUFFER_HEADER*>(meshData + header->IndexStreamHeadersOffset);

    if (dataSize < header->MeshDataOffset
        || (dataSize < (header->MeshDataOffset + uint64_t(header->NumMeshes) * sizeof(DXUT::SDKMESH_MESH))))
        throw std::runtime_error("End of file");
    auto meshArray = reinterpret_cast<const DXUT::SDKMESH_MESH*>(meshData + header->MeshDataOffset);

    if (dataSize < header->SubsetDataOffset
        || (dataSize < (header->SubsetDataOffset + uint64_t(header->NumTotalSubsets) * sizeof(DXUT::SDKMESH_SUBSET))))
        throw std::runtime_error("End of file");
    auto subsetArray = reinterpret_cast<const DXUT::SDKMESH_SUBSET*>(meshData + header->SubsetDataOffset);

    const DXUT::SDKMESH_FRAME* frameArray = nullptr;
    if (header->NumFrames > 0)
    {
        if (dataSize < header->FrameDataOffset
            || (dataSize < (header->FrameDataOffset + uint64_t(header->NumFrames) * sizeof(DXUT::SDKMESH_FRAME))))
            throw std::runtime_error("End of file");

        if (flags & ModelLoader_IncludeBones)
        {
            frameArray = reinterpret_cast<const DXUT::SDKMESH_FRAME*>(meshData + header->FrameDataOffset);
        }
    }

    if (dataSize < header->MaterialDataOffset
        || (dataSize < (header->MaterialDataOffset + uint64_t(header->NumMaterials) * sizeof(DXUT::SDKMESH_MATERIAL))))
        throw std::runtime_error("End of file");

    const DXUT::SDKMESH_MATERIAL* materialArray = nullptr;
    const DXUT::SDKMESH_MATERIAL_V2* materialArray_v2 = nullptr;
    if (header->Version == DXUT::SDKMESH_FILE_VERSION_V2)
    {
        materialArray_v2 = reinterpret_cast<const DXUT::SDKMESH_MATERIAL_V2*>(meshData + header->MaterialDataOffset);
    }
    else
    {
        materialArray = reinterpret_cast<const DXUT::SDKMESH_MATERIAL*>(meshData + header->MaterialDataOffset);
    }

    // Buffer data
    const uint64_t bufferDataOffset = header->HeaderSize + header->NonBufferDataSize;
    if ((dataSize < bufferDataOffset)
        || (dataSize < bufferDataOffset + header->BufferDataSize))
        throw std::runtime_error("End of file");
    const uint8_t* bufferData = meshData + bufferDataOffset;

    // Create vertex buffers
    std::vector<std::shared_ptr<ModelMeshPart::InputLayoutCollection>> vbDecls;
    vbDecls.resize(header->NumVertexBuffers);

    std::vector<unsigned int> materialFlags;
    materialFlags.resize(header->NumVertexBuffers);

    bool dec3nwarning = false;
    for (size_t j = 0; j < header->NumVertexBuffers; ++j)
    {
        auto& vh = vbArray[j];

        if (vh.SizeBytes > UINT32_MAX)
            throw std::runtime_error("VB too large");

        if (!(flags & ModelLoader_AllowLargeModels))
        {
            if (vh.SizeBytes > (D3D12_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_A_TERM * 1024u * 1024u))
                throw std::runtime_error("VB too large for DirectX 12");
        }

        if (dataSize < vh.DataOffset
            || (dataSize < vh.DataOffset + vh.SizeBytes))
            throw std::runtime_error("End of file");

        vbDecls[j] = std::make_shared<ModelMeshPart::InputLayoutCollection>();
        unsigned int ilflags = GetInputLayoutDesc(vh.Decl, *vbDecls[j].get());

        if (flags & ModelLoader_DisableSkinning)
        {
            ilflags &= ~static_cast<unsigned int>(SKINNING);
        }

        if (ilflags & SKINNING)
        {
            ilflags &= ~static_cast<unsigned int>(DUAL_TEXTURE);
        }
        if (ilflags & USES_OBSOLETE_DEC3N)
        {
            dec3nwarning = true;
        }

        materialFlags[j] = ilflags;
    }

    if (dec3nwarning)
    {
        DebugTrace("WARNING: Vertex declaration uses legacy Direct3D 9 D3DDECLTYPE_DEC3N which has no DXGI equivalent\n"
            "         (treating as DXGI_FORMAT_R10G10B10A2_UNORM which is not a signed format)\n");
    }

    // Validate index buffers
    for (size_t j = 0; j < header->NumIndexBuffers; ++j)
    {
        auto& ih = ibArray[j];

        if (ih.SizeBytes > UINT32_MAX)
            throw std::runtime_error("IB too large");

        if (!(flags & ModelLoader_AllowLargeModels))
        {
            if (ih.SizeBytes > (D3D12_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_A_TERM * 1024u * 1024u))
                throw std::runtime_error("IB too large for DirectX 12");
        }

        if (dataSize < ih.DataOffset
            || (dataSize < ih.DataOffset + ih.SizeBytes))
            throw std::runtime_error("End of file");

        if (ih.IndexType != DXUT::IT_16BIT && ih.IndexType != DXUT::IT_32BIT)
            throw std::runtime_error("Invalid index buffer type found");
    }

    // Create meshes
    std::vector<ModelMaterialInfo> materials;
    materials.resize(header->NumMaterials);

    std::map<std::wstring, int> textureDictionary;

    auto model = std::make_unique<Model>();
    model->meshes.reserve(header->NumMeshes);

    uint32_t partCount = 0;

    for (size_t meshIndex = 0; meshIndex < header->NumMeshes; ++meshIndex)
    {
        auto& mh = meshArray[meshIndex];

        if (!mh.NumSubsets
            || !mh.NumVertexBuffers
            || mh.IndexBuffer >= header->NumIndexBuffers
            || mh.VertexBuffers[0] >= header->NumVertexBuffers)
            throw std::out_of_range("Invalid mesh found");

        // mh.NumVertexBuffers is sometimes not what you'd expect, so we skip validating it

        if (dataSize < mh.SubsetOffset
            || (dataSize < mh.SubsetOffset + uint64_t(mh.NumSubsets) * sizeof(uint32_t)))
            throw std::runtime_error("End of file");

        auto subsets = reinterpret_cast<const uint32_t*>(meshData + mh.SubsetOffset);

        const uint32_t* influences = nullptr;
        if (mh.NumFrameInfluences > 0)
        {
            if (dataSize < mh.FrameInfluenceOffset
                || (dataSize < mh.FrameInfluenceOffset + uint64_t(mh.NumFrameInfluences) * sizeof(uint32_t)))
                throw std::runtime_error("End of file");

            if (flags & ModelLoader_IncludeBones)
            {
                influences = reinterpret_cast<const uint32_t*>(meshData + mh.FrameInfluenceOffset);
            }
        }

        auto mesh = std::make_shared<ModelMesh>();
        wchar_t meshName[DXUT::MAX_MESH_NAME] = {};
        ASCIIToWChar(meshName, mh.Name);

        mesh->name = meshName;

        // Extents
        mesh->boundingBox.Center = mh.BoundingBoxCenter;
        mesh->boundingBox.Extents = mh.BoundingBoxExtents;
        BoundingSphere::CreateFromBoundingBox(mesh->boundingSphere, mesh->boundingBox);

        if (influences)
        {
            mesh->boneInfluences.resize(mh.NumFrameInfluences);
            memcpy(mesh->boneInfluences.data(), influences, sizeof(uint32_t) * mh.NumFrameInfluences);
        }

        // Create subsets
        for (size_t j = 0; j < mh.NumSubsets; ++j)
        {
            auto const sIndex = subsets[j];
            if (sIndex >= header->NumTotalSubsets)
                throw std::out_of_range("Invalid mesh found");

            auto& subset = subsetArray[sIndex];

            D3D_PRIMITIVE_TOPOLOGY primType;
            switch (subset.PrimitiveType)
            {
            case DXUT::PT_TRIANGLE_LIST:        primType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;       break;
            case DXUT::PT_TRIANGLE_STRIP:       primType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;      break;
            case DXUT::PT_LINE_LIST:            primType = D3D_PRIMITIVE_TOPOLOGY_LINELIST;           break;
            case DXUT::PT_LINE_STRIP:           primType = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;          break;
            case DXUT::PT_POINT_LIST:           primType = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;          break;
            case DXUT::PT_TRIANGLE_LIST_ADJ:    primType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ;   break;
            case DXUT::PT_TRIANGLE_STRIP_ADJ:   primType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ;  break;
            case DXUT::PT_LINE_LIST_ADJ:        primType = D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ;       break;
            case DXUT::PT_LINE_STRIP_ADJ:       primType = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ;      break;

            case DXUT::PT_QUAD_PATCH_LIST:
            case DXUT::PT_TRIANGLE_PATCH_LIST:
                throw std::runtime_error("Direct3D9 era tessellation not supported");

            default:
                throw std::runtime_error("Unknown primitive type");
            }

            if (subset.MaterialID >= header->NumMaterials)
                throw std::out_of_range("Invalid mesh found");

            auto& mat = materials[subset.MaterialID];

            const size_t vi = mh.VertexBuffers[0];
            if (materialArray_v2)
            {
                InitMaterial(
                    materialArray_v2[subset.MaterialID],
                    materialFlags[vi],
                    mat,
                    textureDictionary);
            }
            else
            {
                InitMaterial(
                    materialArray[subset.MaterialID],
                    materialFlags[vi],
                    mat,
                    textureDictionary,
                    (flags & ModelLoader_MaterialColorsSRGB) != 0);
            }

            auto part = new ModelMeshPart(partCount++);

            const auto& vh = vbArray[mh.VertexBuffers[0]];
            const auto& ih = ibArray[mh.IndexBuffer];

            part->indexCount = static_cast<uint32_t>(subset.IndexCount);
            part->startIndex = static_cast<uint32_t>(subset.IndexStart);
            part->vertexOffset = static_cast<int32_t>(subset.VertexStart);
            part->vertexStride = static_cast<uint32_t>(vh.StrideBytes);
            part->vertexCount = static_cast<uint32_t>(subset.VertexCount);
            part->primitiveType = primType;
            part->indexFormat = (ibArray[mh.IndexBuffer].IndexType == DXUT::IT_32BIT) ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;

            // Vertex data
            auto verts = bufferData + (vh.DataOffset - bufferDataOffset);
            auto const vbytes = static_cast<size_t>(vh.SizeBytes);
            part->vertexBufferSize = static_cast<uint32_t>(vh.SizeBytes);
            part->vertexBuffer = GraphicsMemory::Get(device).Allocate(vbytes);
            memcpy(part->vertexBuffer.Memory(), verts, vbytes);

            // Index data
            auto indices = bufferData + (ih.DataOffset - bufferDataOffset);
            auto const ibytes = static_cast<size_t>(ih.SizeBytes);
            part->indexBufferSize = static_cast<uint32_t>(ih.SizeBytes);
            part->indexBuffer = GraphicsMemory::Get(device).Allocate(ibytes);
            memcpy(part->indexBuffer.Memory(), indices, ibytes);

            part->materialIndex = subset.MaterialID;
            part->vbDecl = vbDecls[mh.VertexBuffers[0]];

            if (mat.alphaValue < 1.0f)
                mesh->alphaMeshParts.emplace_back(part);
            else
                mesh->opaqueMeshParts.emplace_back(part);
        }

        model->meshes.emplace_back(mesh);
    }

    // Copy the materials and texture names into contiguous arrays
    model->materials = std::move(materials);
    model->textureNames.resize(textureDictionary.size());
    for (auto texture = std::cbegin(textureDictionary); texture != std::cend(textureDictionary); ++texture)
    {
        model->textureNames[static_cast<size_t>(texture->second)] = texture->first;
    }

    // Load model bones (if present and requested)
    if (frameArray)
    {
        static_assert(DXUT::INVALID_FRAME == ModelBone::c_Invalid, "ModelBone invalid type mismatch");

        ModelBone::Collection bones;
        bones.reserve(header->NumFrames);
        auto transforms = ModelBone::MakeArray(header->NumFrames);

        for (uint32_t j = 0; j < header->NumFrames; ++j)
        {
            ModelBone bone(
                frameArray[j].ParentFrame,
                frameArray[j].ChildFrame,
                frameArray[j].SiblingFrame);

            wchar_t boneName[DXUT::MAX_FRAME_NAME] = {};
            ASCIIToWChar(boneName, frameArray[j].Name);
            bone.name = boneName;
            bones.emplace_back(bone);

            transforms[j] = XMLoadFloat4x4(&frameArray[j].Matrix);

            const uint32_t index = frameArray[j].Mesh;
            if (index != DXUT::INVALID_MESH)
            {
                if (index >= model->meshes.size())
                {
                    throw std::out_of_range("Invalid mesh index found in frame data");
                }

                if (model->meshes[index]->boneIndex == ModelBone::c_Invalid)
                {
                    // Bind the first bone that links to a given mesh
                    model->meshes[index]->boneIndex = j;
                }
            }
        }

        std::swap(model->bones, bones);

        // Compute inverse bind pose matrices for the model
        auto bindPose = ModelBone::MakeArray(header->NumFrames);
        model->CopyAbsoluteBoneTransforms(header->NumFrames, transforms.get(), bindPose.get());

        auto invBoneTransforms = ModelBone::MakeArray(header->NumFrames);
        for (size_t j = 0; j < header->NumFrames; ++j)
        {
            invBoneTransforms[j] = XMMatrixInverse(nullptr, bindPose[j]);
        }

        std::swap(model->boneMatrices, transforms);
        std::swap(model->invBindPoseMatrices, invBoneTransforms);
    }

    return model;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
std::unique_ptr<Model> DirectX::Model::CreateFromSDKMESH(
    ID3D12Device* device,
    const wchar_t* szFileName,
    ModelLoaderFlags flags)
{
    size_t dataSize = 0;
    std::unique_ptr<uint8_t[]> data;
    HRESULT hr = BinaryReader::ReadEntireFile(szFileName, data, &dataSize);
    if (FAILED(hr))
    {
        DebugTrace("ERROR: CreateFromSDKMESH failed (%08X) loading '%ls'\n",
            static_cast<unsigned int>(hr), szFileName);
        throw std::runtime_error("CreateFromSDKMESH");
    }

    auto model = CreateFromSDKMESH(device, data.get(), dataSize, flags);

    model->name = szFileName;

    return model;
}
