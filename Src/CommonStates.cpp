//--------------------------------------------------------------------------------------
// File: CommonStates.cpp
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
#include "CommonStates.h"
#include "DirectXHelpers.h"

using namespace DirectX;

// --------------------------------------------------------------------------
// Blend States
// --------------------------------------------------------------------------

const D3D12_BLEND_DESC CommonStates::Opaque =
{
    FALSE, // AlphaToCoverageEnable
    FALSE, // IndependentBlendEnable
    {
        FALSE, // BlendEnable
        FALSE, // LogicOpEnable
        D3D12_BLEND_ONE, // SrcBlend
        D3D12_BLEND_ZERO, // DestBlend
        D3D12_BLEND_OP_ADD, // BlendOp
        D3D12_BLEND_ONE, // SrcBlendAlpha
        D3D12_BLEND_ZERO, // DestBlendAlpha
        D3D12_BLEND_OP_ADD, // BlendOpAlpha
        D3D12_LOGIC_OP_CLEAR, // LogicOp
        D3D12_COLOR_WRITE_ENABLE_ALL // RenderTargetWriteMask
    }
};

const D3D12_BLEND_DESC CommonStates::AlphaBlend =
{
    FALSE, // AlphaToCoverageEnable
    FALSE, // IndependentBlendEnable
    {
        TRUE, // BlendEnable
        FALSE, // LogicOpEnable
        D3D12_BLEND_ONE, // SrcBlend
        D3D12_BLEND_INV_SRC_ALPHA, // DestBlend
        D3D12_BLEND_OP_ADD, // BlendOp
        D3D12_BLEND_ONE, // SrcBlendAlpha
        D3D12_BLEND_INV_SRC_ALPHA, // DestBlendAlpha
        D3D12_BLEND_OP_ADD, // BlendOpAlpha
        D3D12_LOGIC_OP_CLEAR, // LogicOp
        D3D12_COLOR_WRITE_ENABLE_ALL // RenderTargetWriteMask
    }
};

const D3D12_BLEND_DESC CommonStates::Additive =
{
    FALSE, // AlphaToCoverageEnable
    FALSE, // IndependentBlendEnable
    {
        TRUE, // BlendEnable
        FALSE, // LogicOpEnable
        D3D12_BLEND_ONE, // SrcBlend
        D3D12_BLEND_ONE, // DestBlend
        D3D12_BLEND_OP_ADD, // BlendOp
        D3D12_BLEND_ONE, // SrcBlendAlpha
        D3D12_BLEND_ONE, // DestBlendAlpha
        D3D12_BLEND_OP_ADD, // BlendOpAlpha
        D3D12_LOGIC_OP_CLEAR, // LogicOp
        D3D12_COLOR_WRITE_ENABLE_ALL // RenderTargetWriteMask
    }
};

const D3D12_BLEND_DESC CommonStates::NonPremultiplied =
{
    FALSE, // AlphaToCoverageEnable
    FALSE, // IndependentBlendEnable
    {
        TRUE, // BlendEnable
        FALSE, // LogicOpEnable
        D3D12_BLEND_SRC_ALPHA, // SrcBlend
        D3D12_BLEND_INV_SRC_ALPHA, // DestBlend
        D3D12_BLEND_OP_ADD, // BlendOp
        D3D12_BLEND_SRC_ALPHA, // SrcBlendAlpha
        D3D12_BLEND_INV_SRC_ALPHA, // DestBlendAlpha
        D3D12_BLEND_OP_ADD, // BlendOpAlpha
        D3D12_LOGIC_OP_CLEAR, // LogicOp
        D3D12_COLOR_WRITE_ENABLE_ALL // RenderTargetWriteMask
    }
};


// --------------------------------------------------------------------------
// Depth-Stencil States
// --------------------------------------------------------------------------

const D3D12_DEPTH_STENCIL_DESC CommonStates::DepthNone =
{
    FALSE, // DepthEnable
    D3D12_DEPTH_WRITE_MASK_ZERO, // DepthWriteMask
    D3D12_COMPARISON_FUNC_ALWAYS, // DepthFunc
    FALSE, // StencilEnable
    D3D12_DEFAULT_STENCIL_READ_MASK, // StencilReadMask
    D3D12_DEFAULT_STENCIL_READ_MASK, // StencilWriteMask
    {
        D3D12_STENCIL_OP_KEEP, // StencilFailOp
        D3D12_STENCIL_OP_KEEP, // StencilDepthFailOp
        D3D12_STENCIL_OP_KEEP, // StencilPassOp
        D3D12_COMPARISON_FUNC_ALWAYS // StencilFunc
    }, // FrontFace,
    {
        D3D12_STENCIL_OP_KEEP, // StencilFailOp
        D3D12_STENCIL_OP_KEEP, // StencilDepthFailOp
        D3D12_STENCIL_OP_KEEP, // StencilPassOp
        D3D12_COMPARISON_FUNC_ALWAYS // StencilFunc
    } // BackFace
};

