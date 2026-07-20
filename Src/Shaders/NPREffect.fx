// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// https://go.microsoft.com/fwlink/?LinkID=615561


Texture2D<float4> Texture : register(t0);
SamplerState Sampler : register(s0);
Texture2D<float4> Texture2 : register(t1);


cbuffer Parameters : register(b0)
{
    float3 LightDirection            : packoffset(c0);
    float  CelBands                  : packoffset(c0.w);

    float3 DiffuseColor              : packoffset(c1);
    float  Alpha                     : packoffset(c1.w);

    float3 SpecularColor             : packoffset(c2);
    float  SpecularThreshold         : packoffset(c2.w);

    float3 RimColor                  : packoffset(c3);
    float  RimPower                  : packoffset(c3.w);

    float  SpecularSmoothing         : packoffset(c4.x);
    float  RimStrength               : packoffset(c4.y);
    float  RimStart                  : packoffset(c4.z);
    float  RimEnd                    : packoffset(c4.w);

    float3 GoochCoolColor            : packoffset(c5);
    float  GoochAlpha                : packoffset(c5.w);

    float3 GoochWarmColor            : packoffset(c6);
    float  GoochBeta                 : packoffset(c6.w);

    float3 EyePosition               : packoffset(c7);

    float4x4 World                   : packoffset(c8);
    float3x3 WorldInverseTranspose   : packoffset(c12);
    float4x4 WorldViewProj           : packoffset(c15);
};

cbuffer SkinningParameters : register(b1)
{
    float4x3 Bones[72];
}


#include "RootSig.fxh"
#include "Structures.fxh"
#include "Skinning.fxh"
#include "Utilities.fxh"


VSOutputPixelLighting ComputeCommonVS(float4 position, float3 normal)
{
    VSOutputPixelLighting vout;
    vout.PositionPS = mul(position, WorldViewProj);
    vout.PositionWS = float4(mul(position, World).xyz, 1);
    vout.NormalWS = normalize(mul(normal, WorldInverseTranspose));
    return vout;
}

VSOutputPixelLightingTx ComputeCommonVSTx(float4 position, float3 normal, float2 texcoord)
{
    VSOutputPixelLightingTx vout;
    vout.TexCoord = texcoord;
    vout.PositionPS = mul(position, WorldViewProj);
    vout.PositionWS = float4(mul(position, World).xyz, 1);
    vout.NormalWS = normalize(mul(normal, WorldInverseTranspose));
    return vout;
}

// Vertex shader: basic.
[RootSignature(NoTextureRS)]
VSOutputPixelLighting VSNPREffect(VSInputNm vin)
{
    VSOutputPixelLighting vout = ComputeCommonVS(vin.Position, vin.Normal);
    vout.Diffuse = float4(DiffuseColor, Alpha);
    return vout;
}

[RootSignature(MainRS)]
VSOutputPixelLighting VSNPREffectMC(VSInputNm vin)
{
    return VSNPREffect(vin);
}

[RootSignature(NoTextureRS)]
VSOutputPixelLighting VSNPREffectBn(VSInputNm vin)
{
    float3 normal = BiasX2(vin.Normal);

    VSOutputPixelLighting vout = ComputeCommonVS(vin.Position, normal);
    vout.Diffuse = float4(DiffuseColor, Alpha);
    return vout;
}

[RootSignature(MainRS)]
VSOutputPixelLighting VSNPREffectBnMC(VSInputNm vin)
{
    return VSNPREffectBn(vin);
}


// Vertex shader: vertex color.
[RootSignature(NoTextureRS)]
VSOutputPixelLighting VSNPREffectVc(VSInputNmVc vin)
{
    VSOutputPixelLighting vout = ComputeCommonVS(vin.Position, vin.Normal);
    vout.Diffuse.rgb = vin.Color.rgb * DiffuseColor;
    vout.Diffuse.a = vin.Color.a * Alpha;
    return vout;
}

[RootSignature(MainRS)]
VSOutputPixelLighting VSNPREffectVcMC(VSInputNmVc vin)
{
    return VSNPREffectVc(vin);
}

[RootSignature(NoTextureRS)]
VSOutputPixelLighting VSNPREffectVcBn(VSInputNmVc vin)
{
    float3 normal = BiasX2(vin.Normal);

    VSOutputPixelLighting vout = ComputeCommonVS(vin.Position, normal);
    vout.Diffuse.rgb = vin.Color.rgb * DiffuseColor;
    vout.Diffuse.a = vin.Color.a * Alpha;
    return vout;
}

