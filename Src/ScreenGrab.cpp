//--------------------------------------------------------------------------------------
// File: ScreenGrab.cpp
//
// Function for capturing a 2D texture and saving it to a file (aka a 'screenshot'
// when used on a Direct3D Render Target).
//
// Note these functions are useful as a light-weight runtime screen grabber. For
// full-featured texture capture, DDS writer, and texture processing pipeline,
// see the 'Texconv' sample and the 'DirectXTex' library.
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

// Does not capture 1D textures or 3D textures (volume maps)

// Does not capture mipmap chains, only the top-most texture level is saved

// For 2D array textures and cubemaps, it captures only the first image in the array

#include "pch.h"

#include "ScreenGrab.h"
#include "DirectXHelpers.h"

#include "PlatformHelpers.h"
#include "DDS.h"
#include "LoaderHelpers.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::LoaderHelpers;

namespace
{
    //--------------------------------------------------------------------------------------
    HRESULT CaptureTexture(_In_ ID3D12Device* device,
        _In_ ID3D12CommandQueue* pCommandQ,
        _In_ ID3D12Resource* pSource,
        UINT64 srcPitch,
        const D3D12_RESOURCE_DESC& desc,
        ComPtr<ID3D12Resource>& pStaging,
        D3D12_RESOURCE_STATES beforeState,
        D3D12_RESOURCE_STATES afterState) noexcept
    {
        if (!pCommandQ || !pSource)
            return E_INVALIDARG;

        if (desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D)
        {
            DebugTrace("ERROR: ScreenGrab does not support 1D or volume textures. Consider using DirectXTex instead.\n");
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        }

        if (desc.DepthOrArraySize > 1 || desc.MipLevels > 1)
        {
            DebugTrace("WARNING: ScreenGrab does not support 2D arrays, cubemaps, or mipmaps; only the first surface is written. Consider using DirectXTex instead.\n");
        }

        if (srcPitch > UINT32_MAX)
            return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);

