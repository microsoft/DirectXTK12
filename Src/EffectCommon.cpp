//--------------------------------------------------------------------------------------
// File: EffectCommon.cpp
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
#include "EffectCommon.h"
#include "DemandCreate.h"
#include "ResourceUploadBatch.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace
{
    // 0x04C11DB7 is the official polynomial used by PKZip, WinZip and Ethernet
    static const uint32_t s_crc32[] =
    {
        0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
        0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
        0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
        0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
        0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
        0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
        0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
        0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
        0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
        0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
        0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
        0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
        0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
        0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
        0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
        0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
        0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
        0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
        0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
        0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
        0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
        0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
        0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
        0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
        0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
        0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
        0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
        0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
        0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
        0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
        0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
        0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
        0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
        0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
        0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
        0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
        0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
        0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
        0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
        0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
        0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
        0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
        0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d,
    };
}

uint32_t EffectPipelineStateDescription::ComputeHash() const
{
    // Computes a CRC32 of the structure
    uint32_t crc = 0xFFFFFFFF;

    // Ignores the input layout which contains pointers
    auto ptr = reinterpret_cast<const uint8_t*>(this) + sizeof(D3D12_INPUT_LAYOUT_DESC);
    for (size_t j = 0; j < (sizeof(EffectPipelineStateDescription) - sizeof(D3D12_INPUT_LAYOUT_DESC)); ++j)
    {
        crc = (crc >> 8) ^ s_crc32[(crc & 0xff) ^ *ptr++];
    }

    return crc ^ 0xFFFFFFFF;
}


// IEffectMatrices default method
void XM_CALLCONV IEffectMatrices::SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection)
{
    SetWorld(world);
    SetView(view);
    SetProjection(projection);
}


// Constructor initializes default matrix values.
EffectMatrices::EffectMatrices()
{
    world = XMMatrixIdentity();
    view = XMMatrixIdentity();
    projection = XMMatrixIdentity();
    worldView = XMMatrixIdentity();
}


// Lazily recomputes the combined world+view+projection matrix.
_Use_decl_annotations_ void EffectMatrices::SetConstants(int& dirtyFlags, XMMATRIX& worldViewProjConstant)
{
    if (dirtyFlags & EffectDirtyFlags::WorldViewProj)
    {
        worldView = XMMatrixMultiply(world, view);

        worldViewProjConstant = XMMatrixTranspose(XMMatrixMultiply(worldView, projection));
                
        dirtyFlags &= ~EffectDirtyFlags::WorldViewProj;
        dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
    }
}


// Constructor initializes default fog settings.
EffectFog::EffectFog()
{
    enabled = false;
    start = 0;
    end = 1;
}


// Lazily recomputes the derived vector used by shader fog calculations.
_Use_decl_annotations_
void XM_CALLCONV EffectFog::SetConstants(int& dirtyFlags, FXMMATRIX worldView, XMVECTOR& fogVectorConstant)
{
    if (enabled)
    {
        if (dirtyFlags & (EffectDirtyFlags::FogVector | EffectDirtyFlags::FogEnable))
        {
            if (start == end)
            {
                // Degenerate case: force everything to 100% fogged if start and end are the same.
                static const XMVECTORF32 fullyFogged = { 0, 0, 0, 1 };

                fogVectorConstant = fullyFogged;
            }
            else
            {
                // We want to transform vertex positions into view space, take the resulting
                // Z value, then scale and offset according to the fog start/end distances.
                // Because we only care about the Z component, the shader can do all this
                // with a single dot product, using only the Z row of the world+view matrix.
        
                // _13, _23, _33, _43
                XMVECTOR worldViewZ = XMVectorMergeXY(XMVectorMergeZW(worldView.r[0], worldView.r[2]),
                                                      XMVectorMergeZW(worldView.r[1], worldView.r[3]));

                // 0, 0, 0, fogStart
                XMVECTOR wOffset = XMVectorSwizzle<1, 2, 3, 0>(XMLoadFloat(&start));

                fogVectorConstant = (worldViewZ + wOffset) / (start - end);
            }

            dirtyFlags &= ~(EffectDirtyFlags::FogVector | EffectDirtyFlags::FogEnable);
            dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
        }
    }
    else
    {
        // When fog is disabled, make sure the fog vector is reset to zero.
        if (dirtyFlags & EffectDirtyFlags::FogEnable)
        {
            fogVectorConstant = g_XMZero;

            dirtyFlags &= ~EffectDirtyFlags::FogEnable;
            dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
        }
    }
}


// Constructor initializes default material color settings.
EffectColor::EffectColor()
{
    diffuseColor = g_XMOne;
    alpha = 1;
}