[RootSignature(MainRS)]
VSOutputPixelLighting VSNPREffectVcBnMC(VSInputNmVc vin)
{
    return VSNPREffectVcBn(vin);

}


// Vertex shader: instancing.
[RootSignature(NoTextureRS)]
VSOutputPixelLighting VSNPREffectInst(VSInputNmInst vin)
{
    CommonInstancing inst = ComputeCommonInstancing(vin.Position, vin.Normal, vin.Transform);
    VSOutputPixelLighting vout = ComputeCommonVS(inst.Position, inst.Normal);
    vout.Diffuse = float4(DiffuseColor, Alpha);
    return vout;
}

[RootSignature(MainRS)]
VSOutputPixelLighting VSNPREffectInstMC(VSInputNmInst vin)
{
    return VSNPREffectInst(vin);
}

[RootSignature(NoTextureRS)]
VSOutputPixelLighting VSNPREffectBnInst(VSInputNmInst vin)
{
    float3 normal = BiasX2(vin.Normal);

    CommonInstancing inst = ComputeCommonInstancing(vin.Position, normal, vin.Transform);
    VSOutputPixelLighting vout = ComputeCommonVS(inst.Position, inst.Normal);
    vout.Diffuse = float4(DiffuseColor, Alpha);
    return vout;
}

[RootSignature(MainRS)]
VSOutputPixelLighting VSNPREffectBnInstMC(VSInputNmInst vin)
{
    return VSNPREffectBnInst(vin);
}


// Vertex shader: vertex color + instancing.
[RootSignature(NoTextureRS)]
VSOutputPixelLighting VSNPREffectVcInst(VSInputNmVcInst vin)
{
    CommonInstancing inst = ComputeCommonInstancing(vin.Position, vin.Normal, vin.Transform);
    VSOutputPixelLighting vout = ComputeCommonVS(inst.Position, inst.Normal);
    vout.Diffuse.rgb = vin.Color.rgb * DiffuseColor;
    vout.Diffuse.a = vin.Color.a * Alpha;
    return vout;
}

[RootSignature(MainRS)]
VSOutputPixelLighting VSNPREffectVcInstMC(VSInputNmVcInst vin)
{
    return VSNPREffectVcInst(vin);
}

[RootSignature(NoTextureRS)]
VSOutputPixelLighting VSNPREffectVcBnInst(VSInputNmVcInst vin)
{
    float3 normal = BiasX2(vin.Normal);

    CommonInstancing inst = ComputeCommonInstancing(vin.Position, normal, vin.Transform);
    VSOutputPixelLighting vout = ComputeCommonVS(inst.Position, inst.Normal);
    vout.Diffuse.rgb = vin.Color.rgb * DiffuseColor;
    vout.Diffuse.a = vin.Color.a * Alpha;
    return vout;
}

[RootSignature(MainRS)]
VSOutputPixelLighting VSNPREffectVcBnInstMC(VSInputNmVcInst vin)
{
    return VSNPREffectVcBnInst(vin);

}


// Vertex shader: texture.
[RootSignature(MainRS)]
VSOutputPixelLightingTx VSNPREffectTx(VSInputNmTx vin)
{
    VSOutputPixelLightingTx vout = ComputeCommonVSTx(vin.Position, vin.Normal, vin.TexCoord);
    vout.Diffuse = float4(DiffuseColor, Alpha);
    return vout;
}

[RootSignature(DualTextureOneSamplerRS)]
VSOutputPixelLightingTx VSNPREffectTxMC(VSInputNmTx vin)
{
    return VSNPREffectTx(vin);
}

[RootSignature(MainRS)]
VSOutputPixelLightingTx VSNPREffectBnTx(VSInputNmTx vin)
{
    float3 normal = BiasX2(vin.Normal);

    VSOutputPixelLightingTx vout = ComputeCommonVSTx(vin.Position, normal, vin.TexCoord);
    vout.Diffuse = float4(DiffuseColor, Alpha);
    return vout;
}

[RootSignature(DualTextureOneSamplerRS)]
VSOutputPixelLightingTx VSNPREffectBnTxMC(VSInputNmTx vin)
{
    return VSNPREffectBnTx(vin);

}


