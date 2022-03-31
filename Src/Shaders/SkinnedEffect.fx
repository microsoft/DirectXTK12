// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=615561


Texture2D<float4> Texture : register(t0);
sampler Sampler : register(s0);


cbuffer Parameters : register(b0)
{
    float4 DiffuseColor             : packoffset(c0);
    float3 EmissiveColor            : packoffset(c1);
    float3 SpecularColor            : packoffset(c2);
    float  SpecularPower            : packoffset(c2.w);

    float3 LightDirection[3]        : packoffset(c3);
    float3 LightDiffuseColor[3]     : packoffset(c6);
    float3 LightSpecularColor[3]    : packoffset(c9);

    float3 EyePosition              : packoffset(c12);

    float3 FogColor                 : packoffset(c13);
    float4 FogVector                : packoffset(c14);

    float4x4 World                  : packoffset(c15);
    float3x3 WorldInverseTranspose  : packoffset(c19);
    float4x4 WorldViewProj          : packoffset(c22);

    float4x3 Bones[72]              : packoffset(c26);
};


#include "Structures.fxh"
#include "Common.fxh"
#include "RootSig.fxh"
#include "Lighting.fxh"
#include "Utilities.fxh"
#include "Skinning.fxh"


// Vertex shader: vertex lighting, one bone.
[RootSignature(MainRS)]
VSOutputTx VSSkinnedVertexLightingOneBone(VSInputNmTxWeights vin)
{
    VSOutputTx vout;

    float3 normal = Skin(vin, vin.Normal, 1);

    CommonVSOutput cout = ComputeCommonVSOutputWithLighting(vin.Position, normal, 3);
    SetCommonVSOutputParams;

    vout.TexCoord = vin.TexCoord;

    return vout;
}

[RootSignature(MainRS)]
VSOutputTx VSSkinnedVertexLightingOneBoneBn(VSInputNmTxWeights vin)
{
    VSOutputTx vout;

    float3 normal = BiasX2(vin.Normal);

    normal = Skin(vin, normal, 1);

    CommonVSOutput cout = ComputeCommonVSOutputWithLighting(vin.Position, normal, 3);
    SetCommonVSOutputParams;

    vout.TexCoord = vin.TexCoord;

    return vout;
}


// Vertex shader: vertex lighting, two bones.
[RootSignature(MainRS)]
VSOutputTx VSSkinnedVertexLightingTwoBones(VSInputNmTxWeights vin)
{
    VSOutputTx vout;

    float3 normal = Skin(vin, vin.Normal, 2);

    CommonVSOutput cout = ComputeCommonVSOutputWithLighting(vin.Position, normal, 3);
    SetCommonVSOutputParams;

    vout.TexCoord = vin.TexCoord;

    return vout;
}

[RootSignature(MainRS)]
VSOutputTx VSSkinnedVertexLightingTwoBonesBn(VSInputNmTxWeights vin)
{
    VSOutputTx vout;

    float3 normal = BiasX2(vin.Normal);

    normal = Skin(vin, normal, 2);

    CommonVSOutput cout = ComputeCommonVSOutputWithLighting(vin.Position, normal, 3);
    SetCommonVSOutputParams;

    vout.TexCoord = vin.TexCoord;

    return vout;
}


// Vertex shader: vertex lighting, four bones.
[RootSignature(MainRS)]
VSOutputTx VSSkinnedVertexLightingFourBones(VSInputNmTxWeights vin)
{
    VSOutputTx vout;

    float3 normal = Skin(vin, vin.Normal, 4);

    CommonVSOutput cout = ComputeCommonVSOutputWithLighting(vin.Position, normal, 3);
    SetCommonVSOutputParams;

    vout.TexCoord = vin.TexCoord;

    return vout;
}

[RootSignature(MainRS)]
VSOutputTx VSSkinnedVertexLightingFourBonesBn(VSInputNmTxWeights vin)
{
    VSOutputTx vout;

    float3 normal = BiasX2(vin.Normal);

    normal = Skin(vin, normal, 4);

    CommonVSOutput cout = ComputeCommonVSOutputWithLighting(vin.Position, normal, 3);
    SetCommonVSOutputParams;

    vout.TexCoord = vin.TexCoord;

    return vout;
}


// Vertex shader: pixel lighting, one bone.
[RootSignature(MainRS)]
VSOutputPixelLightingTx VSSkinnedPixelLightingOneBone(VSInputNmTxWeights vin)
{
    VSOutputPixelLightingTx vout;

    float3 normal = Skin(vin, vin.Normal, 1);

    CommonVSOutputPixelLighting cout = ComputeCommonVSOutputPixelLighting(vin.Position, normal);
    SetCommonVSOutputParamsPixelLighting;

    vout.Diffuse = float4(1, 1, 1, DiffuseColor.a);
    vout.TexCoord = vin.TexCoord;

    return vout;
}

