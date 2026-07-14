//--------------------------------------------------------------------------------------
// File: NPREffect.fx
//
// Non-photorealistic rendering effects (cel shading and Gooch shading)
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// https://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------


Texture2D<float4> Texture : register(t0);
SamplerState Sampler : register(s0);


cbuffer Parameters : register(b0)
{
    float3 LightDirection            : packoffset(c0);
    float  CelBands                  : packoffset(c0.w);

    float3 DiffuseColor              : packoffset(c1);
    float  Alpha                     : packoffset(c1.w);

    float3 SpecularColor             : packoffset(c2);
    float  SpecularPower             : packoffset(c2.w);

    float3 GoochCoolColor            : packoffset(c3);
    float  GoochAlpha                : packoffset(c3.w);

    float3 GoochWarmColor            : packoffset(c4);
    float  GoochBeta                 : packoffset(c4.w);

    float3 EyePosition               : packoffset(c5);

    float4x4 World                   : packoffset(c6);
    float3x3 WorldInverseTranspose   : packoffset(c10);
    float4x4 WorldViewProj           : packoffset(c13);
};


#include "RootSig.fxh"
#include "Structures.fxh"
#include "Utilities.fxh"

// Vertex shader: basic.
[RootSignature(NoTextureRS)]
VSOutputPixelLighting VSNPREffect(VSInputNm vin)
{
    VSOutputPixelLighting vout;

    vout.PositionPS = mul(vin.Position, WorldViewProj);
    vout.PositionWS = float4(mul(vin.Position, World).xyz, 1);
    vout.NormalWS = normalize(mul(vin.Normal, WorldInverseTranspose));
    vout.Diffuse = float4(DiffuseColor, Alpha);

    return vout;
}

[RootSignature(NoTextureRS)]
VSOutputPixelLighting VSNPREffectBn(VSInputNm vin)
{
    VSOutputPixelLighting vout;

    float3 normal = BiasX2(vin.Normal);

    vout.PositionPS = mul(vin.Position, WorldViewProj);
    vout.PositionWS = float4(mul(vin.Position, World).xyz, 1);
    vout.NormalWS = normalize(mul(normal, WorldInverseTranspose));
    vout.Diffuse = float4(DiffuseColor, Alpha);

    return vout;
}


// Vertex shader: vertex color.
[RootSignature(NoTextureRS)]
VSOutputPixelLighting VSNPREffectVc(VSInputNmVc vin)
{
    VSOutputPixelLighting vout;

    vout.PositionPS = mul(vin.Position, WorldViewProj);
    vout.PositionWS = float4(mul(vin.Position, World).xyz, 1);
    vout.NormalWS = normalize(mul(vin.Normal, WorldInverseTranspose));
    vout.Diffuse.rgb = vin.Color.rgb * DiffuseColor;
    vout.Diffuse.a = vin.Color.a * Alpha;

    return vout;
}

[RootSignature(NoTextureRS)]
VSOutputPixelLighting VSNPREffectVcBn(VSInputNmVc vin)
{
    VSOutputPixelLighting vout;

    float3 normal = BiasX2(vin.Normal);

    vout.PositionPS = mul(vin.Position, WorldViewProj);
    vout.PositionWS = float4(mul(vin.Position, World).xyz, 1);
    vout.NormalWS = normalize(mul(normal, WorldInverseTranspose));
    vout.Diffuse.rgb = vin.Color.rgb * DiffuseColor;
    vout.Diffuse.a = vin.Color.a * Alpha;

    return vout;
}


// Vertex shader: instancing.
[RootSignature(NoTextureRS)]
VSOutputPixelLighting VSNPREffectInst(VSInputNmInst vin)
{
    VSOutputPixelLighting vout;

    CommonInstancing inst = ComputeCommonInstancing(vin.Position, vin.Normal, vin.Transform);

    vout.PositionPS = mul(inst.Position, WorldViewProj);
    vout.PositionWS = float4(mul(inst.Position, World).xyz, 1);
    vout.NormalWS = normalize(mul(inst.Normal, WorldInverseTranspose));
    vout.Diffuse = float4(DiffuseColor, Alpha);

    return vout;
}