// Vertex shader: texture + vertex color.
[RootSignature(MainRS)]
VSOutputPixelLightingTx VSNPREffectVcTx(VSInputNmTxVc vin)
{
    VSOutputPixelLightingTx vout = ComputeCommonVSTx(vin.Position, vin.Normal, vin.TexCoord);
    vout.Diffuse.rgb = vin.Color.rgb * DiffuseColor;
    vout.Diffuse.a = vin.Color.a * Alpha;
    return vout;
}

[RootSignature(DualTextureOneSamplerRS)]
VSOutputPixelLightingTx VSNPREffectVcTxMC(VSInputNmTxVc vin)
{
    return VSNPREffectVcTx(vin);
}

[RootSignature(MainRS)]
VSOutputPixelLightingTx VSNPREffectVcBnTx(VSInputNmTxVc vin)
{
    float3 normal = BiasX2(vin.Normal);

    VSOutputPixelLightingTx vout = ComputeCommonVSTx(vin.Position, normal, vin.TexCoord);
    vout.Diffuse.rgb = vin.Color.rgb * DiffuseColor;
    vout.Diffuse.a = vin.Color.a * Alpha;
    return vout;
}

[RootSignature(DualTextureOneSamplerRS)]
VSOutputPixelLightingTx VSNPREffectVcBnTxMC(VSInputNmTxVc vin)
{
    return VSNPREffectVcBnTx(vin);

}


// Vertex shader: texture + instancing.
[RootSignature(MainRS)]
VSOutputPixelLightingTx VSNPREffectInstTx(VSInputNmTxInst vin)
{
    CommonInstancing inst = ComputeCommonInstancing(vin.Position, vin.Normal, vin.Transform);
    VSOutputPixelLightingTx vout = ComputeCommonVSTx(inst.Position, inst.Normal, vin.TexCoord);
    vout.Diffuse = float4(DiffuseColor, Alpha);
    return vout;
}

[RootSignature(DualTextureOneSamplerRS)]
VSOutputPixelLightingTx VSNPREffectInstTxMC(VSInputNmTxInst vin)
{
    return VSNPREffectInstTx(vin);
}

[RootSignature(MainRS)]
VSOutputPixelLightingTx VSNPREffectBnInstTx(VSInputNmTxInst vin)
{
    float3 normal = BiasX2(vin.Normal);

    CommonInstancing inst = ComputeCommonInstancing(vin.Position, normal, vin.Transform);
    VSOutputPixelLightingTx vout = ComputeCommonVSTx(inst.Position, inst.Normal, vin.TexCoord);
    vout.Diffuse = float4(DiffuseColor, Alpha);
    return vout;
}

[RootSignature(DualTextureOneSamplerRS)]
VSOutputPixelLightingTx VSNPREffectBnInstTxMC(VSInputNmTxInst vin)
{
    return VSNPREffectBnInstTx(vin);

}


// Vertex shader: texture + vertex color + instancing.
[RootSignature(MainRS)]
VSOutputPixelLightingTx VSNPREffectVcInstTx(VSInputNmTxVcInst vin)
{
    CommonInstancing inst = ComputeCommonInstancing(vin.Position, vin.Normal, vin.Transform);
    VSOutputPixelLightingTx vout = ComputeCommonVSTx(inst.Position, inst.Normal, vin.TexCoord);
    vout.Diffuse.rgb = vin.Color.rgb * DiffuseColor;
    vout.Diffuse.a = vin.Color.a * Alpha;
    return vout;
}

[RootSignature(DualTextureOneSamplerRS)]
VSOutputPixelLightingTx VSNPREffectVcInstTxMC(VSInputNmTxVcInst vin)
{
    return VSNPREffectVcInstTx(vin);
}

[RootSignature(MainRS)]
VSOutputPixelLightingTx VSNPREffectVcBnInstTx(VSInputNmTxVcInst vin)
{
    float3 normal = BiasX2(vin.Normal);

    CommonInstancing inst = ComputeCommonInstancing(vin.Position, normal, vin.Transform);
    VSOutputPixelLightingTx vout = ComputeCommonVSTx(inst.Position, inst.Normal, vin.TexCoord);
    vout.Diffuse.rgb = vin.Color.rgb * DiffuseColor;
    vout.Diffuse.a = vin.Color.a * Alpha;
    return vout;
}

[RootSignature(DualTextureOneSamplerRS)]
VSOutputPixelLightingTx VSNPREffectVcBnInstTxMC(VSInputNmTxVcInst vin)
{
    return VSNPREffectVcBnInstTx(vin);

}