        const UINT numberOfPlanes = D3D12GetFormatPlaneCount(device, desc.Format);
        if (numberOfPlanes != 1)
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);

        D3D12_HEAP_PROPERTIES sourceHeapProperties;
        HRESULT hr = pSource->GetHeapProperties(&sourceHeapProperties, nullptr);
        if (SUCCEEDED(hr) && sourceHeapProperties.Type == D3D12_HEAP_TYPE_READBACK)
        {
            // Handle case where the source is already a staging texture we can use directly
            pStaging = pSource;
            return S_OK;
        }

        // Create a command allocator
        ComPtr<ID3D12CommandAllocator> commandAlloc;
        hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_GRAPHICS_PPV_ARGS(commandAlloc.GetAddressOf()));
        if (FAILED(hr))
            return hr;

        SetDebugObjectName(commandAlloc.Get(), L"ScreenGrab");

        // Spin up a new command list
        ComPtr<ID3D12GraphicsCommandList> commandList;
        hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAlloc.Get(), nullptr, IID_GRAPHICS_PPV_ARGS(commandList.GetAddressOf()));
        if (FAILED(hr))
            return hr;

        SetDebugObjectName(commandList.Get(), L"ScreenGrab");

        // Create a fence
        ComPtr<ID3D12Fence> fence;
        hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_GRAPHICS_PPV_ARGS(fence.GetAddressOf()));
        if (FAILED(hr))
            return hr;

        SetDebugObjectName(fence.Get(), L"ScreenGrab");

        assert((srcPitch & 0xFF) == 0);

        const CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
        const CD3DX12_HEAP_PROPERTIES readBackHeapProperties(D3D12_HEAP_TYPE_READBACK);

        // Readback resources must be buffers
        D3D12_RESOURCE_DESC bufferDesc = {};
        bufferDesc.DepthOrArraySize = 1;
        bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        bufferDesc.Height = 1;
        bufferDesc.Width = srcPitch * desc.Height;
        bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        bufferDesc.MipLevels = 1;
        bufferDesc.SampleDesc.Count = 1;

        ComPtr<ID3D12Resource> copySource(pSource);
        if (desc.SampleDesc.Count > 1)
        {
            // MSAA content must be resolved before being copied to a staging texture
            auto descCopy = desc;
            descCopy.SampleDesc.Count = 1;
            descCopy.SampleDesc.Quality = 0;
            descCopy.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

            ComPtr<ID3D12Resource> pTemp;
            hr = device->CreateCommittedResource(
                &defaultHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &descCopy,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(pTemp.GetAddressOf()));
            if (FAILED(hr))
                return hr;

            assert(pTemp);

            SetDebugObjectName(pTemp.Get(), L"ScreenGrab temporary");

            const DXGI_FORMAT fmt = EnsureNotTypeless(desc.Format);

            D3D12_FEATURE_DATA_FORMAT_SUPPORT formatInfo = { fmt, D3D12_FORMAT_SUPPORT1_NONE, D3D12_FORMAT_SUPPORT2_NONE };
            hr = device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &formatInfo, sizeof(formatInfo));
            if (FAILED(hr))
                return hr;

            if (!(formatInfo.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURE2D))
                return E_FAIL;

            for (UINT item = 0; item < desc.DepthOrArraySize; ++item)
            {
                for (UINT level = 0; level < desc.MipLevels; ++level)
                {
                    const UINT index = D3D12CalcSubresource(level, item, 0, desc.MipLevels, desc.DepthOrArraySize);
                    commandList->ResolveSubresource(pTemp.Get(), index, pSource, index, fmt);
                }
            }

            copySource = pTemp;
        }

        // Create a staging texture
        hr = device->CreateCommittedResource(
            &readBackHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(pStaging.ReleaseAndGetAddressOf()));
        if (FAILED(hr))
            return hr;

        SetDebugObjectName(pStaging.Get(), L"ScreenGrab staging");

        assert(pStaging);

        // Transition the resource if necessary
        TransitionResource(commandList.Get(), pSource, beforeState, D3D12_RESOURCE_STATE_COPY_SOURCE);

        // Get the copy target location
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT bufferFootprint = {};
        bufferFootprint.Footprint.Width = static_cast<UINT>(desc.Width);
        bufferFootprint.Footprint.Height = desc.Height;
        bufferFootprint.Footprint.Depth = 1;
        bufferFootprint.Footprint.RowPitch = static_cast<UINT>(srcPitch);
        bufferFootprint.Footprint.Format = desc.Format;

        const CD3DX12_TEXTURE_COPY_LOCATION copyDest(pStaging.Get(), bufferFootprint);
        const CD3DX12_TEXTURE_COPY_LOCATION copySrc(copySource.Get(), 0);

        // Copy the texture
        commandList->CopyTextureRegion(&copyDest, 0, 0, 0, &copySrc, nullptr);

        // Transition the resource to the next state
        TransitionResource(commandList.Get(), pSource, D3D12_RESOURCE_STATE_COPY_SOURCE, afterState);

        hr = commandList->Close();
        if (FAILED(hr))
            return hr;

        // Execute the command list
        pCommandQ->ExecuteCommandLists(1, CommandListCast(commandList.GetAddressOf()));

        // Signal the fence
        hr = pCommandQ->Signal(fence.Get(), 1);
        if (FAILED(hr))
            return hr;

        // Block until the copy is complete
        while (fence->GetCompletedValue() < 1)
            SwitchToThread();

        return S_OK;
    }
} // anonymous namespace


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT DirectX::SaveDDSTextureToFile(
    ID3D12CommandQueue* pCommandQ,
    ID3D12Resource* pSource,
    const wchar_t* fileName,
    D3D12_RESOURCE_STATES beforeState,
    D3D12_RESOURCE_STATES afterState) noexcept
{
    if (!fileName)
        return E_INVALIDARG;

    ComPtr<ID3D12Device> device;
    pCommandQ->GetDevice(IID_GRAPHICS_PPV_ARGS(device.GetAddressOf()));

    // Get the size of the image
#if defined(_MSC_VER) || !defined(_WIN32)
    const auto desc = pSource->GetDesc();
#else
    D3D12_RESOURCE_DESC tmpDesc;
    const auto& desc = *pSource->GetDesc(&tmpDesc);
#endif

    if (desc.Width > UINT32_MAX)
        return E_INVALIDARG;

    UINT64 totalResourceSize = 0;
    UINT64 fpRowPitch = 0;
    UINT fpRowCount = 0;
    // Get the rowcount, pitch and size of the top mip
    device->GetCopyableFootprints(
        &desc,
        0,
        1,
        0,
        nullptr,
        &fpRowCount,
        &fpRowPitch,
        &totalResourceSize);

#if (defined(_XBOX_ONE) && defined(_TITLE)) || defined(_GAMING_XBOX)
    // Round up the srcPitch to multiples of 1024
    const UINT64 dstRowPitch = (fpRowPitch + static_cast<uint64_t>(D3D12XBOX_TEXTURE_DATA_PITCH_ALIGNMENT) - 1u) & ~(static_cast<uint64_t>(D3D12XBOX_TEXTURE_DATA_PITCH_ALIGNMENT) - 1u);
#else
    // Round up the srcPitch to multiples of 256
    const UINT64 dstRowPitch = (fpRowPitch + 255) & ~0xFFu;
#endif

    if (dstRowPitch > UINT32_MAX)
        return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);

    ComPtr<ID3D12Resource> pStaging;
    HRESULT hr = CaptureTexture(device.Get(), pCommandQ, pSource, dstRowPitch, desc, pStaging, beforeState, afterState);
    if (FAILED(hr))
        return hr;

    // Create file
    ScopedHandle hFile(safe_handle(CreateFile2(
        fileName,
        GENERIC_WRITE, 0, CREATE_ALWAYS,
        nullptr)));
    if (!hFile)
        return HRESULT_FROM_WIN32(GetLastError());

    auto_delete_file delonfail(hFile.get());

    // Setup header
    constexpr size_t MAX_HEADER_SIZE = sizeof(uint32_t) + sizeof(DDS_HEADER) + sizeof(DDS_HEADER_DXT10);
    uint8_t fileHeader[MAX_HEADER_SIZE] = {};

    *reinterpret_cast<uint32_t*>(&fileHeader[0]) = DDS_MAGIC;

    auto header = reinterpret_cast<DDS_HEADER*>(&fileHeader[0] + sizeof(uint32_t));
    size_t headerSize = sizeof(uint32_t) + sizeof(DDS_HEADER);
    header->size = sizeof(DDS_HEADER);
    header->flags = DDS_HEADER_FLAGS_TEXTURE | DDS_HEADER_FLAGS_MIPMAP;
    header->height = desc.Height;
    header->width = static_cast<uint32_t>(desc.Width);
    header->mipMapCount = 1;
    header->caps = DDS_SURFACE_FLAGS_TEXTURE;

    // Try to use a legacy .DDS pixel format for better tools support, otherwise fallback to 'DX10' header extension
    DDS_HEADER_DXT10* extHeader = nullptr;
    switch (desc.Format)
    {
    case DXGI_FORMAT_R8G8B8A8_UNORM:        memcpy(&header->ddspf, &DDSPF_A8B8G8R8, sizeof(DDS_PIXELFORMAT));    break;
    case DXGI_FORMAT_R16G16_UNORM:          memcpy(&header->ddspf, &DDSPF_G16R16, sizeof(DDS_PIXELFORMAT));      break;
    case DXGI_FORMAT_R8G8_UNORM:            memcpy(&header->ddspf, &DDSPF_A8L8, sizeof(DDS_PIXELFORMAT));        break;
    case DXGI_FORMAT_R16_UNORM:             memcpy(&header->ddspf, &DDSPF_L16, sizeof(DDS_PIXELFORMAT));         break;
    case DXGI_FORMAT_R8_UNORM:              memcpy(&header->ddspf, &DDSPF_L8, sizeof(DDS_PIXELFORMAT));          break;
    case DXGI_FORMAT_A8_UNORM:              memcpy(&header->ddspf, &DDSPF_A8, sizeof(DDS_PIXELFORMAT));          break;
    case DXGI_FORMAT_R8G8_B8G8_UNORM:       memcpy(&header->ddspf, &DDSPF_R8G8_B8G8, sizeof(DDS_PIXELFORMAT));   break;
    case DXGI_FORMAT_G8R8_G8B8_UNORM:       memcpy(&header->ddspf, &DDSPF_G8R8_G8B8, sizeof(DDS_PIXELFORMAT));   break;
    case DXGI_FORMAT_BC1_UNORM:             memcpy(&header->ddspf, &DDSPF_DXT1, sizeof(DDS_PIXELFORMAT));        break;
    case DXGI_FORMAT_BC2_UNORM:             memcpy(&header->ddspf, &DDSPF_DXT3, sizeof(DDS_PIXELFORMAT));        break;
    case DXGI_FORMAT_BC3_UNORM:             memcpy(&header->ddspf, &DDSPF_DXT5, sizeof(DDS_PIXELFORMAT));        break;
    case DXGI_FORMAT_BC4_UNORM:             memcpy(&header->ddspf, &DDSPF_BC4_UNORM, sizeof(DDS_PIXELFORMAT));   break;
    case DXGI_FORMAT_BC4_SNORM:             memcpy(&header->ddspf, &DDSPF_BC4_SNORM, sizeof(DDS_PIXELFORMAT));   break;
    case DXGI_FORMAT_BC5_UNORM:             memcpy(&header->ddspf, &DDSPF_BC5_UNORM, sizeof(DDS_PIXELFORMAT));   break;
    case DXGI_FORMAT_BC5_SNORM:             memcpy(&header->ddspf, &DDSPF_BC5_SNORM, sizeof(DDS_PIXELFORMAT));   break;
    case DXGI_FORMAT_B5G6R5_UNORM:          memcpy(&header->ddspf, &DDSPF_R5G6B5, sizeof(DDS_PIXELFORMAT));      break;
    case DXGI_FORMAT_B5G5R5A1_UNORM:        memcpy(&header->ddspf, &DDSPF_A1R5G5B5, sizeof(DDS_PIXELFORMAT));    break;
    case DXGI_FORMAT_R8G8_SNORM:            memcpy(&header->ddspf, &DDSPF_V8U8, sizeof(DDS_PIXELFORMAT));        break;
    case DXGI_FORMAT_R8G8B8A8_SNORM:        memcpy(&header->ddspf, &DDSPF_Q8W8V8U8, sizeof(DDS_PIXELFORMAT));    break;
    case DXGI_FORMAT_R16G16_SNORM:          memcpy(&header->ddspf, &DDSPF_V16U16, sizeof(DDS_PIXELFORMAT));      break;
    case DXGI_FORMAT_B8G8R8A8_UNORM:        memcpy(&header->ddspf, &DDSPF_A8R8G8B8, sizeof(DDS_PIXELFORMAT));    break;
    case DXGI_FORMAT_B8G8R8X8_UNORM:        memcpy(&header->ddspf, &DDSPF_X8R8G8B8, sizeof(DDS_PIXELFORMAT));    break;
    case DXGI_FORMAT_YUY2:                  memcpy(&header->ddspf, &DDSPF_YUY2, sizeof(DDS_PIXELFORMAT));        break;
    case DXGI_FORMAT_B4G4R4A4_UNORM:        memcpy(&header->ddspf, &DDSPF_A4R4G4B4, sizeof(DDS_PIXELFORMAT));    break;

        // Legacy D3DX formats using D3DFMT enum value as FourCC
    case DXGI_FORMAT_R32G32B32A32_FLOAT:    header->ddspf.size = sizeof(DDS_PIXELFORMAT); header->ddspf.flags = DDS_FOURCC; header->ddspf.fourCC = 116; break; // D3DFMT_A32B32G32R32F
    case DXGI_FORMAT_R16G16B16A16_FLOAT:    header->ddspf.size = sizeof(DDS_PIXELFORMAT); header->ddspf.flags = DDS_FOURCC; header->ddspf.fourCC = 113; break; // D3DFMT_A16B16G16R16F
    case DXGI_FORMAT_R16G16B16A16_UNORM:    header->ddspf.size = sizeof(DDS_PIXELFORMAT); header->ddspf.flags = DDS_FOURCC; header->ddspf.fourCC = 36;  break; // D3DFMT_A16B16G16R16
    case DXGI_FORMAT_R16G16B16A16_SNORM:    header->ddspf.size = sizeof(DDS_PIXELFORMAT); header->ddspf.flags = DDS_FOURCC; header->ddspf.fourCC = 110; break; // D3DFMT_Q16W16V16U16
    case DXGI_FORMAT_R32G32_FLOAT:          header->ddspf.size = sizeof(DDS_PIXELFORMAT); header->ddspf.flags = DDS_FOURCC; header->ddspf.fourCC = 115; break; // D3DFMT_G32R32F
    case DXGI_FORMAT_R16G16_FLOAT:          header->ddspf.size = sizeof(DDS_PIXELFORMAT); header->ddspf.flags = DDS_FOURCC; header->ddspf.fourCC = 112; break; // D3DFMT_G16R16F
    case DXGI_FORMAT_R32_FLOAT:             header->ddspf.size = sizeof(DDS_PIXELFORMAT); header->ddspf.flags = DDS_FOURCC; header->ddspf.fourCC = 114; break; // D3DFMT_R32F
    case DXGI_FORMAT_R16_FLOAT:             header->ddspf.size = sizeof(DDS_PIXELFORMAT); header->ddspf.flags = DDS_FOURCC; header->ddspf.fourCC = 111; break; // D3DFMT_R16F

    case DXGI_FORMAT_AI44:
    case DXGI_FORMAT_IA44:
    case DXGI_FORMAT_P8:
    case DXGI_FORMAT_A8P8:
        DebugTrace("ERROR: ScreenGrab does not support video textures. Consider using DirectXTex.\n");
        return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);

    default:
        memcpy(&header->ddspf, &DDSPF_DX10, sizeof(DDS_PIXELFORMAT));

        headerSize += sizeof(DDS_HEADER_DXT10);
        extHeader = reinterpret_cast<DDS_HEADER_DXT10*>(fileHeader + sizeof(uint32_t) + sizeof(DDS_HEADER));
        extHeader->dxgiFormat = desc.Format;
        extHeader->resourceDimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        extHeader->arraySize = 1;
        break;
    }

    size_t rowPitch, slicePitch, rowCount;
    hr = GetSurfaceInfo(static_cast<size_t>(desc.Width), desc.Height, desc.Format, &slicePitch, &rowPitch, &rowCount);
    if (FAILED(hr))
        return hr;

    if (rowPitch > UINT32_MAX || slicePitch > UINT32_MAX)
        return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);

    if (IsCompressed(desc.Format))
    {
        header->flags |= DDS_HEADER_FLAGS_LINEARSIZE;
        header->pitchOrLinearSize = static_cast<uint32_t>(slicePitch);
    }
    else
    {
        header->flags |= DDS_HEADER_FLAGS_PITCH;
        header->pitchOrLinearSize = static_cast<uint32_t>(rowPitch);
    }

    // Setup pixels
    std::unique_ptr<uint8_t[]> pixels(new (std::nothrow) uint8_t[slicePitch]);
    if (!pixels)
        return E_OUTOFMEMORY;

    assert(fpRowCount == rowCount);
    assert(fpRowPitch == rowPitch);

    const UINT64 imageSize = dstRowPitch * UINT64(rowCount);
    if (imageSize > UINT32_MAX)
        return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);

    void* pMappedMemory = nullptr;
    D3D12_RANGE readRange = { 0, static_cast<SIZE_T>(imageSize) };
    D3D12_RANGE writeRange = { 0, 0 };
    hr = pStaging->Map(0, &readRange, &pMappedMemory);
    if (FAILED(hr))
        return hr;

    auto sptr = static_cast<const uint8_t*>(pMappedMemory);
    if (!sptr)
    {
        pStaging->Unmap(0, &writeRange);
        return E_POINTER;
    }

    uint8_t* dptr = pixels.get();

    const size_t msize = std::min<size_t>(rowPitch, size_t(dstRowPitch));
    for (size_t h = 0; h < rowCount; ++h)
    {
        memcpy(dptr, sptr, msize);
        sptr += dstRowPitch;
        dptr += rowPitch;
    }

    pStaging->Unmap(0, &writeRange);

    // Write header & pixels
    DWORD bytesWritten;
    if (!WriteFile(hFile.get(), fileHeader, static_cast<DWORD>(headerSize), &bytesWritten, nullptr))
        return HRESULT_FROM_WIN32(GetLastError());

    if (bytesWritten != headerSize)
        return E_FAIL;

    if (!WriteFile(hFile.get(), pixels.get(), static_cast<DWORD>(slicePitch), &bytesWritten, nullptr))
        return HRESULT_FROM_WIN32(GetLastError());

    if (bytesWritten != slicePitch)
        return E_FAIL;

    delonfail.clear();

    return S_OK;
}