[RootSignature(NoTextureRS)]
VSOutputPixelLighting VSNPREffectBnInst(VSInputNmInst vin)
{
    VSOutputPixelLighting vout;

    float3 normal = BiasX2(vin.Normal);

    CommonInstancing inst = ComputeCommonInstancing(vin.Position, normal, vin.Transform);

    vout.PositionPS = mul(inst.Position, WorldViewProj);
    vout.PositionWS = float4(mul(inst.Position, World).xyz, 1);
    vout.NormalWS = normalize(mul(inst.Normal, WorldInverseTranspose));
    vout.Diffuse = float4(DiffuseColor, Alpha);

    return vout;
}


// Vertex shader: vertex color + instancing.
[RootSignature(NoTextureRS)]
VSOutputPixelLighting VSNPREffectVcInst(VSInputNmVcInst vin)
{
    VSOutputPixelLighting vout;

    CommonInstancing inst = ComputeCommonInstancing(vin.Position, vin.Normal, vin.Transform);

    vout.PositionPS = mul(inst.Position, WorldViewProj);
    vout.PositionWS = float4(mul(inst.Position, World).xyz, 1);
    vout.NormalWS = normalize(mul(inst.Normal, WorldInverseTranspose));
    vout.Diffuse.rgb = vin.Color.rgb * DiffuseColor;
    vout.Diffuse.a = vin.Color.a * Alpha;

    return vout;
}

[RootSignature(NoTextureRS)]
VSOutputPixelLighting VSNPREffectVcBnInst(VSInputNmVcInst vin)
{
    VSOutputPixelLighting vout;

    float3 normal = BiasX2(vin.Normal);

    CommonInstancing inst = ComputeCommonInstancing(vin.Position, normal, vin.Transform);

    vout.PositionPS = mul(inst.Position, WorldViewProj);
    vout.PositionWS = float4(mul(inst.Position, World).xyz, 1);
    vout.NormalWS = normalize(mul(inst.Normal, WorldInverseTranspose));
    vout.Diffuse.rgb = vin.Color.rgb * DiffuseColor;
    vout.Diffuse.a = vin.Color.a * Alpha;

    return vout;
}


// Vertex shader: texture.
[RootSignature(MainRS)]
VSOutputPixelLightingTx VSNPREffectTx(VSInputNmTx vin)
{
    VSOutputPixelLightingTx vout;

    vout.PositionPS = mul(vin.Position, WorldViewProj);
    vout.PositionWS = float4(mul(vin.Position, World).xyz, 1);
    vout.NormalWS = normalize(mul(vin.Normal, WorldInverseTranspose));
    vout.Diffuse = float4(DiffuseColor, Alpha);
    vout.TexCoord = vin.TexCoord;

    return vout;
}

[RootSignature(MainRS)]
VSOutputPixelLightingTx VSNPREffectBnTx(VSInputNmTx vin)
{
    VSOutputPixelLightingTx vout;

    float3 normal = BiasX2(vin.Normal);

    vout.PositionPS = mul(vin.Position, WorldViewProj);
    vout.PositionWS = float4(mul(vin.Position, World).xyz, 1);
    vout.NormalWS = normalize(mul(normal, WorldInverseTranspose));
    vout.Diffuse = float4(DiffuseColor, Alpha);
    vout.TexCoord = vin.TexCoord;

    return vout;
}


// Vertex shader: texture + vertex color.
[RootSignature(MainRS)]
VSOutputPixelLightingTx VSNPREffectVcTx(VSInputNmTxVc vin)
{
    VSOutputPixelLightingTx vout;

    vout.PositionPS = mul(vin.Position, WorldViewProj);
    vout.PositionWS = float4(mul(vin.Position, World).xyz, 1);
    vout.NormalWS = normalize(mul(vin.Normal, WorldInverseTranspose));
    vout.Diffuse.rgb = vin.Color.rgb * DiffuseColor;
    vout.Diffuse.a = vin.Color.a * Alpha;
    vout.TexCoord = vin.TexCoord;

    return vout;
}