// Vertex shader: skinning (four bones) + texture.
[RootSignature(SkinTextureSamplerRS)]
VSOutputPixelLightingTx VSSkinnedNPREffectTx(VSInputNmTxWeights vin)
{
    float3 normal = Skin(vin, vin.Normal, 4);

    VSOutputPixelLightingTx vout = ComputeCommonVSTx(vin.Position, normal, vin.TexCoord);
    vout.Diffuse = float4(DiffuseColor, Alpha);
    return vout;
}

[RootSignature(SkinDualTextureOneSamplerRS)]
VSOutputPixelLightingTx VSSkinnedNPREffectTxMC(VSInputNmTxWeights vin)
{
    return VSSkinnedNPREffectTx(vin);
}

[RootSignature(SkinTextureSamplerRS)]
VSOutputPixelLightingTx VSSkinnedNPREffectTxBn(VSInputNmTxWeights vin)
{
    float3 normal = BiasX2(vin.Normal);

    normal = Skin(vin, normal, 4);

    VSOutputPixelLightingTx vout = ComputeCommonVSTx(vin.Position, normal, vin.TexCoord);
    vout.Diffuse = float4(DiffuseColor, Alpha);
    return vout;
}

[RootSignature(SkinDualTextureOneSamplerRS)]
VSOutputPixelLightingTx VSSkinnedNPREffectTxBnMC(VSInputNmTxWeights vin)
{
    return VSSkinnedNPREffectTxBn(vin);
}


//--- Cel shading ---
float Quantize(float intensity, float bands)
{
    intensity = max(0, intensity);
    return floor(intensity * bands) / bands;
}

float HardEdgeSpecular(float dot1)
{
    return smoothstep(SpecularThreshold - SpecularSmoothing, SpecularThreshold + SpecularSmoothing, dot1);
}

float RimLighting(float dot1)
{
    float fresnel = pow(1.0 - saturate(dot1), RimPower);
    return smoothstep(RimStart, RimEnd, fresnel) * RimStrength;
}


// Pixel shader: cel shading.
[RootSignature(NoTextureRS)]
float4 PSCelShading(PSInputPixelLighting pin) : SV_Target0
{
    float3 normal = normalize(pin.NormalWS);

    float3 lightDir = normalize(-LightDirection);
    float NdotL = dot(normal, lightDir);

    float3 viewDir = normalize(EyePosition - pin.PositionWS.xyz);
    float NdotV = max(0, dot(normal, viewDir));

    float3 halfVec = normalize(lightDir + viewDir);
    float NdotH = max(0, dot(normal, halfVec));

    // Quantize the diffuse lighting into discrete bands
    float quantized = Quantize(NdotL, CelBands);
    float3 color = pin.Diffuse.rgb * quantized;

    // Rim lighting
    float outline = RimLighting(NdotV);
    color = lerp(color, RimColor, outline);

    // Specular highlight (hard edge)
    float specular = HardEdgeSpecular(NdotH);
    color += SpecularColor * specular;

    return float4(color, pin.Diffuse.a);
}


// Pixel shader: cel shading + texture.
[RootSignature(MainRS)]
float4 PSCelShadingTx(PSInputPixelLightingTx pin) : SV_Target0
{
    float3 normal = normalize(pin.NormalWS);

    float3 lightDir = normalize(-LightDirection);
    float NdotL = dot(normal, lightDir);

    float3 viewDir = normalize(EyePosition - pin.PositionWS.xyz);
    float NdotV = max(0, dot(normal, viewDir));
    float VdotL = max(0, dot(viewDir, lightDir));

    float3 halfVec = normalize(lightDir + viewDir);
    float NdotH = max(0, dot(normal, halfVec));

    // Sample base texture
    float4 texColor = Texture.Sample(Sampler, pin.TexCoord);

    // Quantize the diffuse lighting into discrete bands
    float quantized = Quantize(NdotL, CelBands);
    float3 color = pin.Diffuse.rgb * texColor.rgb * quantized;

    // Rim lighting
    float outline = RimLighting(NdotV);
    color = lerp(color, RimColor, outline);

    // Specular highlight (hard edge)
    float specular = HardEdgeSpecular(NdotH);
    color += SpecularColor * specular;

    return float4(color, pin.Diffuse.a * texColor.a);
}

[RootSignature(SkinTextureSamplerRS)]
float4 PSCelShadingSkinTx(PSInputPixelLightingTx pin) : SV_Target0
{
    return PSCelShadingTx(pin);
}