//--------------------------------------------------------------------------------------
namespace DirectX
{
    inline namespace DX12
    {
        namespace Internal
        {
            extern IWICImagingFactory2* GetWIC() noexcept;
        }
    }
}

_Use_decl_annotations_
HRESULT DirectX::SaveWICTextureToFile(
    ID3D12CommandQueue* pCommandQ,
    ID3D12Resource* pSource,
    REFGUID guidContainerFormat,
    const wchar_t* fileName,
    D3D12_RESOURCE_STATES beforeState,
    D3D12_RESOURCE_STATES afterState,
    const GUID* targetFormat,
    std::function<void(IPropertyBag2*)> setCustomProps,
    bool forceSRGB)
{
    using namespace DirectX::DX12::Internal;

    if (!fileName)
        return E_INVALIDARG;

    ComPtr<ID3D12Device> device;
    pCommandQ->GetDevice(IID_GRAPHICS_PPV_ARGS(device.GetAddressOf()));

    // Get the size of the image
#if defined(_MSC_VER) || !defined(_WIN32)
    const auto desc = pSource->GetDesc();
#else
    D3D12_RESOURCE_DESC tmpDesc;
    const auto& desc = *pSource->GetDesc(&tmpDesc);
#endif

    if (desc.Width > UINT32_MAX)
        return E_INVALIDARG;

    UINT64 totalResourceSize = 0;
    UINT64 fpRowPitch = 0;
    UINT fpRowCount = 0;
    // Get the rowcount, pitch and size of the top mip
    device->GetCopyableFootprints(
        &desc,
        0,
        1,
        0,
        nullptr,
        &fpRowCount,
        &fpRowPitch,
        &totalResourceSize);

#if (defined(_XBOX_ONE) && defined(_TITLE)) || defined(_GAMING_XBOX)
    // Round up the srcPitch to multiples of 1024
    UINT64 dstRowPitch = (fpRowPitch + static_cast<uint64_t>(D3D12XBOX_TEXTURE_DATA_PITCH_ALIGNMENT) - 1u) & ~(static_cast<uint64_t>(D3D12XBOX_TEXTURE_DATA_PITCH_ALIGNMENT) - 1u);
#else
    // Round up the srcPitch to multiples of 256
    const UINT64 dstRowPitch = (fpRowPitch + 255) & ~0xFFu;
#endif

    if (dstRowPitch > UINT32_MAX)
        return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);

    ComPtr<ID3D12Resource> pStaging;
    HRESULT hr = CaptureTexture(device.Get(), pCommandQ, pSource, dstRowPitch, desc, pStaging, beforeState, afterState);
    if (FAILED(hr))
        return hr;

    // Determine source format's WIC equivalent
    WICPixelFormatGUID pfGuid = {};
    bool sRGB = forceSRGB;
    switch (desc.Format)
    {
    case DXGI_FORMAT_R32G32B32A32_FLOAT:            pfGuid = GUID_WICPixelFormat128bppRGBAFloat; break;
    case DXGI_FORMAT_R16G16B16A16_FLOAT:            pfGuid = GUID_WICPixelFormat64bppRGBAHalf; break;
    case DXGI_FORMAT_R16G16B16A16_UNORM:            pfGuid = GUID_WICPixelFormat64bppRGBA; break;
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:    pfGuid = GUID_WICPixelFormat32bppRGBA1010102XR; break;
    case DXGI_FORMAT_R10G10B10A2_UNORM:             pfGuid = GUID_WICPixelFormat32bppRGBA1010102; break;
    case DXGI_FORMAT_B5G5R5A1_UNORM:                pfGuid = GUID_WICPixelFormat16bppBGRA5551; break;
    case DXGI_FORMAT_B5G6R5_UNORM:                  pfGuid = GUID_WICPixelFormat16bppBGR565; break;
    case DXGI_FORMAT_R32_FLOAT:                     pfGuid = GUID_WICPixelFormat32bppGrayFloat; break;
    case DXGI_FORMAT_R16_FLOAT:                     pfGuid = GUID_WICPixelFormat16bppGrayHalf; break;
    case DXGI_FORMAT_R16_UNORM:                     pfGuid = GUID_WICPixelFormat16bppGray; break;
    case DXGI_FORMAT_R8_UNORM:                      pfGuid = GUID_WICPixelFormat8bppGray; break;
    case DXGI_FORMAT_A8_UNORM:                      pfGuid = GUID_WICPixelFormat8bppAlpha; break;

    case DXGI_FORMAT_R8G8B8A8_UNORM:
        pfGuid = GUID_WICPixelFormat32bppRGBA;
        break;

    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        pfGuid = GUID_WICPixelFormat32bppRGBA;
        sRGB = true;
        break;

    case DXGI_FORMAT_B8G8R8A8_UNORM:
        pfGuid = GUID_WICPixelFormat32bppBGRA;
        break;

    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        pfGuid = GUID_WICPixelFormat32bppBGRA;
        sRGB = true;
        break;

    case DXGI_FORMAT_B8G8R8X8_UNORM:
        pfGuid = GUID_WICPixelFormat32bppBGR;
        break;

    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        pfGuid = GUID_WICPixelFormat32bppBGR;
        sRGB = true;
        break;

    default:
        DebugTrace("ERROR: ScreenGrab does not support all DXGI formats (%u). Consider using DirectXTex.\n", static_cast<uint32_t>(desc.Format));
        return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
    }

    auto pWIC = GetWIC();
    if (!pWIC)
        return E_NOINTERFACE;

    ComPtr<IWICStream> stream;
    hr = pWIC->CreateStream(stream.GetAddressOf());
    if (FAILED(hr))
        return hr;

    hr = stream->InitializeFromFilename(fileName, GENERIC_WRITE);
    if (FAILED(hr))
        return hr;

    auto_delete_file_wic delonfail(stream, fileName);

    ComPtr<IWICBitmapEncoder> encoder;
    hr = pWIC->CreateEncoder(guidContainerFormat, nullptr, encoder.GetAddressOf());
    if (FAILED(hr))
        return hr;

    hr = encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache);
    if (FAILED(hr))
        return hr;

    ComPtr<IWICBitmapFrameEncode> frame;
    ComPtr<IPropertyBag2> props;
    hr = encoder->CreateNewFrame(frame.GetAddressOf(), props.GetAddressOf());
    if (FAILED(hr))
        return hr;

    if (targetFormat && memcmp(&guidContainerFormat, &GUID_ContainerFormatBmp, sizeof(WICPixelFormatGUID)) == 0)
    {
        // Opt-in to the WIC2 support for writing 32-bit Windows BMP files with an alpha channel
        PROPBAG2 option = {};
        option.pstrName = const_cast<wchar_t*>(L"EnableV5Header32bppBGRA");

        VARIANT varValue;
        varValue.vt = VT_BOOL;
        varValue.boolVal = VARIANT_TRUE;
        std::ignore = props->Write(1, &option, &varValue);
    }

    if (setCustomProps)
    {
        setCustomProps(props.Get());
    }

    hr = frame->Initialize(props.Get());
    if (FAILED(hr))
        return hr;

    hr = frame->SetSize(static_cast<UINT>(desc.Width), desc.Height);
    if (FAILED(hr))
        return hr;

    hr = frame->SetResolution(72, 72);
    if (FAILED(hr))
        return hr;

    // Pick a target format
    WICPixelFormatGUID targetGuid = {};
    if (targetFormat)
    {
        targetGuid = *targetFormat;
    }
    else
    {
        // Screenshots don't typically include the alpha channel of the render target
        switch (desc.Format)
        {
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            targetGuid = GUID_WICPixelFormat96bppRGBFloat; // WIC 2
            break;

        case DXGI_FORMAT_R16G16B16A16_UNORM: targetGuid = GUID_WICPixelFormat48bppBGR; break;
        case DXGI_FORMAT_B5G5R5A1_UNORM:     targetGuid = GUID_WICPixelFormat16bppBGR555; break;
        case DXGI_FORMAT_B5G6R5_UNORM:       targetGuid = GUID_WICPixelFormat16bppBGR565; break;

        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_R16_UNORM:
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_A8_UNORM:
            targetGuid = GUID_WICPixelFormat8bppGray;
            break;

        default:
            targetGuid = GUID_WICPixelFormat24bppBGR;
            break;
        }
    }

    hr = frame->SetPixelFormat(&targetGuid);
    if (FAILED(hr))
        return hr;

    if (targetFormat && memcmp(targetFormat, &targetGuid, sizeof(WICPixelFormatGUID)) != 0)
    {
        // Requested output pixel format is not supported by the WIC codec
        return E_FAIL;
    }

    // Encode WIC metadata
    ComPtr<IWICMetadataQueryWriter> metawriter;
    if (SUCCEEDED(frame->GetMetadataQueryWriter(metawriter.GetAddressOf())))
    {
        PROPVARIANT value;
        PropVariantInit(&value);

        value.vt = VT_LPSTR;
        value.pszVal = const_cast<char*>("DirectXTK");

        if (memcmp(&guidContainerFormat, &GUID_ContainerFormatPng, sizeof(GUID)) == 0)
        {
            // Set Software name
            std::ignore = metawriter->SetMetadataByName(L"/tEXt/{str=Software}", &value);

            // Set sRGB chunk
            if (sRGB)
            {
                value.vt = VT_UI1;
                value.bVal = 0;
                std::ignore = metawriter->SetMetadataByName(L"/sRGB/RenderingIntent", &value);
            }
            else
            {
                // add gAMA chunk with gamma 1.0
                value.vt = VT_UI4;
                value.uintVal = 100000; // gama value * 100,000 -- i.e. gamma 1.0
                std::ignore = metawriter->SetMetadataByName(L"/gAMA/ImageGamma", &value);

                // remove sRGB chunk which is added by default.
                std::ignore = metawriter->RemoveMetadataByName(L"/sRGB/RenderingIntent");
            }
        }
    #if (defined(_XBOX_ONE) && defined(_TITLE)) || defined(_GAMING_XBOX)
        else if (memcmp(&guidContainerFormat, &GUID_ContainerFormatJpeg, sizeof(GUID)) == 0)
        {
            // Set Software name
            std::ignore = metawriter->SetMetadataByName(L"/app1/ifd/{ushort=305}", &value);

            if (sRGB)
            {
                // Set EXIF Colorspace of sRGB
                value.vt = VT_UI2;
                value.uiVal = 1;
                std::ignore = metawriter->SetMetadataByName(L"/app1/ifd/exif/{ushort=40961}", &value);
            }
        }
        else if (memcmp(&guidContainerFormat, &GUID_ContainerFormatTiff, sizeof(GUID)) == 0)
        {
            // Set Software name
            std::ignore = metawriter->SetMetadataByName(L"/ifd/{ushort=305}", &value);

            if (sRGB)
            {
                // Set EXIF Colorspace of sRGB
                value.vt = VT_UI2;
                value.uiVal = 1;
                std::ignore = metawriter->SetMetadataByName(L"/ifd/exif/{ushort=40961}", &value);
            }
        }
    #else
        else
        {
            // Set Software name
            std::ignore = metawriter->SetMetadataByName(L"System.ApplicationName", &value);

            if (sRGB)
            {
                // Set EXIF Colorspace of sRGB
                value.vt = VT_UI2;
                value.uiVal = 1;
                std::ignore = metawriter->SetMetadataByName(L"System.Image.ColorSpace", &value);
            }
        }
    #endif
    }

    const UINT64 imageSize = dstRowPitch * UINT64(desc.Height);
    if (imageSize > UINT32_MAX)
        return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);

    void* pMappedMemory = nullptr;
    D3D12_RANGE readRange = { 0, static_cast<SIZE_T>(imageSize) };
    D3D12_RANGE writeRange = { 0, 0 };
    hr = pStaging->Map(0, &readRange, &pMappedMemory);
    if (FAILED(hr))
        return hr;

    if (memcmp(&targetGuid, &pfGuid, sizeof(WICPixelFormatGUID)) != 0)
    {
        // Conversion required to write
        ComPtr<IWICBitmap> source;
        hr = pWIC->CreateBitmapFromMemory(static_cast<UINT>(desc.Width), desc.Height,
            pfGuid,
            static_cast<UINT>(dstRowPitch), static_cast<UINT>(imageSize),
            static_cast<BYTE*>(pMappedMemory), source.GetAddressOf());
        if (FAILED(hr))
        {
            pStaging->Unmap(0, &writeRange);
            return hr;
        }

        ComPtr<IWICFormatConverter> FC;
        hr = pWIC->CreateFormatConverter(FC.GetAddressOf());
        if (FAILED(hr))
        {
            pStaging->Unmap(0, &writeRange);
            return hr;
        }

        BOOL canConvert = FALSE;
        hr = FC->CanConvert(pfGuid, targetGuid, &canConvert);
        if (FAILED(hr) || !canConvert)
        {
            pStaging->Unmap(0, &writeRange);
            return E_UNEXPECTED;
        }

        hr = FC->Initialize(source.Get(), targetGuid, WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeMedianCut);
        if (FAILED(hr))
        {
            pStaging->Unmap(0, &writeRange);
            return hr;
        }

        WICRect rect = { 0, 0, static_cast<INT>(desc.Width), static_cast<INT>(desc.Height) };
        hr = frame->WriteSource(FC.Get(), &rect);
    }
    else
    {
        // No conversion required
        hr = frame->WritePixels(desc.Height, static_cast<UINT>(dstRowPitch), static_cast<UINT>(imageSize), static_cast<BYTE*>(pMappedMemory));
    }

    pStaging->Unmap(0, &writeRange);

    if (FAILED(hr))
        return hr;

    hr = frame->Commit();
    if (FAILED(hr))
        return hr;

    hr = encoder->Commit();
    if (FAILED(hr))
        return hr;

    delonfail.clear();

    return S_OK;
}