// Lazily recomputes the material color parameter for shaders that do not support realtime lighting.
void EffectColor::SetConstants(_Inout_ int& dirtyFlags, _Inout_ XMVECTOR& diffuseColorConstant)
{
    if (dirtyFlags & EffectDirtyFlags::MaterialColor)
    {
        XMVECTOR alphaVector = XMVectorReplicate(alpha);

        // xyz = diffuse * alpha, w = alpha.
        diffuseColorConstant = XMVectorSelect(alphaVector, diffuseColor * alphaVector, g_XMSelect1110);

        dirtyFlags &= ~EffectDirtyFlags::MaterialColor;
        dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
    }
}


// Constructor initializes default light settings.
EffectLights::EffectLights()
{
    emissiveColor = g_XMZero;
    ambientLightColor = g_XMZero;

    for (int i = 0; i < MaxDirectionalLights; i++)
    {
        lightEnabled[i] = (i == 0);
        lightDiffuseColor[i] = g_XMOne;
        lightSpecularColor[i] = g_XMZero;
    }
}


#pragma prefast(push)
#pragma prefast(disable:22103, "PREFAST doesn't understand buffer is bounded by a static const value even with SAL" )

// Initializes constant buffer fields to match the current lighting state.
_Use_decl_annotations_ void EffectLights::InitializeConstants(XMVECTOR& specularColorAndPowerConstant, XMVECTOR* lightDirectionConstant, XMVECTOR* lightDiffuseConstant, XMVECTOR* lightSpecularConstant)
{
    static const XMVECTORF32 defaultSpecular = { 1, 1, 1, 16 };
    static const XMVECTORF32 defaultLightDirection = { 0, -1, 0, 0 };
    
    specularColorAndPowerConstant = defaultSpecular;

    for (int i = 0; i < MaxDirectionalLights; i++)
    {
        lightDirectionConstant[i] = defaultLightDirection;

        lightDiffuseConstant[i]  = lightEnabled[i] ? lightDiffuseColor[i]  : g_XMZero;
        lightSpecularConstant[i] = lightEnabled[i] ? lightSpecularColor[i] : g_XMZero;
    }
}

#pragma prefast(pop)


// Lazily recomputes derived parameter values used by shader lighting calculations.
_Use_decl_annotations_ void EffectLights::SetConstants(int& dirtyFlags, EffectMatrices const& matrices, XMMATRIX& worldConstant, XMVECTOR worldInverseTransposeConstant[3], XMVECTOR& eyePositionConstant, XMVECTOR& diffuseColorConstant, XMVECTOR& emissiveColorConstant, bool lightingEnabled)
{
    if (lightingEnabled)
    {
        // World inverse transpose matrix.
        if (dirtyFlags & EffectDirtyFlags::WorldInverseTranspose)
        {
            worldConstant = XMMatrixTranspose(matrices.world);

            XMMATRIX worldInverse = XMMatrixInverse(nullptr, matrices.world);

            worldInverseTransposeConstant[0] = worldInverse.r[0];
            worldInverseTransposeConstant[1] = worldInverse.r[1];
            worldInverseTransposeConstant[2] = worldInverse.r[2];

            dirtyFlags &= ~EffectDirtyFlags::WorldInverseTranspose;
            dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
        }

        // Eye position vector.
        if (dirtyFlags & EffectDirtyFlags::EyePosition)
        {
            XMMATRIX viewInverse = XMMatrixInverse(nullptr, matrices.view);
        
            eyePositionConstant = viewInverse.r[3];

            dirtyFlags &= ~EffectDirtyFlags::EyePosition;
            dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
        }
    }

    // Material color parameters. The desired lighting model is:
    //
    //     ((ambientLightColor + sum(diffuse directional light)) * diffuseColor) + emissiveColor
    //
    // When lighting is disabled, ambient and directional lights are ignored, leaving:
    //
    //     diffuseColor + emissiveColor
    //
    // For the lighting disabled case, we can save one shader instruction by precomputing
    // diffuse+emissive on the CPU, after which the shader can use diffuseColor directly,
    // ignoring its emissive parameter.
    //
    // When lighting is enabled, we can merge the ambient and emissive settings. If we
    // set our emissive parameter to emissive+(ambient*diffuse), the shader no longer
    // needs to bother adding the ambient contribution, simplifying its computation to:
    //
    //     (sum(diffuse directional light) * diffuseColor) + emissiveColor
    //
    // For futher optimization goodness, we merge material alpha with the diffuse
    // color parameter, and premultiply all color values by this alpha.

    if (dirtyFlags & EffectDirtyFlags::MaterialColor)
    {
        XMVECTOR diffuse = diffuseColor;
        XMVECTOR alphaVector = XMVectorReplicate(alpha);

        if (lightingEnabled)
        {
            // Merge emissive and ambient light contributions.
            emissiveColorConstant = (emissiveColor + ambientLightColor * diffuse) * alphaVector;
        }
        else
        {
            // Merge diffuse and emissive light contributions.
            diffuse += emissiveColor;
        }

        // xyz = diffuse * alpha, w = alpha.
        diffuseColorConstant = XMVectorSelect(alphaVector, diffuse * alphaVector, g_XMSelect1110);

        dirtyFlags &= ~EffectDirtyFlags::MaterialColor;
        dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
    }
}