//--- Gooch shading ---
float3 GoochShading(float dot1, float3 color)
{
    float t = (1.0 + dot1) * 0.5;

    float3 coolContrib = GoochCoolColor + GoochAlpha * color;
    float3 warmContrib = GoochWarmColor + GoochBeta * color;

    return lerp(coolContrib, warmContrib, t);
}


// Pixel shader: Gooch shading.
[RootSignature(NoTextureRS)]
float4 PSGoochShading(PSInputPixelLighting pin) : SV_Target0
{
    float3 normal = normalize(pin.NormalWS);
    float3 lightDir = normalize(-LightDirection);
    float NdotL = dot(normal, lightDir);

    float3 viewDir = normalize(EyePosition - pin.PositionWS.xyz);
    float NdotV = max(0, dot(normal, viewDir));

    float3 reflectDir = reflect(LightDirection, normal);
    float RdotV = max(0, dot(viewDir, reflectDir));

    // Gooch diffuse term: blend between cool and warm based
    float3 color = GoochShading(NdotL, pin.Diffuse.rgb);

    // Rim lighting
    float outline = RimLighting(NdotV);
    color += lerp(color, RimColor, outline);

    // Specular highlight (hard edge)
    float specular = HardEdgeSpecular(RdotV);
    color += SpecularColor * specular;

    return float4(color, pin.Diffuse.a);
}


// Pixel shader: Gooch shading + texture.
[RootSignature(MainRS)]
float4 PSGoochShadingTx(PSInputPixelLightingTx pin) : SV_Target0
{
    float3 normal = normalize(pin.NormalWS);
    float3 lightDir = normalize(-LightDirection);
    float NdotL = dot(normal, lightDir);

    float3 viewDir = normalize(EyePosition - pin.PositionWS.xyz);
    float NdotV = max(0, dot(normal, viewDir));

    float3 reflectDir = reflect(LightDirection, normal);
    float RdotV = max(0, dot(viewDir, reflectDir));

    // Sample base texture
    float4 texColor = Texture.Sample(Sampler, pin.TexCoord);

    // Gooch diffuse term: blend between cool and warm based
    float3 color = GoochShading(NdotL, pin.Diffuse.rgb * texColor.rgb);

    // Rim lighting
    float outline = RimLighting(NdotV);
    color += lerp(color, RimColor, outline);

    // Specular highlight
    float specular = HardEdgeSpecular(RdotV);
    color += SpecularColor * specular;

    return float4(color, pin.Diffuse.a * texColor.a);
}

[RootSignature(SkinTextureSamplerRS)]
float4 PSGoochShadingSkinTx(PSInputPixelLightingTx pin) : SV_Target0
{
    return PSGoochShadingTx(pin);
}


//--- MatCap shading ---

// Pixel shader: MatCap shading.
[RootSignature(MainRS)]
float4 PSMatCapShading(PSInputPixelLighting pin) : SV_Target0
{
    float3 normal = normalize(pin.NormalWS);
    float3 viewDir = normalize(EyePosition - pin.PositionWS.xyz);
    float3 reflectDir = reflect(viewDir, normal);

    float2 uv = reflectDir.xy * 0.5 + 0.5;

    // Sample matcap texture
    float4 matcap = Texture.Sample(Sampler, uv);

    float3 color = pin.Diffuse.rgb * matcap.rgb;

    return float4(color, pin.Diffuse.a);
}


// Pixel shader: MatCap shading + texture.
[RootSignature(DualTextureOneSamplerRS)]
float4 PSMatCapShadingTx(PSInputPixelLightingTx pin) : SV_Target0
{
    float3 normal = normalize(pin.NormalWS);
    float3 viewDir = normalize(EyePosition - pin.PositionWS.xyz);
    float3 reflectDir = reflect(viewDir, normal);

    float2 uv = reflectDir.xy * 0.5 + 0.5;

    // Sample base texture
    float4 texColor = Texture.Sample(Sampler, pin.TexCoord);

    // Sample matcap texture
    float4 matcap = Texture2.Sample(Sampler, uv);

    float3 color = pin.Diffuse.rgb * texColor.rgb * matcap.rgb;

    return float4(color, pin.Diffuse.a * texColor.a);
}

[RootSignature(SkinDualTextureOneSamplerRS)]
float4 PSMatCapShadingSkinTx(PSInputPixelLightingTx pin) : SV_Target0
{
    return PSMatCapShadingTx(pin);
}
