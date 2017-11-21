// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkID=615561


Texture2D<float4> AlbedoTexture : register(t0);
Texture2D<float3> NormalTexture : register(t1);
Texture2D<float3> RMATexture    : register(t2);

Texture2D<float3> EmissiveTexture : register(t3);

TextureCube<float3> RadianceTexture   : register(t4);
TextureCube<float3> IrradianceTexture : register(t5);

sampler SurfaceSampler : register(s0);
sampler IBLSampler     : register(s1);

cbuffer Constants : register(b0)
{
    float3   EyePosition            : packoffset(c0);
    float4x4 World                  : packoffset(c1);
    float3x3 WorldInverseTranspose  : packoffset(c5);
    float4x4 WorldViewProj          : packoffset(c8);
    float4x4 PrevWorldViewProj      : packoffset(c12);

    float3 LightDirection[3]        : packoffset(c16);
    float3 LightColor[3]            : packoffset(c19);   // "Specular and diffuse light" in PBR
 
    float3 ConstantAlbedo           : packoffset(c22);   // Constant values if not a textured effect
    float  ConstantMetallic         : packoffset(c23.x);
    float  ConstantRoughness        : packoffset(c23.y);

    int NumRadianceMipLevels        : packoffset(c23.z);

    // Size of render target
    float TargetWidth               : packoffset(c23.w);
    float TargetHeight              : packoffset(c24.x);
};


#include "Structures.fxh"
#include "PBRCommon.fxh"
#include "RootSig.fxh"
#include "Utilities.fxh"


// Vertex shader: pbr
[RootSignature(PBREffectRS)]
VSOutputPixelLightingTxTangent VSConstant(VSInputNmTxTangent vin)
{
    VSOutputPixelLightingTxTangent vout;

    CommonVSOutputPixelLighting cout = ComputeCommonVSOutputPixelLighting(vin.Position, vin.Normal);
    
    vout.PositionPS = cout.Pos_ps;
    vout.PositionWS = float4(cout.Pos_ws, 1);
    vout.NormalWS = cout.Normal_ws;
    vout.Diffuse = float4(ConstantAlbedo,1);
    vout.TexCoord = vin.TexCoord;
    vout.TangentWS = normalize(mul(vin.Tangent.xyz, WorldInverseTranspose));

    return vout;
}


// Vertex shader: pbr + velocity
[RootSignature(PBREffectRS)]
VSOut_Velocity VSConstantVelocity(VSInputNmTxTangent vin)
{
    VSOut_Velocity vout;

    CommonVSOutputPixelLighting cout = ComputeCommonVSOutputPixelLighting(vin.Position, vin.Normal);
    
    vout.current.PositionPS = cout.Pos_ps;
    vout.current.PositionWS = float4(cout.Pos_ws, 1);
    vout.current.NormalWS = cout.Normal_ws;
    vout.current.Diffuse = float4(ConstantAlbedo,1);
    vout.current.TexCoord = vin.TexCoord;
    vout.current.TangentWS = normalize(mul(vin.Tangent.xyz, WorldInverseTranspose));
    vout.prevPosition = mul(vin.Position, PrevWorldViewProj);

    return vout;
}


// Vertex shader: pbr (biased normal)
[RootSignature(PBREffectRS)]
VSOutputPixelLightingTxTangent VSConstantBn(VSInputNmTxTangent vin)
{
    VSOutputPixelLightingTxTangent vout;

    float3 normal = BiasX2(vin.Normal);

    CommonVSOutputPixelLighting cout = ComputeCommonVSOutputPixelLighting(vin.Position, normal);

    vout.PositionPS = cout.Pos_ps;
    vout.PositionWS = float4(cout.Pos_ws, 1);
    vout.NormalWS = cout.Normal_ws;
    vout.Diffuse = float4(ConstantAlbedo, 1);
    vout.TexCoord = vin.TexCoord;

    float3 tangent = BiasX2(vin.Tangent.xyz);
    vout.TangentWS = normalize(mul(tangent, WorldInverseTranspose));

    return vout;
}