[RootSignature(MainRS)]
VSOutputPixelLightingTx VSNPREffectVcBnTx(VSInputNmTxVc vin)
{
    VSOutputPixelLightingTx vout;

    float3 normal = BiasX2(vin.Normal);

    vout.PositionPS = mul(vin.Position, WorldViewProj);
    vout.PositionWS = float4(mul(vin.Position, World).xyz, 1);
    vout.NormalWS = normalize(mul(normal, WorldInverseTranspose));
    vout.Diffuse.rgb = vin.Color.rgb * DiffuseColor;
    vout.Diffuse.a = vin.Color.a * Alpha;
    vout.TexCoord = vin.TexCoord;

    return vout;
}


// Vertex shader: texture + instancing.
[RootSignature(MainRS)]
VSOutputPixelLightingTx VSNPREffectInstTx(VSInputNmTxInst vin)
{
    VSOutputPixelLightingTx vout;

    CommonInstancing inst = ComputeCommonInstancing(vin.Position, vin.Normal, vin.Transform);

    vout.PositionPS = mul(inst.Position, WorldViewProj);
    vout.PositionWS = float4(mul(inst.Position, World).xyz, 1);
    vout.NormalWS = normalize(mul(inst.Normal, WorldInverseTranspose));
    vout.Diffuse = float4(DiffuseColor, Alpha);
    vout.TexCoord = vin.TexCoord;

    return vout;
}

[RootSignature(MainRS)]
VSOutputPixelLightingTx VSNPREffectBnInstTx(VSInputNmTxInst vin)
{
    VSOutputPixelLightingTx vout;

    float3 normal = BiasX2(vin.Normal);

    CommonInstancing inst = ComputeCommonInstancing(vin.Position, normal, vin.Transform);

    vout.PositionPS = mul(inst.Position, WorldViewProj);
    vout.PositionWS = float4(mul(inst.Position, World).xyz, 1);
    vout.NormalWS = normalize(mul(inst.Normal, WorldInverseTranspose));
    vout.Diffuse = float4(DiffuseColor, Alpha);
    vout.TexCoord = vin.TexCoord;

    return vout;
}


// Vertex shader: texture + vertex color + instancing.
[RootSignature(MainRS)]
VSOutputPixelLightingTx VSNPREffectVcInstTx(VSInputNmTxVcInst vin)
{
    VSOutputPixelLightingTx vout;

    CommonInstancing inst = ComputeCommonInstancing(vin.Position, vin.Normal, vin.Transform);

    vout.PositionPS = mul(inst.Position, WorldViewProj);
    vout.PositionWS = float4(mul(inst.Position, World).xyz, 1);
    vout.NormalWS = normalize(mul(inst.Normal, WorldInverseTranspose));
    vout.Diffuse.rgb = vin.Color.rgb * DiffuseColor;
    vout.Diffuse.a = vin.Color.a * Alpha;
    vout.TexCoord = vin.TexCoord;

    return vout;
}

[RootSignature(MainRS)]
VSOutputPixelLightingTx VSNPREffectVcBnInstTx(VSInputNmTxVcInst vin)
{
    VSOutputPixelLightingTx vout;

    float3 normal = BiasX2(vin.Normal);

    CommonInstancing inst = ComputeCommonInstancing(vin.Position, normal, vin.Transform);

    vout.PositionPS = mul(inst.Position, WorldViewProj);
    vout.PositionWS = float4(mul(inst.Position, World).xyz, 1);
    vout.NormalWS = normalize(mul(inst.Normal, WorldInverseTranspose));
    vout.Diffuse.rgb = vin.Color.rgb * DiffuseColor;
    vout.Diffuse.a = vin.Color.a * Alpha;
    vout.TexCoord = vin.TexCoord;

    return vout;
}


