// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkID=615561

// Root signatures must match definition in each effect, or shaders will be recompiled on Xbox when PSO loads
#define MainRS \
"RootFlags ( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |" \
"            DENY_DOMAIN_SHADER_ROOT_ACCESS |" \
"            DENY_GEOMETRY_SHADER_ROOT_ACCESS |" \
"            DENY_HULL_SHADER_ROOT_ACCESS )," \
"DescriptorTable ( SRV(t0) ),"\
"CBV(b0)," \
"StaticSampler(s0)"

#define DualTextureRS \
"RootFlags ( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |" \
"            DENY_DOMAIN_SHADER_ROOT_ACCESS |" \
"            DENY_GEOMETRY_SHADER_ROOT_ACCESS |" \
"            DENY_HULL_SHADER_ROOT_ACCESS )," \
"DescriptorTable ( SRV(t0) )," \
"DescriptorTable ( SRV(t1) )," \
"CBV(b0)," \
"StaticSampler(s0)," \
"StaticSampler(s1)"

#define GenerateMipsRS \
"RootFlags ( DENY_VERTEX_SHADER_ROOT_ACCESS   |" \
"            DENY_DOMAIN_SHADER_ROOT_ACCESS   |" \
"            DENY_GEOMETRY_SHADER_ROOT_ACCESS |" \
"            DENY_HULL_SHADER_ROOT_ACCESS     |" \
"            DENY_PIXEL_SHADER_ROOT_ACCESS  )," \
"RootConstants(num32BitConstants=3, b0)," \
"DescriptorTable ( SRV(t0) )," \
"DescriptorTable ( UAV(u0) )," \
"StaticSampler(s0,"\
"           filter =   FILTER_MIN_MAG_LINEAR_MIP_POINT,"\
"           addressU = TEXTURE_ADDRESS_CLAMP,"\
"           addressV = TEXTURE_ADDRESS_CLAMP,"\
"           addressW = TEXTURE_ADDRESS_CLAMP )"
