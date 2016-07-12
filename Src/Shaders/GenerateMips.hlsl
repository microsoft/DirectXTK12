// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkID=615561
// http://create.msdn.com/en-US/education/catalog/sample/stock_effects

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

//-----------------------------------------------------------------------------
// DE-GAMMA AND RE-GAMMA IN PLACE
// On hardware that supports Tier 2 UAV-formats this is trivial because we 
// could just use a RWTexture2D<float4>, but unfortunately Tier 1 doesn't 
// support typed UAV loads. As such, we have to pack and unpack the color data 
// manually. Luckily, DXTK only supports mip-map generation for 8888 SRGB 
// formats, so we can manually unpack the channels from a uint. Additionally,
// it doesn't matter whether we're using BGRA or RGBA.
//-----------------------------------------------------------------------------

RWTexture2D<uint> GammaUAV : register(u0);

float3 SRGBToLinear(float3 c)
{
    return c < 0.04045 ? c / 12.92 : pow(saturate(c) + 0.055, 2.4);
}

float4 SRGBToLinear(float4 c)
{
    return float4(SRGBToLinear(c.rgb), c.a);
}

float3 LinearToSRGB(float3 c)
{
    return c < 0.003131 ? 12.92 * c : 1.055 * pow(saturate(c), 1.0 / 2.4) - 0.055;
}

float4 LinearToSRGB(float4 c)
{
    return float4(LinearToSRGB(c.rgb), c.a);
}

float4 unpack_8888_to_float4(uint packedInput)  
{  
    precise float4 unpackedOutput;  
    unpackedOutput.x = (float)  (packedInput      & 0xff)  / 255;  
    unpackedOutput.y = (float)(((packedInput>> 8) & 0xff)) / 255;  
    unpackedOutput.z = (float)(((packedInput>>16) & 0xff)) / 255;  
    unpackedOutput.w = (float)  (packedInput>>24)          / 255;  
    return unpackedOutput;  
}  

uint pack_float4_to_8888(precise float4 unpackedInput)  
{  
    uint packedOutput;  
    unpackedInput = saturate(unpackedInput);
    packedOutput = ( ((uint)floor(unpackedInput.x * 255 + 0.5f))     |  
                     ((uint)floor(unpackedInput.y * 255 + 0.5f)<< 8) |  
                     ((uint)floor(unpackedInput.z * 255 + 0.5f)<<16) |  
                     ((uint)floor(unpackedInput.w * 255 + 0.5f)<<24) );  
    return packedOutput;  
}  

[RootSignature(GenerateMipsRS)]
[numthreads(8, 8, 1)]
void DegammaInPlace(uint3 DTid : SV_DispatchThreadID)
{
    float4 v = unpack_8888_to_float4(GammaUAV[DTid.xy]);
    v = SRGBToLinear(v);
    GammaUAV[DTid.xy] = pack_float4_to_8888(v);
}

[RootSignature(GenerateMipsRS)]
[numthreads(8, 8, 1)]
void RegammaInPlace(uint3 DTid : SV_DispatchThreadID)
{
    float4 v = unpack_8888_to_float4(GammaUAV[DTid.xy]);
    v = LinearToSRGB(v);
    GammaUAV[DTid.xy] = pack_float4_to_8888(v);
}