// Vertex shader: pbr + velocity (biased normal)
[RootSignature(PBREffectRS)]
VSOut_Velocity VSConstantVelocityBn(VSInputNmTxTangent vin)
{
    VSOut_Velocity vout;

    float3 normal = BiasX2(vin.Normal);

    CommonVSOutputPixelLighting cout = ComputeCommonVSOutputPixelLighting(vin.Position, normal);

    vout.current.PositionPS = cout.Pos_ps;
    vout.current.PositionWS = float4(cout.Pos_ws, 1);
    vout.current.NormalWS = cout.Normal_ws;
    vout.current.Diffuse = float4(ConstantAlbedo, 1);
    vout.current.TexCoord = vin.TexCoord;

    float3 tangent = BiasX2(vin.Tangent.xyz);
    vout.current.TangentWS = normalize(mul(tangent, WorldInverseTranspose));

    vout.prevPosition = mul(vin.Position, PrevWorldViewProj);

    return vout;
}


// Pixel shader: pbr (constants) + image-based lighting
[RootSignature(PBREffectRS)]
float4 PSConstant(PSInputPixelLightingTxTangent pin) : SV_Target0
{
    // vectors
    const float3 V = normalize(EyePosition - pin.PositionWS.xyz);   // view vector
    const float3 N = normalize(pin.NormalWS);                       // surface normal
    const float AO = 1;                                             // ambient term

    float3 output = LightSurface(V, N, 3,
        LightColor, LightDirection,
        ConstantAlbedo, ConstantRoughness, ConstantMetallic, AO);

    return float4(output,1);
}


// Pixel shader: pbr (textures) + image-based lighting
[RootSignature(PBREffectRS)]
float4 PSTextured(PSInputPixelLightingTxTangent pin) : SV_Target0
{
    const float3 V = normalize(EyePosition - pin.PositionWS.xyz); // view vector
    const float3 L = normalize(-LightDirection[0]);               // light vector ("to light" opposite of light's direction)

    // Before lighting, peturb the surface's normal by the one given in normal map.
    float3 localNormal = BiasX2(NormalTexture.Sample(SurfaceSampler, pin.TexCoord).xyz);
    float3 N = PeturbNormal(localNormal, pin.NormalWS, pin.TangentWS);

    // Get albedo
    float3 albedo = AlbedoTexture.Sample(SurfaceSampler, pin.TexCoord).rgb;

    // Get roughness, metalness, and ambient occlusion
    float3 RMA = RMATexture.Sample(SurfaceSampler, pin.TexCoord);

    // glTF2 defines metalness as B channel, roughness as G channel, and occlusion as R channel

    // Shade surface
    float3 output = LightSurface(V, N, 3, LightColor, LightDirection, albedo, RMA.g, RMA.b, RMA.r);

    return float4(output, 1);
}


// Pixel shader: pbr (textures) + emissive + image-based lighting
[RootSignature(PBREffectRS)]
float4 PSTexturedEmissive(PSInputPixelLightingTxTangent pin) : SV_Target0
{
    const float3 V = normalize(EyePosition - pin.PositionWS.xyz); // view vector
    const float3 L = normalize(-LightDirection[0]);               // light vector ("to light" opposite of light's direction)

    // Before lighting, peturb the surface's normal by the one given in normal map.
    float3 localNormal = BiasX2(NormalTexture.Sample(SurfaceSampler, pin.TexCoord).xyz);
    float3 N = PeturbNormal(localNormal, pin.NormalWS, pin.TangentWS);

    // Get albedo
    float3 albedo = AlbedoTexture.Sample(SurfaceSampler, pin.TexCoord).rgb;

    // Get roughness, metalness, and ambient occlusion
    float3 RMA = RMATexture.Sample(SurfaceSampler, pin.TexCoord);

    // glTF2 defines metalness as B channel, roughness as G channel, and occlusion as R channel

    // Shade surface
    float3 output = LightSurface(V, N, 3, LightColor, LightDirection, albedo, RMA.g, RMA.b, RMA.r);

    output += EmissiveTexture.Sample(SurfaceSampler, pin.TexCoord).rgb;

    return float4(output, 1);
}


