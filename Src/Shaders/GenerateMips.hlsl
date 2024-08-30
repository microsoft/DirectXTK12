// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=615561

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

float3 ApplySRGBCurve(float3 x)
{
#if __HLSL_VERSION >= 2021
    // This is exactly the sRGB curve
    //return select(x < 0.0031308, 12.92 * x, 1.055 * pow(abs(x), 1.0 / 2.4) - 0.055);

    // This is cheaper but nearly equivalent to the exact sRGB curve
    return select(x < 0.0031308, 12.92 * x, 1.13005 * sqrt(abs(x - 0.00228)) - 0.13448 * x + 0.005719);
#else
    float3 result;
    if (x.r < 0.0031308)
    {
        result.r = 12.92 * x.r;
    }
    else
    {
        result.r = 1.055 * pow(abs(x.r), 1.0 / 2.4) - 0.055;
    }
    if (x.g < 0.0031308)
    {
        result.g = 12.92 * x.g;
    }
    else
    {
        result.g = 1.055 * pow(abs(x.g), 1.0 / 2.4) - 0.055;
    }
    if (x.b < 0.0031308)
    {
        result.b = 12.92 * x.b;
    }
    else
    {
        result.b = 1.055 * pow(abs(x.b), 1.0 / 2.4) - 0.055;
    }
    return result;
#endif
}

[RootSignature(GenerateMipsRS)]
[numthreads(8, 8, 1)]
void sRGB(uint3 DTid : SV_DispatchThreadID)
{
    float4 Linear = Mip(DTid.xy);
    OutMip[DTid.xy] =  float4(ApplySRGBCurve(Linear.rgb), Linear.a);
}
