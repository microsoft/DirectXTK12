//--------------------------------------------------------------------------------------
// File: PostProcess.h
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#pragma once

#ifdef _GAMING_XBOX_SCARLETT
#include <d3d12_xs.h>
#elif (defined(_XBOX_ONE) && defined(_TITLE)) || defined(_GAMING_XBOX)
#include <d3d12_x.h>
#elif defined(USING_DIRECTX_HEADERS)
#include <directx/d3d12.h>
#include <dxguids/dxguids.h>
#else
#include <d3d12.h>
#endif

#include <memory>

#include <DirectXMath.h>

#include "RenderTargetState.h"


namespace DirectX
{
    //----------------------------------------------------------------------------------
    // Abstract interface representing a post-process pass
    class IPostProcess
    {
    public:
        virtual ~IPostProcess() = default;

        IPostProcess(const IPostProcess&) = delete;
        IPostProcess& operator=(const IPostProcess&) = delete;

        virtual void __cdecl Process(_In_ ID3D12GraphicsCommandList* commandList) = 0;

    protected:
        IPostProcess() = default;
        IPostProcess(IPostProcess&&) = default;
        IPostProcess& operator=(IPostProcess&&) = default;
    };


    //----------------------------------------------------------------------------------
    // Basic post-process
    class BasicPostProcess : public IPostProcess
    {
    public:
        enum Effect : unsigned int
        {
            Copy,
            Monochrome,
            Sepia,
            DownScale_2x2,
            DownScale_4x4,
            GaussianBlur_5x5,
            BloomExtract,
            BloomBlur,
            Effect_Max
        };

        BasicPostProcess(_In_ ID3D12Device* device, const RenderTargetState& rtState, Effect fx);

        BasicPostProcess(BasicPostProcess&&) noexcept;
        BasicPostProcess& operator= (BasicPostProcess&&) noexcept;

        BasicPostProcess(BasicPostProcess const&) = delete;
        BasicPostProcess& operator= (BasicPostProcess const&) = delete;

        ~BasicPostProcess() override;

        // IPostProcess methods.
        void __cdecl Process(_In_ ID3D12GraphicsCommandList* commandList) override;

        // Properties
        void __cdecl SetSourceTexture(D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor, _In_opt_ ID3D12Resource* resource);

        // Sets multiplier for GaussianBlur_5x5
        void __cdecl SetGaussianParameter(float multiplier);

        // Sets parameters for BloomExtract
        void __cdecl SetBloomExtractParameter(float threshold);

        // Sets parameters for BloomBlur
        void __cdecl SetBloomBlurParameters(bool horizontal, float size, float brightness);

    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;
    };


    //----------------------------------------------------------------------------------
    // Dual-texure post-process
    class DualPostProcess : public IPostProcess
    {
    public:
        enum Effect : unsigned int
        {
            Merge,
            BloomCombine,
            Effect_Max
        };

        DualPostProcess(_In_ ID3D12Device* device, const RenderTargetState& rtState, Effect fx);

        DualPostProcess(DualPostProcess&&) noexcept;
        DualPostProcess& operator= (DualPostProcess&&) noexcept;

        DualPostProcess(DualPostProcess const&) = delete;
        DualPostProcess& operator= (DualPostProcess const&) = delete;

        ~DualPostProcess() override;

        // IPostProcess methods.
        void __cdecl Process(_In_ ID3D12GraphicsCommandList* commandList) override;

        // Properties
        void __cdecl SetSourceTexture(D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor);
        void __cdecl SetSourceTexture2(D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor);

        // Sets parameters for Merge
        void __cdecl SetMergeParameters(float weight1, float weight2);

        // Sets parameters for BloomCombine
        void __cdecl SetBloomCombineParameters(float bloom, float base, float bloomSaturation, float baseSaturation);

    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;
    };


    //----------------------------------------------------------------------------------
    // Tone-map post-process
    class ToneMapPostProcess : public IPostProcess
    {
    public:
        // Tone-mapping operator
        enum Operator : unsigned int
        {
            None,               // Pass-through
            Saturate,           // Clamp [0,1]
            Reinhard,           // x/(1+x)
            ACESFilmic,
            Operator_Max
        };

        // Electro-Optical Transfer Function (EOTF)
        enum TransferFunction : unsigned int
        {
            Linear,             // Pass-through
            SRGB,               // sRGB (Rec.709 and approximate sRGB display curve)
            ST2084,             // HDR10 (Rec.2020 color primaries and ST.2084 display curve)
            TransferFunction_Max
        };

        // Color Rotation Transform for HDR10
        enum ColorPrimaryRotation : unsigned int
        {
            HDTV_to_UHDTV,       // Rec.709 to Rec.2020
            DCI_P3_D65_to_UHDTV, // DCI-P3-D65 (a.k.a Display P3 or P3D65) to Rec.2020
            HDTV_to_DCI_P3_D65,  // Rec.709 to DCI-P3-D65 (a.k.a Display P3 or P3D65)
        };

        ToneMapPostProcess(_In_ ID3D12Device* device, const RenderTargetState& rtState,
            Operator op, TransferFunction func
        #if (defined(_XBOX_ONE) && defined(_TITLE)) || defined(_GAMING_XBOX)
            , bool mrt = false
        #endif
        );

        ToneMapPostProcess(ToneMapPostProcess&&) noexcept;
        ToneMapPostProcess& operator= (ToneMapPostProcess&&) noexcept;

        ToneMapPostProcess(ToneMapPostProcess const&) = delete;
        ToneMapPostProcess& operator= (ToneMapPostProcess const&) = delete;

        ~ToneMapPostProcess() override;

        // IPostProcess methods.
        void __cdecl Process(_In_ ID3D12GraphicsCommandList* commandList) override;

        // Properties
        void __cdecl SetHDRSourceTexture(D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor);

        // Sets the Color Rotation Transform for HDR10 signal output
        void __cdecl SetColorRotation(ColorPrimaryRotation value);
        void __cdecl SetColorRotation(CXMMATRIX value);

        // Sets exposure value for LDR tonemap operators
        void __cdecl SetExposure(float exposureValue);

        // Sets ST.2084 parameter for how bright white should be in nits
        void __cdecl SetST2084Parameter(float paperWhiteNits);

    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;
    };
}