[RootSignature(MainRS)]
VSOutputPixelLightingTx VSSkinnedPixelLightingOneBoneBn(VSInputNmTxWeights vin)
{
    VSOutputPixelLightingTx vout;

    float3 normal = BiasX2(vin.Normal);

    normal = Skin(vin, normal, 1);

    CommonVSOutputPixelLighting cout = ComputeCommonVSOutputPixelLighting(vin.Position, normal);
    SetCommonVSOutputParamsPixelLighting;

    vout.Diffuse = float4(1, 1, 1, DiffuseColor.a);
    vout.TexCoord = vin.TexCoord;

    return vout;
}


// Vertex shader: pixel lighting, two bones.
[RootSignature(MainRS)]
VSOutputPixelLightingTx VSSkinnedPixelLightingTwoBones(VSInputNmTxWeights vin)
{
    VSOutputPixelLightingTx vout;

    float3 normal = Skin(vin, vin.Normal, 2);

    CommonVSOutputPixelLighting cout = ComputeCommonVSOutputPixelLighting(vin.Position, normal);
    SetCommonVSOutputParamsPixelLighting;

    vout.Diffuse = float4(1, 1, 1, DiffuseColor.a);
    vout.TexCoord = vin.TexCoord;

    return vout;
}

[RootSignature(MainRS)]
VSOutputPixelLightingTx VSSkinnedPixelLightingTwoBonesBn(VSInputNmTxWeights vin)
{
    VSOutputPixelLightingTx vout;

    float3 normal = BiasX2(vin.Normal);

    normal = Skin(vin, normal, 2);

    CommonVSOutputPixelLighting cout = ComputeCommonVSOutputPixelLighting(vin.Position, normal);
    SetCommonVSOutputParamsPixelLighting;

    vout.Diffuse = float4(1, 1, 1, DiffuseColor.a);
    vout.TexCoord = vin.TexCoord;

    return vout;
}


// Vertex shader: pixel lighting, four bones.
[RootSignature(MainRS)]
VSOutputPixelLightingTx VSSkinnedPixelLightingFourBones(VSInputNmTxWeights vin)
{
    VSOutputPixelLightingTx vout;

    float3 normal = Skin(vin, vin.Normal, 4);

    CommonVSOutputPixelLighting cout = ComputeCommonVSOutputPixelLighting(vin.Position, normal);
    SetCommonVSOutputParamsPixelLighting;

    vout.Diffuse = float4(1, 1, 1, DiffuseColor.a);
    vout.TexCoord = vin.TexCoord;

    return vout;
}

[RootSignature(MainRS)]
VSOutputPixelLightingTx VSSkinnedPixelLightingFourBonesBn(VSInputNmTxWeights vin)
{
    VSOutputPixelLightingTx vout;

    float3 normal = BiasX2(vin.Normal);

    normal = Skin(vin, normal, 4);

    CommonVSOutputPixelLighting cout = ComputeCommonVSOutputPixelLighting(vin.Position, normal);
    SetCommonVSOutputParamsPixelLighting;

    vout.Diffuse = float4(1, 1, 1, DiffuseColor.a);
    vout.TexCoord = vin.TexCoord;

    return vout;
}


// Pixel shader: vertex lighting.
[RootSignature(MainRS)]
float4 PSSkinnedVertexLighting(PSInputTx pin) : SV_Target0
{
    float4 color = Texture.Sample(Sampler, pin.TexCoord) * pin.Diffuse;

    AddSpecular(color, pin.Specular.rgb);
    ApplyFog(color, pin.Specular.w);

    return color;
}


// Pixel shader: vertex lighting, no fog.
[RootSignature(MainRS)]
float4 PSSkinnedVertexLightingNoFog(PSInputTx pin) : SV_Target0
{
    float4 color = Texture.Sample(Sampler, pin.TexCoord) * pin.Diffuse;

    AddSpecular(color, pin.Specular.rgb);

    return color;
}


// Pixel shader: pixel lighting.
[RootSignature(MainRS)]
float4 PSSkinnedPixelLighting(PSInputPixelLightingTx pin) : SV_Target0
{
    float4 color = Texture.Sample(Sampler, pin.TexCoord) * pin.Diffuse;

    float3 eyeVector = normalize(EyePosition - pin.PositionWS.xyz);
    float3 worldNormal = normalize(pin.NormalWS);

    ColorPair lightResult = ComputeLights(eyeVector, worldNormal, 3);

    color.rgb *= lightResult.Diffuse;

    AddSpecular(color, lightResult.Specular);
    ApplyFog(color, pin.PositionWS.w);

    return color;
}