// Pixel shader: pbr (textures) + image-based lighting + velocity
#include "PixelPacking_Velocity.hlsli"

struct PSOut_Velocity
{
    float3 color : SV_Target0;
    packed_velocity_t velocity : SV_Target1;
};

[RootSignature(PBREffectRS)]
PSOut_Velocity PSTexturedVelocity(VSOut_Velocity pin)
{
    PSOut_Velocity output;

    const float3 V = normalize(EyePosition - pin.current.PositionWS.xyz); // view vector
    const float3 L = normalize(-LightDirection[0]);                       // light vector ("to light" opposite of light's direction)

    // Before lighting, peturb the surface's normal by the one given in normal map.
    float3 localNormal = BiasX2(NormalTexture.Sample(SurfaceSampler, pin.current.TexCoord).xyz);
    float3 N = PeturbNormal(localNormal, pin.current.NormalWS, pin.current.TangentWS);

    // Get albedo
    float3 albedo = AlbedoTexture.Sample(SurfaceSampler, pin.current.TexCoord).rgb;

    // Get roughness, metalness, and ambient occlusion
    float3 RMA = RMATexture.Sample(SurfaceSampler, pin.current.TexCoord);

    // glTF2 defines metalness as B channel, roughness as G channel, and occlusion as R channel

    // Shade surface
    output.color = LightSurface(V, N, 3, LightColor, LightDirection, albedo, RMA.g, RMA.b, RMA.r);

    // Calculate velocity of this point
    float4 prevPos = pin.prevPosition;
    prevPos.xyz /= prevPos.w;
    prevPos.xy *= float2(0.5f, -0.5f);
    prevPos.xy += 0.5f;
    prevPos.xy *= float2(TargetWidth, TargetHeight);

    output.velocity = PackVelocity(prevPos.xyz - pin.current.PositionPS.xyz);

    return output;
}

[RootSignature(PBREffectRS)]
PSOut_Velocity PSTexturedEmissiveVelocity(VSOut_Velocity pin)
{
    PSOut_Velocity output;

    const float3 V = normalize(EyePosition - pin.current.PositionWS.xyz); // view vector
    const float3 L = normalize(-LightDirection[0]);                       // light vector ("to light" opposite of light's direction)

    // Before lighting, peturb the surface's normal by the one given in normal map.
    float3 localNormal = BiasX2(NormalTexture.Sample(SurfaceSampler, pin.current.TexCoord).xyz);
    float3 N = PeturbNormal(localNormal, pin.current.NormalWS, pin.current.TangentWS);

    // Get albedo
    float3 albedo = AlbedoTexture.Sample(SurfaceSampler, pin.current.TexCoord).rgb;

    // Get roughness, metalness, and ambient occlusion
    float3 RMA = RMATexture.Sample(SurfaceSampler, pin.current.TexCoord);

    // glTF2 defines metalness as B channel, roughness as G channel, and occlusion as R channel

    // Shade surface
    output.color = LightSurface(V, N, 3, LightColor, LightDirection, albedo, RMA.g, RMA.b, RMA.r);

    output.color += EmissiveTexture.Sample(SurfaceSampler, pin.current.TexCoord).rgb;

    // Calculate velocity of this point
    float4 prevPos = pin.prevPosition;
    prevPos.xyz /= prevPos.w;
    prevPos.xy *= float2(0.5f, -0.5f);
    prevPos.xy += 0.5f;
    prevPos.xy *= float2(TargetWidth, TargetHeight);

    output.velocity = PackVelocity(prevPos.xyz - pin.current.PositionPS.xyz);

    return output;
}