#pragma prefast(push)
#pragma prefast(disable:26015, "PREFAST doesn't understand that ValidateLightIndex bounds whichLight" )

// Helper for turning one of the directional lights on or off.
_Use_decl_annotations_ int EffectLights::SetLightEnabled(int whichLight, bool value, XMVECTOR* lightDiffuseConstant, XMVECTOR* lightSpecularConstant)
{
    ValidateLightIndex(whichLight);

    if (lightEnabled[whichLight] == value)
        return 0;

    lightEnabled[whichLight] = value;

    if (value)
    {
        // If this light is now on, store its color in the constant buffer.
        lightDiffuseConstant[whichLight] = lightDiffuseColor[whichLight];
        lightSpecularConstant[whichLight] = lightSpecularColor[whichLight];
    }
    else
    {
        // If the light is off, reset constant buffer colors to zero.
        lightDiffuseConstant[whichLight] = g_XMZero;
        lightSpecularConstant[whichLight] = g_XMZero;
    }

    return EffectDirtyFlags::ConstantBuffer;
}


// Helper for setting diffuse color of one of the directional lights.
_Use_decl_annotations_
int XM_CALLCONV EffectLights::SetLightDiffuseColor(int whichLight, FXMVECTOR value, XMVECTOR* lightDiffuseConstant)
{
    ValidateLightIndex(whichLight);

    // Locally store the new color.
    lightDiffuseColor[whichLight] = value;

    // If this light is currently on, also update the constant buffer.
    if (lightEnabled[whichLight])
    {
        lightDiffuseConstant[whichLight] = value;
        
        return EffectDirtyFlags::ConstantBuffer;
    }

    return 0;
}


// Helper for setting specular color of one of the directional lights.
_Use_decl_annotations_
int XM_CALLCONV EffectLights::SetLightSpecularColor(int whichLight, FXMVECTOR value, XMVECTOR* lightSpecularConstant)
{
    ValidateLightIndex(whichLight);

    // Locally store the new color.
    lightSpecularColor[whichLight] = value;

    // If this light is currently on, also update the constant buffer.
    if (lightEnabled[whichLight])
    {
        lightSpecularConstant[whichLight] = value;

        return EffectDirtyFlags::ConstantBuffer;
    }
    
    return 0;
}

#pragma prefast(pop)


// Parameter validation helper.
void EffectLights::ValidateLightIndex(int whichLight)
{
    if (whichLight < 0 || whichLight >= MaxDirectionalLights)
    {
        throw std::out_of_range("whichLight parameter out of range");
    }
}


// Activates the default lighting rig (key, fill, and back lights).
void EffectLights::EnableDefaultLighting(_In_ IEffectLights* effect)
{
    static const XMVECTORF32 defaultDirections[MaxDirectionalLights] =
    {
        { -0.5265408f, -0.5735765f, -0.6275069f },
        {  0.7198464f,  0.3420201f,  0.6040227f },
        {  0.4545195f, -0.7660444f,  0.4545195f },
    };

    static const XMVECTORF32 defaultDiffuse[MaxDirectionalLights] =
    {
        { 1.0000000f, 0.9607844f, 0.8078432f },
        { 0.9647059f, 0.7607844f, 0.4078432f },
        { 0.3231373f, 0.3607844f, 0.3937255f },
    };

    static const XMVECTORF32 defaultSpecular[MaxDirectionalLights] =
    {
        { 1.0000000f, 0.9607844f, 0.8078432f },
        { 0.0000000f, 0.0000000f, 0.0000000f },
        { 0.3231373f, 0.3607844f, 0.3937255f },
    };

    static const XMVECTORF32 defaultAmbient = { 0.05333332f, 0.09882354f, 0.1819608f };

    effect->SetAmbientLightColor(defaultAmbient);

    for (int i = 0; i < MaxDirectionalLights; i++)
    {
        effect->SetLightEnabled(i, true);
        effect->SetLightDirection(i, defaultDirections[i]);
        effect->SetLightDiffuseColor(i, defaultDiffuse[i]);
        effect->SetLightSpecularColor(i, defaultSpecular[i]);
    }
}