const D3D12_DEPTH_STENCIL_DESC CommonStates::DepthDefault =
{
    TRUE, // DepthEnable
    D3D12_DEPTH_WRITE_MASK_ALL, // DepthWriteMask
    D3D12_COMPARISON_FUNC_LESS_EQUAL, // DepthFunc
    FALSE, // StencilEnable
    D3D12_DEFAULT_STENCIL_READ_MASK, // StencilReadMask
    D3D12_DEFAULT_STENCIL_READ_MASK, // StencilWriteMask
    {
        D3D12_STENCIL_OP_KEEP, // StencilFailOp
        D3D12_STENCIL_OP_KEEP, // StencilDepthFailOp
        D3D12_STENCIL_OP_KEEP, // StencilPassOp
        D3D12_COMPARISON_FUNC_ALWAYS // StencilFunc
    }, // FrontFace,
    {
        D3D12_STENCIL_OP_KEEP, // StencilFailOp
        D3D12_STENCIL_OP_KEEP, // StencilDepthFailOp
        D3D12_STENCIL_OP_KEEP, // StencilPassOp
        D3D12_COMPARISON_FUNC_ALWAYS // StencilFunc
    } // BackFace
};

const D3D12_DEPTH_STENCIL_DESC CommonStates::DepthRead =
{
    TRUE, // DepthEnable
    D3D12_DEPTH_WRITE_MASK_ZERO, // DepthWriteMask
    D3D12_COMPARISON_FUNC_LESS_EQUAL, // DepthFunc
    FALSE, // StencilEnable
    D3D12_DEFAULT_STENCIL_READ_MASK, // StencilReadMask
    D3D12_DEFAULT_STENCIL_READ_MASK, // StencilWriteMask
    {
        D3D12_STENCIL_OP_KEEP, // StencilFailOp
        D3D12_STENCIL_OP_KEEP, // StencilDepthFailOp
        D3D12_STENCIL_OP_KEEP, // StencilPassOp
        D3D12_COMPARISON_FUNC_ALWAYS // StencilFunc
    }, // FrontFace,
    {
        D3D12_STENCIL_OP_KEEP, // StencilFailOp
        D3D12_STENCIL_OP_KEEP, // StencilDepthFailOp
        D3D12_STENCIL_OP_KEEP, // StencilPassOp
        D3D12_COMPARISON_FUNC_ALWAYS // StencilFunc
    } // BackFace
};


// --------------------------------------------------------------------------
// Rasterizer States
// --------------------------------------------------------------------------

const D3D12_RASTERIZER_DESC CommonStates::CullNone =
{
    D3D12_FILL_MODE_SOLID, // FillMode
    D3D12_CULL_MODE_NONE, // CullMode
    FALSE, // FrontCounterClockwise
    0, // DepthBias
    0, // DepthBiasClamp
    0, // SlopeScaledDepthBias
    TRUE, // DepthClipEnable
    TRUE, // MultisampleEnable
    FALSE, // AntialiasedLineEnable
    0, // ForcedSampleCount
    D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF // ConservativeRaster
};

const D3D12_RASTERIZER_DESC CommonStates::CullClockwise =
{
    D3D12_FILL_MODE_SOLID, // FillMode
    D3D12_CULL_MODE_FRONT, // CullMode
    FALSE, // FrontCounterClockwise
    0, // DepthBias
    0, // DepthBiasClamp
    0, // SlopeScaledDepthBias
    TRUE, // DepthClipEnable
    TRUE, // MultisampleEnable
    FALSE, // AntialiasedLineEnable
    0, // ForcedSampleCount
    D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF // ConservativeRaster
};

const D3D12_RASTERIZER_DESC CommonStates::CullCounterClockwise =
{
    D3D12_FILL_MODE_SOLID, // FillMode
    D3D12_CULL_MODE_BACK, // CullMode
    FALSE, // FrontCounterClockwise
    0, // DepthBias
    0, // DepthBiasClamp
    0, // SlopeScaledDepthBias
    TRUE, // DepthClipEnable
    TRUE, // MultisampleEnable
    FALSE, // AntialiasedLineEnable
    0, // ForcedSampleCount
    D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF // ConservativeRaster
};

