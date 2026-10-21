// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// https://go.microsoft.com/fwlink/?LinkID=615561

#include "Structures.fxh"
#include "RootSig.fxh"

SamplerState Sampler       : register(s0);
Texture2D<float4> SrcMip   : register(t0);
RWTexture2D<float4> OutMip : register(u0);

cbuffer MipConstants : register(b0)
{
    float2 InvOutTexelSize; // texel size for OutMip (NOT SrcMip)
    uint SrcMipIndex;
}

float4 Mip(uint2 coord)
{
    float2 uv = (coord.xy + 0.5) * InvOutTexelSize;
    return SrcMip.SampleLevel(Sampler, uv, SrcMipIndex);
}

[RootSignature(GenerateMipsRS)]
[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    OutMip[DTid.xy] = Mip(DTid.xy);
}

float LinearToSRGB(float color)
{
    return (color <= 0.0031308f) ? 12.92f * color : 1.055f * pow(abs(color), 1.0f / 2.4f) - 0.055f;
}

[RootSignature(GenerateMipsRS)]
[numthreads(8, 8, 1)]
void sRGB(uint3 DTid : SV_DispatchThreadID)
{
    float4 color = Mip(DTid.xy);
    // Encode the filtered RGB to sRGB before writing to the UNORM UAV.
    OutMip[DTid.xy] = float4(LinearToSRGB(color.r), LinearToSRGB(color.g), LinearToSRGB(color.b), color.a);
}