// Pixel shader: cel shading.
[RootSignature(NoTextureRS)]
float4 PSCelShading(PSInputPixelLighting pin) : SV_Target0
{
    float3 normal = normalize(pin.NormalWS);
    float3 lightDir = normalize(-LightDirection);

    // Quantize the diffuse lighting into discrete bands
    float NdotL = dot(normal, lightDir);
    float intensity = max(0, NdotL);
    float quantized = floor(intensity * CelBands) / CelBands;

    float3 color = pin.Diffuse.rgb * quantized;

    // Specular highlight (hard edge)
    float3 viewDir = normalize(EyePosition - pin.PositionWS.xyz);
    float3 halfVec = normalize(lightDir + viewDir);
    float NdotH = max(0, dot(normal, halfVec));

    //TODO: Need to revisit for cel
    float specular = step(0.95, pow(NdotH, SpecularPower));

    color += SpecularColor * specular;

    return float4(color, pin.Diffuse.a);
}


// Pixel shader: cel shading + texture.
[RootSignature(MainRS)]
float4 PSCelShadingTx(PSInputPixelLightingTx pin) : SV_Target0
{
    float3 normal = normalize(pin.NormalWS);
    float3 lightDir = normalize(-LightDirection);

    // Sample base texture
    float4 texColor = Texture.Sample(Sampler, pin.TexCoord);

    // Quantize the diffuse lighting into discrete bands
    float NdotL = dot(normal, lightDir);
    float intensity = max(0, NdotL);
    float quantized = floor(intensity * CelBands) / CelBands;

    float3 color = pin.Diffuse.rgb * texColor.rgb * quantized;

    // Specular highlight (hard edge)
    float3 viewDir = normalize(EyePosition - pin.PositionWS.xyz);
    float3 halfVec = normalize(lightDir + viewDir);
    float NdotH = max(0, dot(normal, halfVec));

    //TODO: Need to revisit for cel
    float specular = step(0.95, pow(NdotH, SpecularPower));

    color += SpecularColor * specular;

    return float4(color, pin.Diffuse.a * texColor.a);
}


// Pixel shader: Gooch shading.
[RootSignature(NoTextureRS)]
float4 PSGoochShading(PSInputPixelLighting pin) : SV_Target0
{
    float3 normal = normalize(pin.NormalWS);
    float3 lightDir = normalize(-LightDirection);

    // Gooch diffuse term: blend between cool and warm based on NdotL
    float NdotL = dot(normal, lightDir);
    float t = (1.0 + NdotL) * 0.5;

    float3 coolContrib = GoochCoolColor + GoochAlpha * pin.Diffuse.rgb;
    float3 warmContrib = GoochWarmColor + GoochBeta * pin.Diffuse.rgb;

    float3 color = lerp(coolContrib, warmContrib, t);

    // Specular highlight
    float3 viewDir = normalize(EyePosition - pin.PositionWS.xyz);
    float3 reflectDir = reflect(LightDirection, normal);

    //TODO: Need to revisit for Gooch
    float specular = pow(max(0, dot(viewDir, reflectDir)), SpecularPower);

    color += SpecularColor * specular;

    return float4(color, pin.Diffuse.a);
}


// Pixel shader: Gooch shading + texture.
[RootSignature(MainRS)]
float4 PSGoochShadingTx(PSInputPixelLightingTx pin) : SV_Target0
{
    float3 normal = normalize(pin.NormalWS);
    float3 lightDir = normalize(-LightDirection);

    // Sample base texture
    float4 texColor = Texture.Sample(Sampler, pin.TexCoord);

    // Gooch diffuse term: blend between cool and warm based on NdotL
    float NdotL = dot(normal, lightDir);
    float t = (1.0 + NdotL) * 0.5;

    float3 coolContrib = GoochCoolColor + GoochAlpha * pin.Diffuse.rgb * texColor.rgb;
    float3 warmContrib = GoochWarmColor + GoochBeta * pin.Diffuse.rgb * texColor.rgb;

    float3 color = lerp(coolContrib, warmContrib, t);

    // Specular highlight
    float3 viewDir = normalize(EyePosition - pin.PositionWS.xyz);
    float3 reflectDir = reflect(LightDirection, normal);

    //TODO: Need to revisit for Gooch
    float specular = pow(max(0, dot(viewDir, reflectDir)), SpecularPower);

    color += SpecularColor * specular;

    return float4(color, pin.Diffuse.a * texColor.a);
}