const D3D12_RASTERIZER_DESC CommonStates::Wireframe =
{
    D3D12_FILL_MODE_WIREFRAME, // FillMode
    D3D12_CULL_MODE_BACK, // CullMode
    FALSE, // FrontCounterClockwise
    0, // DepthBias
    0, // DepthBiasClamp
    0, // SlopeScaledDepthBias
    TRUE, // DepthClipEnable
    TRUE, // MultisampleEnable
    FALSE, // AntialiasedLineEnable
    0, // ForcedSampleCount
    D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF // ConservativeRaster
};


// --------------------------------------------------------------------------
// Sampler States
// --------------------------------------------------------------------------

const D3D12_SAMPLER_DESC CommonStates::PointWrap = 
{
    D3D12_FILTER_MIN_MAG_MIP_POINT, // Filter
    D3D12_TEXTURE_ADDRESS_MODE_WRAP, // AddressU
    D3D12_TEXTURE_ADDRESS_MODE_WRAP, // AddressV
    D3D12_TEXTURE_ADDRESS_MODE_WRAP, // AddressW
    0, // MipLODBias
    D3D12_MAX_MAXANISOTROPY, // MaxAnisotropy
    D3D12_COMPARISON_FUNC_NEVER, // ComparisonFunc
    { 0, 0, 0, 0 }, // BorderColor
    0, // MinLOD
    FLT_MAX // MaxLOD
};

const D3D12_SAMPLER_DESC CommonStates::PointClamp =
{
    D3D12_FILTER_MIN_MAG_MIP_POINT, // Filter
    D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // AddressU
    D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // AddressV
    D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // AddressW
    0, // MipLODBias
    D3D12_MAX_MAXANISOTROPY, // MaxAnisotropy
    D3D12_COMPARISON_FUNC_NEVER, // ComparisonFunc
    { 0, 0, 0, 0 }, // BorderColor
    0, // MinLOD
    FLT_MAX // MaxLOD
};

const D3D12_SAMPLER_DESC CommonStates::LinearWrap = 
{
    D3D12_FILTER_MIN_MAG_MIP_LINEAR, // Filter
    D3D12_TEXTURE_ADDRESS_MODE_WRAP, // AddressU
    D3D12_TEXTURE_ADDRESS_MODE_WRAP, // AddressV
    D3D12_TEXTURE_ADDRESS_MODE_WRAP, // AddressW
    0, // MipLODBias
    D3D12_MAX_MAXANISOTROPY, // MaxAnisotropy
    D3D12_COMPARISON_FUNC_NEVER, // ComparisonFunc
    { 0, 0, 0, 0 }, // BorderColor
    0, // MinLOD
    FLT_MAX // MaxLOD
};

const D3D12_SAMPLER_DESC CommonStates::LinearClamp =
{
    D3D12_FILTER_MIN_MAG_MIP_LINEAR, // Filter
    D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // AddressU
    D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // AddressV
    D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // AddressW
    0, // MipLODBias
    D3D12_MAX_MAXANISOTROPY, // MaxAnisotropy
    D3D12_COMPARISON_FUNC_NEVER, // ComparisonFunc
    { 0, 0, 0, 0 }, // BorderColor
    0, // MinLOD
    FLT_MAX // MaxLOD
};

const D3D12_SAMPLER_DESC CommonStates::AnisotropicWrap = 
{
    D3D12_FILTER_MIN_MAG_MIP_LINEAR, // Filter
    D3D12_TEXTURE_ADDRESS_MODE_WRAP, // AddressU
    D3D12_TEXTURE_ADDRESS_MODE_WRAP, // AddressV
    D3D12_TEXTURE_ADDRESS_MODE_WRAP, // AddressW
    0, // MipLODBias
    D3D12_MAX_MAXANISOTROPY, // MaxAnisotropy
    D3D12_COMPARISON_FUNC_NEVER, // ComparisonFunc
    { 0, 0, 0, 0 }, // BorderColor
    0, // MinLOD
    FLT_MAX // MaxLOD
};

const D3D12_SAMPLER_DESC CommonStates::AnisotropicClamp =
{
    D3D12_FILTER_ANISOTROPIC, // Filter
    D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // AddressU
    D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // AddressV
    D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // AddressW
    0, // MipLODBias
    D3D12_MAX_MAXANISOTROPY, // MaxAnisotropy
    D3D12_COMPARISON_FUNC_NEVER, // ComparisonFunc
    { 0, 0, 0, 0 }, // BorderColor
    0, // MinLOD
    FLT_MAX // MaxLOD
};

