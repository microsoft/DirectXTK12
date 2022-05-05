//--------------------------------------------------------------------------------------
// File: ResourceUploadBatch.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "ResourceUploadBatch.h"

#include "DirectXHelpers.h"
#include "LoaderHelpers.h"
#include "PlatformHelpers.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

#ifdef __MINGW32__
const GUID IID_ID3D12Device = { 0x189819f1, 0x1db6, 0x4b57, { 0xbe, 0x54, 0x18, 0x21, 0x33, 0x9b, 0x85, 0xf7 } };
#endif

// Include the precompiled shader code.
namespace
{
#ifdef _GAMING_XBOX_SCARLETT
#include "XboxGamingScarlettGenerateMips_main.inc"
#elif defined(_GAMING_XBOX)
#include "XboxGamingXboxOneGenerateMips_main.inc"
#elif defined(_XBOX_ONE) && defined(_TITLE)
#include "XboxOneGenerateMips_main.inc"
#else
#include "GenerateMips_main.inc"
#endif

    bool FormatIsUAVCompatible(_In_ ID3D12Device* device, bool typedUAVLoadAdditionalFormats, DXGI_FORMAT format) noexcept
    {
        switch (format)
        {
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
            // Unconditionally supported.
            return true;

        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SINT:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SINT:
        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_R16_UINT:
        case DXGI_FORMAT_R16_SINT:
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8_SINT:
            // All these are supported if this optional feature is set.
            return typedUAVLoadAdditionalFormats;

        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R32G32_FLOAT:
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32_SINT:
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        case DXGI_FORMAT_R10G10B10A2_UINT:
        case DXGI_FORMAT_R11G11B10_FLOAT:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R16G16_FLOAT:
        case DXGI_FORMAT_R16G16_UNORM:
        case DXGI_FORMAT_R16G16_UINT:
        case DXGI_FORMAT_R16G16_SNORM:
        case DXGI_FORMAT_R16G16_SINT:
        case DXGI_FORMAT_R8G8_UNORM:
        case DXGI_FORMAT_R8G8_UINT:
        case DXGI_FORMAT_R8G8_SNORM:
        case DXGI_FORMAT_R8G8_SINT:
        case DXGI_FORMAT_R16_UNORM:
        case DXGI_FORMAT_R16_SNORM:
        case DXGI_FORMAT_R8_SNORM:
        case DXGI_FORMAT_A8_UNORM:
        case DXGI_FORMAT_B5G6R5_UNORM:
        case DXGI_FORMAT_B5G5R5A1_UNORM:
        case DXGI_FORMAT_B4G4R4A4_UNORM:
            // Conditionally supported by specific devices.
            if (typedUAVLoadAdditionalFormats)
            {
                D3D12_FEATURE_DATA_FORMAT_SUPPORT formatSupport = { format, D3D12_FORMAT_SUPPORT1_NONE, D3D12_FORMAT_SUPPORT2_NONE };
                if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &formatSupport, sizeof(formatSupport))))
                {
                    const DWORD mask = D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD | D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE;
                    return ((formatSupport.Support2 & mask) == mask);
                }
            }
            return false;

        default:
            return false;
        }
    }

    bool FormatIsBGR(DXGI_FORMAT format) noexcept
    {
        switch (format)
        {
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8X8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            return true;
        default:
            return false;
        }
    }

    bool FormatIsSRGB(DXGI_FORMAT format) noexcept
    {
        switch (format)
        {
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            return true;
        default:
            return false;
        }
    }

    DXGI_FORMAT ConvertSRVtoResourceFormat(DXGI_FORMAT format) noexcept
    {
        switch (format)
        {
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
            return DXGI_FORMAT_R32G32B32A32_TYPELESS;

        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_SINT:
            return DXGI_FORMAT_R16G16B16A16_TYPELESS;

        case DXGI_FORMAT_R32G32_FLOAT:
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32_SINT:
            return DXGI_FORMAT_R32G32_TYPELESS;

        case DXGI_FORMAT_R10G10B10A2_UNORM:
        case DXGI_FORMAT_R10G10B10A2_UINT:
            return DXGI_FORMAT_R10G10B10A2_TYPELESS;

        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_SINT:
            return DXGI_FORMAT_R8G8B8A8_TYPELESS;

        case DXGI_FORMAT_R16G16_FLOAT:
        case DXGI_FORMAT_R16G16_UNORM:
        case DXGI_FORMAT_R16G16_UINT:
        case DXGI_FORMAT_R16G16_SNORM:
        case DXGI_FORMAT_R16G16_SINT:
            return DXGI_FORMAT_R16G16_TYPELESS;

        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
            return DXGI_FORMAT_R32_TYPELESS;

        case DXGI_FORMAT_R8G8_UNORM:
        case DXGI_FORMAT_R8G8_UINT:
        case DXGI_FORMAT_R8G8_SNORM:
        case DXGI_FORMAT_R8G8_SINT:
            return DXGI_FORMAT_R8G8_TYPELESS;

        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_R16_UNORM:
        case DXGI_FORMAT_R16_UINT:
        case DXGI_FORMAT_R16_SNORM:
        case DXGI_FORMAT_R16_SINT:
            return DXGI_FORMAT_R16_TYPELESS;

        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8_SNORM:
        case DXGI_FORMAT_R8_SINT:
            return DXGI_FORMAT_R8_TYPELESS;

        default:
            return format;
        }
    }

    class GenerateMipsResources
    {
    public:
        enum RootParameterIndex
        {
            Constants,
            SourceTexture,
            TargetTexture,
            RootParameterCount
        };

    #pragma pack(push, 4)
        struct ConstantData
        {
            XMFLOAT2 InvOutTexelSize;
            uint32_t SrcMipIndex;
        };
    #pragma pack(pop)

        static constexpr uint32_t Num32BitConstants = static_cast<uint32_t>(sizeof(ConstantData) / sizeof(uint32_t));
        static constexpr uint32_t ThreadGroupSize = 8;

        ComPtr<ID3D12RootSignature> rootSignature;
        ComPtr<ID3D12PipelineState> generateMipsPSO;

        GenerateMipsResources(
            _In_ ID3D12Device* device)
        {
            rootSignature = CreateGenMipsRootSignature(device);
            generateMipsPSO = CreateGenMipsPipelineState(device, rootSignature.Get(), GenerateMips_main, sizeof(GenerateMips_main));
        }

    private:
        static ComPtr<ID3D12RootSignature> CreateGenMipsRootSignature(
            _In_ ID3D12Device* device)
        {
            ENUM_FLAGS_CONSTEXPR D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
                D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS
                | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
                | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
                | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
                | D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

            const CD3DX12_STATIC_SAMPLER_DESC sampler(
                0, // register
                D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT,
                D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

            const CD3DX12_DESCRIPTOR_RANGE sourceDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
            const CD3DX12_DESCRIPTOR_RANGE targetDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

            CD3DX12_ROOT_PARAMETER rootParameters[RootParameterIndex::RootParameterCount] = {};
            rootParameters[RootParameterIndex::Constants].InitAsConstants(Num32BitConstants, 0);
            rootParameters[RootParameterIndex::SourceTexture].InitAsDescriptorTable(1, &sourceDescriptorRange);
            rootParameters[RootParameterIndex::TargetTexture].InitAsDescriptorTable(1, &targetDescriptorRange);

            CD3DX12_ROOT_SIGNATURE_DESC rsigDesc;
            rsigDesc.Init(static_cast<UINT>(std::size(rootParameters)), rootParameters, 1, &sampler, rootSignatureFlags);

            ComPtr<ID3D12RootSignature> rootSignature;
            ThrowIfFailed(CreateRootSignature(device, &rsigDesc, rootSignature.ReleaseAndGetAddressOf()));

            SetDebugObjectName(rootSignature.Get(), L"GenerateMips RootSignature");

            return rootSignature;
        }

        static ComPtr<ID3D12PipelineState> CreateGenMipsPipelineState(
            _In_ ID3D12Device* device,
            _In_ ID3D12RootSignature* rootSignature,
            _In_reads_(bytecodeSize) const uint8_t* bytecode,
            _In_ size_t bytecodeSize)
        {
            D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
            desc.CS.BytecodeLength = bytecodeSize;
            desc.CS.pShaderBytecode = bytecode;
            desc.pRootSignature = rootSignature;

            ComPtr<ID3D12PipelineState> pso;
            ThrowIfFailed(device->CreateComputePipelineState(&desc, IID_GRAPHICS_PPV_ARGS(pso.GetAddressOf())));

            SetDebugObjectName(pso.Get(), L"GenerateMips PSO");

            return pso;
        }
    };
} // anonymous namespace

class ResourceUploadBatch::Impl
{
public:
    Impl(
        _In_ ID3D12Device* device) noexcept
        : mDevice(device)
        , mCommandType(D3D12_COMMAND_LIST_TYPE_DIRECT)
        , mInBeginEndBlock(false)
        , mTypedUAVLoadAdditionalFormats(false)
        , mStandardSwizzle64KBSupported(false)
    {
        assert(device != nullptr);
        D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
        if (SUCCEEDED(device->CheckFeatureSupport(
            D3D12_FEATURE_D3D12_OPTIONS,
            &options,
            sizeof(options))))
        {
            mTypedUAVLoadAdditionalFormats = options.TypedUAVLoadAdditionalFormats != 0;
            mStandardSwizzle64KBSupported = options.StandardSwizzle64KBSupported != 0;
        }
    }

    // Call this before your multiple calls to Upload.
    void Begin(D3D12_COMMAND_LIST_TYPE commandType)
    {
        if (mInBeginEndBlock)
            throw std::logic_error("Can't Begin: already in a Begin-End block.");

        switch (commandType)
        {
        case D3D12_COMMAND_LIST_TYPE_DIRECT:
        case D3D12_COMMAND_LIST_TYPE_COMPUTE:
        case D3D12_COMMAND_LIST_TYPE_COPY:
            break;

        default:
            DebugTrace("ResourceUploadBatch only supports Direct, Compute, and Copy command queues\n");
            throw std::invalid_argument("commandType parameter is invalid");
        }

        ThrowIfFailed(mDevice->CreateCommandAllocator(commandType, IID_GRAPHICS_PPV_ARGS(mCmdAlloc.ReleaseAndGetAddressOf())));

        SetDebugObjectName(mCmdAlloc.Get(), L"ResourceUploadBatch");

        ThrowIfFailed(mDevice->CreateCommandList(1, commandType, mCmdAlloc.Get(), nullptr, IID_GRAPHICS_PPV_ARGS(mList.ReleaseAndGetAddressOf())));

        SetDebugObjectName(mList.Get(), L"ResourceUploadBatch");

        mCommandType = commandType;
        mInBeginEndBlock = true;
    }

    // Asynchronously uploads a resource. The memory in subRes is copied.
    // The resource must be in the COPY_DEST state.
    void Upload(
        _In_ ID3D12Resource* resource,
        uint32_t subresourceIndexStart,
        _In_reads_(numSubresources) const D3D12_SUBRESOURCE_DATA* subRes,
        uint32_t numSubresources)
    {
        if (!mInBeginEndBlock)
            throw std::logic_error("Can't call Upload on a closed ResourceUploadBatch.");

        const UINT64 uploadSize = GetRequiredIntermediateSize(
            resource,
            subresourceIndexStart,
            numSubresources);

        const CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
        auto const resDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadSize);

        // Create a temporary buffer
        ComPtr<ID3D12Resource> scratchResource = nullptr;
        ThrowIfFailed(mDevice->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr, // D3D12_CLEAR_VALUE* pOptimizedClearValue
            IID_GRAPHICS_PPV_ARGS(scratchResource.GetAddressOf())));

        SetDebugObjectName(scratchResource.Get(), L"ResourceUploadBatch Temporary");

        // Submit resource copy to command list
        UpdateSubresources(mList.Get(), resource, scratchResource.Get(), 0, subresourceIndexStart, numSubresources,
        #if defined(_XBOX_ONE) && defined(_TITLE)
                    // Workaround for header constness issue
            const_cast<D3D12_SUBRESOURCE_DATA*>(subRes)
        #else
            subRes
        #endif
        );

        // Remember this upload object for delayed release
        mTrackedObjects.push_back(scratchResource);
    }

    void Upload(
        _In_ ID3D12Resource* resource,
        const SharedGraphicsResource& buffer)
    {
        if (!mInBeginEndBlock)
            throw std::logic_error("Can't call Upload on a closed ResourceUploadBatch.");

        // Submit resource copy to command list
        mList->CopyBufferRegion(resource, 0, buffer.Resource(), buffer.ResourceOffset(), buffer.Size());

        // Remember this upload resource for delayed release
        mTrackedMemoryResources.push_back(buffer);
    }

    // Asynchronously generate mips from a resource.
    // Resource must be in the PIXEL_SHADER_RESOURCE state
    void GenerateMips(_In_ ID3D12Resource* resource)
    {
        if (resource == nullptr)
        {
            throw std::invalid_argument("Nullptr passed to GenerateMips");
        }

        if (!mInBeginEndBlock)
            throw std::logic_error("Can't call GenerateMips on a closed ResourceUploadBatch.");

        if (mCommandType == D3D12_COMMAND_LIST_TYPE_COPY)
        {
            DebugTrace("ERROR: GenerateMips cannot operate on a copy queue\n");
            throw std::runtime_error("GenerateMips cannot operate on a copy queue");
        }

        const auto desc = resource->GetDesc();

        if (desc.MipLevels == 1)
        {
            // Nothing to do
            return;
        }
        if (desc.MipLevels == 0)
        {
            throw std::runtime_error("GenerateMips: texture has no mips");
        }
        if (desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D)
        {
            throw std::runtime_error("GenerateMips only supports Texture2D resources");
        }
        if (desc.DepthOrArraySize != 1)
        {
            throw std::runtime_error("GenerateMips only supports 2D textures of array size 1");
        }

        const bool uavCompat = FormatIsUAVCompatible(mDevice.Get(), mTypedUAVLoadAdditionalFormats, desc.Format);

        if (!uavCompat && !FormatIsSRGB(desc.Format) && !FormatIsBGR(desc.Format))
        {
            throw std::runtime_error("GenerateMips doesn't support this texture format on this device");
        }

        // Ensure that we have valid generate mips data
        if (mGenMipsResources == nullptr)
        {
            mGenMipsResources = std::make_unique<GenerateMipsResources>(mDevice.Get());
        }

        // If the texture's format doesn't support UAVs we'll have to copy it to a texture that does first.
        // This is true of BGRA or sRGB textures, for example.
        if (uavCompat)
        {
            GenerateMips_UnorderedAccessPath(resource);
        }
        else if (!mTypedUAVLoadAdditionalFormats)
        {
            throw std::runtime_error("GenerateMips needs TypedUAVLoadAdditionalFormats device support for sRGB/BGR");
        }
        else if (FormatIsBGR(desc.Format))
        {
        #if !defined(_GAMING_XBOX) && !(defined(_XBOX_ONE) && defined(_TITLE))
            if (!mStandardSwizzle64KBSupported)
            {
                throw std::runtime_error("GenerateMips needs StandardSwizzle64KBSupported device support for BGR");
            }
        #endif

            GenerateMips_TexturePathBGR(resource);
        }
        else
        {
            GenerateMips_TexturePath(resource);
        }
    }

    // Transition a resource once you're done with it
    void Transition(
        _In_ ID3D12Resource* resource,
        _In_ D3D12_RESOURCE_STATES stateBefore,
        _In_ D3D12_RESOURCE_STATES stateAfter)
    {
        if (!mInBeginEndBlock)
            throw std::logic_error("Can't call Upload on a closed ResourceUploadBatch.");

        if (mCommandType == D3D12_COMMAND_LIST_TYPE_COPY)
        {
            switch (stateAfter)
            {
            case D3D12_RESOURCE_STATE_COPY_DEST:
            case D3D12_RESOURCE_STATE_COPY_SOURCE:
                break;

            default:
                // Ignore other states for copy queues.
                return;
            }
        }
        else if (mCommandType == D3D12_COMMAND_LIST_TYPE_COMPUTE)
        {
            switch (stateAfter)
            {
            case D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER:
            case D3D12_RESOURCE_STATE_UNORDERED_ACCESS:
            case D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:
            case D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT:
            case D3D12_RESOURCE_STATE_COPY_DEST:
            case D3D12_RESOURCE_STATE_COPY_SOURCE:
                break;

            default:
                // Ignore other states for compute queues.
                return;
            }
        }

        TransitionResource(mList.Get(), resource, stateBefore, stateAfter);
    }

    // Submits all the uploads to the driver.
    // No more uploads can happen after this call until Begin is called again.
    // This returns a handle to an event that can be waited on.
    std::future<void> End(
        _In_ ID3D12CommandQueue* commandQueue)
    {
        if (!mInBeginEndBlock)
            throw std::logic_error("ResourceUploadBatch already closed.");

        ThrowIfFailed(mList->Close());

        // Submit the job to the GPU
        commandQueue->ExecuteCommandLists(1, CommandListCast(mList.GetAddressOf()));

        // Set an event so we get notified when the GPU has completed all its work
        ComPtr<ID3D12Fence> fence;
        ThrowIfFailed(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_GRAPHICS_PPV_ARGS(fence.GetAddressOf())));

        SetDebugObjectName(fence.Get(), L"ResourceUploadBatch");

        HANDLE gpuCompletedEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
        if (!gpuCompletedEvent)
            throw std::system_error(std::error_code(static_cast<int>(GetLastError()), std::system_category()), "CreateEventEx");

        ThrowIfFailed(commandQueue->Signal(fence.Get(), 1ULL));
        ThrowIfFailed(fence->SetEventOnCompletion(1ULL, gpuCompletedEvent));

        // Create a packet of data that'll be passed to our waiting upload thread
        auto uploadBatch = new UploadBatch();
        uploadBatch->CommandList = mList;
        uploadBatch->Fence = fence;
        uploadBatch->GpuCompleteEvent.reset(gpuCompletedEvent);
        std::swap(mTrackedObjects, uploadBatch->TrackedObjects);
        std::swap(mTrackedMemoryResources, uploadBatch->TrackedMemoryResources);

        // Kick off a thread that waits for the upload to complete on the GPU timeline.
        // Let the thread run autonomously, but provide a future the user can wait on.
        std::future<void> future = std::async(std::launch::async, [uploadBatch]()
            {
                // Wait on the GPU-complete notification
                const DWORD wr = WaitForSingleObject(uploadBatch->GpuCompleteEvent.get(), INFINITE);
                if (wr != WAIT_OBJECT_0)
                {
                    if (wr == WAIT_FAILED)
                    {
                        throw std::system_error(std::error_code(static_cast<int>(GetLastError()), std::system_category()), "WaitForSingleObject");
                    }
                    else
                    {
                        throw std::runtime_error("WaitForSingleObject");
                    }
                }

                // Delete the batch
                // Because the vectors contain smart-pointers, their destructors will
                // fire and the resources will be released.
                delete uploadBatch;
            });

            // Reset our state
        mCommandType = D3D12_COMMAND_LIST_TYPE_DIRECT;
        mInBeginEndBlock = false;
        mList.Reset();
        mCmdAlloc.Reset();

        // Swap above should have cleared these
        assert(mTrackedObjects.empty());
        assert(mTrackedMemoryResources.empty());

        return future;
    }

    bool IsSupportedForGenerateMips(DXGI_FORMAT format) noexcept
    {
        if (mCommandType == D3D12_COMMAND_LIST_TYPE_COPY)
            return false;

        if (FormatIsUAVCompatible(mDevice.Get(), mTypedUAVLoadAdditionalFormats, format))
            return true;

        if (FormatIsBGR(format))
        {
        #if defined(_GAMING_XBOX) || (defined(_XBOX_ONE) && defined(_TITLE))
                    // We know the RGB and BGR memory layouts match for Xbox One
            return true;
        #else
                    // BGR path requires DXGI_FORMAT_R8G8B8A8_UNORM support for UAV load/store plus matching layouts
            return mTypedUAVLoadAdditionalFormats && mStandardSwizzle64KBSupported;
        #endif
        }

        if (FormatIsSRGB(format))
        {
            // sRGB path requires DXGI_FORMAT_R8G8B8A8_UNORM support for UAV load/store
            return mTypedUAVLoadAdditionalFormats;
        }

        return false;
    }

private:
    // Resource is UAV compatible
    void GenerateMips_UnorderedAccessPath(
        _In_ ID3D12Resource* resource)
    {
        const auto desc = resource->GetDesc();
        assert(!FormatIsBGR(desc.Format) && !FormatIsSRGB(desc.Format));

        const CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

        assert(mCommandType != D3D12_COMMAND_LIST_TYPE_COPY);
        const D3D12_RESOURCE_STATES originalState = (mCommandType == D3D12_COMMAND_LIST_TYPE_COMPUTE)
            ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

        // Create a staging resource if we have to
        ComPtr<ID3D12Resource> staging;
        if ((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) == 0)
        {
            D3D12_RESOURCE_DESC stagingDesc = desc;
            stagingDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            stagingDesc.Format = ConvertSRVtoResourceFormat(desc.Format);

            ThrowIfFailed(mDevice->CreateCommittedResource(
                &defaultHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &stagingDesc,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(staging.GetAddressOf())));

            SetDebugObjectName(staging.Get(), L"GenerateMips Staging");

            // Copy the top mip of resource to staging
            TransitionResource(mList.Get(), resource, originalState, D3D12_RESOURCE_STATE_COPY_SOURCE);

            const CD3DX12_TEXTURE_COPY_LOCATION src(resource, 0);
            const CD3DX12_TEXTURE_COPY_LOCATION dst(staging.Get(), 0);
            mList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

            TransitionResource(mList.Get(), staging.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        }
        else
        {
            // Resource is already a UAV so we can do this in-place
            staging = resource;

            TransitionResource(mList.Get(), staging.Get(), originalState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        }

        // Create a descriptor heap that holds our resource descriptors
        ComPtr<ID3D12DescriptorHeap> descriptorHeap;
        D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
        descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        descriptorHeapDesc.NumDescriptors = desc.MipLevels;
        mDevice->CreateDescriptorHeap(&descriptorHeapDesc, IID_GRAPHICS_PPV_ARGS(descriptorHeap.GetAddressOf()));

        SetDebugObjectName(descriptorHeap.Get(), L"ResourceUploadBatch");

        auto const descriptorSize = static_cast<int>(mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

        // Create the top-level SRV
        CD3DX12_CPU_DESCRIPTOR_HANDLE handleIt(descriptorHeap->GetCPUDescriptorHandleForHeapStart());
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = desc.Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = desc.MipLevels;

        mDevice->CreateShaderResourceView(staging.Get(), &srvDesc, handleIt);

        // Create the UAVs for the tail
        for (uint16_t mip = 1; mip < desc.MipLevels; ++mip)
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.Format = desc.Format;
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            uavDesc.Texture2D.MipSlice = mip;

            handleIt.Offset(descriptorSize);
            mDevice->CreateUnorderedAccessView(staging.Get(), nullptr, &uavDesc, handleIt);
        }

        // Set up UAV barrier (used in loop)
        D3D12_RESOURCE_BARRIER barrierUAV = {};
        barrierUAV.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrierUAV.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrierUAV.UAV.pResource = staging.Get();

        // Barrier for transitioning the subresources to UAVs
        D3D12_RESOURCE_BARRIER srv2uavDesc = {};
        srv2uavDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        srv2uavDesc.Transition.pResource = staging.Get();
        srv2uavDesc.Transition.Subresource = 0;
        srv2uavDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        srv2uavDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

        // Barrier for transitioning the subresources to SRVs
        D3D12_RESOURCE_BARRIER uav2srvDesc = {};
        uav2srvDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        uav2srvDesc.Transition.pResource = staging.Get();
        uav2srvDesc.Transition.Subresource = 0;
        uav2srvDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        uav2srvDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

        // based on format, select srgb or not
        ComPtr<ID3D12PipelineState> pso = mGenMipsResources->generateMipsPSO;

        // Set up state
        mList->SetComputeRootSignature(mGenMipsResources->rootSignature.Get());
        mList->SetPipelineState(pso.Get());
        mList->SetDescriptorHeaps(1, descriptorHeap.GetAddressOf());
        mList->SetComputeRootDescriptorTable(GenerateMipsResources::SourceTexture, descriptorHeap->GetGPUDescriptorHandleForHeapStart());

        // Get the descriptor handle -- uavH will increment over each loop
        CD3DX12_GPU_DESCRIPTOR_HANDLE uavH(
            descriptorHeap->GetGPUDescriptorHandleForHeapStart(),
            descriptorSize); // offset by 1 descriptor

        // Process each mip
        auto mipWidth = static_cast<uint32_t>(desc.Width);
        uint32_t mipHeight = desc.Height;
        for (uint32_t mip = 1; mip < desc.MipLevels; ++mip)
        {
            mipWidth = std::max<uint32_t>(1, mipWidth >> 1);
            mipHeight = std::max<uint32_t>(1, mipHeight >> 1);

            // Transition the mip to a UAV
            srv2uavDesc.Transition.Subresource = mip;
            mList->ResourceBarrier(1, &srv2uavDesc);

            // Bind the mip subresources
            mList->SetComputeRootDescriptorTable(GenerateMipsResources::TargetTexture, uavH);

            // Set constants
            GenerateMipsResources::ConstantData constants;
            constants.SrcMipIndex = mip - 1;
            constants.InvOutTexelSize = XMFLOAT2(1 / float(mipWidth), 1 / float(mipHeight));
            mList->SetComputeRoot32BitConstants(
                GenerateMipsResources::Constants,
                GenerateMipsResources::Num32BitConstants,
                &constants,
                0);

            // Process this mip
            mList->Dispatch(
                (mipWidth + GenerateMipsResources::ThreadGroupSize - 1) / GenerateMipsResources::ThreadGroupSize,
                (mipHeight + GenerateMipsResources::ThreadGroupSize - 1) / GenerateMipsResources::ThreadGroupSize,
                1);

            mList->ResourceBarrier(1, &barrierUAV);

            // Transition the mip to an SRV
            uav2srvDesc.Transition.Subresource = mip;
            mList->ResourceBarrier(1, &uav2srvDesc);

            // Offset the descriptor heap handles
            uavH.Offset(descriptorSize);
        }

        // If the staging resource is NOT the same as the resource, we need to copy everything back
        if (staging.Get() != resource)
        {
            // Transition the resources ready for copy
            D3D12_RESOURCE_BARRIER barrier[2] = {};
            barrier[0].Type = barrier[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier[0].Transition.Subresource = barrier[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barrier[0].Transition.pResource = staging.Get();
            barrier[0].Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
            barrier[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;

            barrier[1].Transition.pResource = resource;
            barrier[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
            barrier[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;

            mList->ResourceBarrier(2, barrier);

            // Copy the entire resource back
            mList->CopyResource(resource, staging.Get());

            // Transition the target resource back to pixel shader resource
            TransitionResource(mList.Get(), resource, D3D12_RESOURCE_STATE_COPY_DEST, originalState);

            mTrackedObjects.push_back(staging);
        }
        else
        {
            TransitionResource(mList.Get(), staging.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, originalState);
        }

        // Add our temporary objects to the deferred deletion queue
        mTrackedObjects.push_back(mGenMipsResources->rootSignature);
        mTrackedObjects.push_back(pso);
        mTrackedObjects.push_back(resource);
        mTrackedObjects.push_back(descriptorHeap);
    }

    // Resource is not UAV compatible
    void GenerateMips_TexturePath(
        _In_ ID3D12Resource* resource)
    {
        const auto resourceDesc = resource->GetDesc();
        assert(!FormatIsBGR(resourceDesc.Format) || FormatIsSRGB(resourceDesc.Format));

        auto copyDesc = resourceDesc;
        copyDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        copyDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);

        // Create a resource with the same description, but without SRGB, and with UAV flags
        ComPtr<ID3D12Resource> resourceCopy;
        ThrowIfFailed(mDevice->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &copyDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(resourceCopy.GetAddressOf())));

        SetDebugObjectName(resourceCopy.Get(), L"GenerateMips Resource Copy");

        assert(mCommandType != D3D12_COMMAND_LIST_TYPE_COPY);
        const D3D12_RESOURCE_STATES originalState = mCommandType == D3D12_COMMAND_LIST_TYPE_COMPUTE
            ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

        // Copy the top mip of resource data
        TransitionResource(mList.Get(), resource, originalState, D3D12_RESOURCE_STATE_COPY_SOURCE);

        const CD3DX12_TEXTURE_COPY_LOCATION src(resource, 0);
        const CD3DX12_TEXTURE_COPY_LOCATION dst(resourceCopy.Get(), 0);
        mList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

        TransitionResource(mList.Get(), resourceCopy.Get(), D3D12_RESOURCE_STATE_COPY_DEST, originalState);

        // Generate the mips
        GenerateMips_UnorderedAccessPath(resourceCopy.Get());

        // Direct copy back
        D3D12_RESOURCE_BARRIER barrier[2] = {};
        barrier[0].Type = barrier[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier[0].Transition.Subresource = barrier[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier[0].Transition.pResource = resourceCopy.Get();
        barrier[0].Transition.StateBefore = originalState;
        barrier[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;

        barrier[1].Transition.pResource = resource;
        barrier[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
        barrier[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;

        mList->ResourceBarrier(2, barrier);

        // Copy the entire resource back
        mList->CopyResource(resource, resourceCopy.Get());

        TransitionResource(mList.Get(), resource, D3D12_RESOURCE_STATE_COPY_DEST, originalState);

        // Track these object lifetimes on the GPU
        mTrackedObjects.push_back(resourceCopy);
        mTrackedObjects.push_back(resource);
    }

    // Resource is not UAV compatible (copy via alias to avoid validation failure)
    void GenerateMips_TexturePathBGR(
        _In_ ID3D12Resource* resource)
    {
        const auto resourceDesc = resource->GetDesc();
        assert(FormatIsBGR(resourceDesc.Format));

        // Create a resource with the same description with RGB and with UAV flags
        auto copyDesc = resourceDesc;
        copyDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        copyDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    #if !defined(_GAMING_XBOX) && !(defined(_XBOX_ONE) && defined(_TITLE))
        copyDesc.Layout = D3D12_TEXTURE_LAYOUT_64KB_STANDARD_SWIZZLE;
    #endif

        D3D12_HEAP_DESC heapDesc = {};
        auto const allocInfo = mDevice->GetResourceAllocationInfo(0, 1, &copyDesc);
        heapDesc.SizeInBytes = allocInfo.SizeInBytes;
        heapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
        heapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;

        ComPtr<ID3D12Heap> heap;
        ThrowIfFailed(mDevice->CreateHeap(&heapDesc, IID_GRAPHICS_PPV_ARGS(heap.GetAddressOf())));

        SetDebugObjectName(heap.Get(), L"ResourceUploadBatch");

        ComPtr<ID3D12Resource> resourceCopy;
        ThrowIfFailed(mDevice->CreatePlacedResource(
            heap.Get(),
            0,
            &copyDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(resourceCopy.GetAddressOf())));

        SetDebugObjectName(resourceCopy.Get(), L"GenerateMips Resource Copy");

        // Create a BGRA alias
        auto aliasDesc = resourceDesc;
        aliasDesc.Format = (resourceDesc.Format == DXGI_FORMAT_B8G8R8X8_UNORM || resourceDesc.Format == DXGI_FORMAT_B8G8R8X8_UNORM_SRGB) ? DXGI_FORMAT_B8G8R8X8_UNORM : DXGI_FORMAT_B8G8R8A8_UNORM;
        aliasDesc.Layout = copyDesc.Layout;
        aliasDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        ComPtr<ID3D12Resource> aliasCopy;
        ThrowIfFailed(mDevice->CreatePlacedResource(
            heap.Get(),
            0,
            &aliasDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(aliasCopy.GetAddressOf())));

        SetDebugObjectName(aliasCopy.Get(), L"GenerateMips BGR Alias Copy");

        assert(mCommandType != D3D12_COMMAND_LIST_TYPE_COPY);
        const D3D12_RESOURCE_STATES originalState = (mCommandType == D3D12_COMMAND_LIST_TYPE_COMPUTE)
            ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

        // Copy the top mip of the resource data BGR to RGB
        D3D12_RESOURCE_BARRIER aliasBarrier[3] = {};
        aliasBarrier[0].Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
        aliasBarrier[0].Aliasing.pResourceAfter = aliasCopy.Get();

        aliasBarrier[1].Type = aliasBarrier[2].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        aliasBarrier[1].Transition.Subresource = aliasBarrier[2].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        aliasBarrier[1].Transition.pResource = resource;
        aliasBarrier[1].Transition.StateBefore = originalState;
        aliasBarrier[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;

        mList->ResourceBarrier(2, aliasBarrier);

        const CD3DX12_TEXTURE_COPY_LOCATION src(resource, 0);
        const CD3DX12_TEXTURE_COPY_LOCATION dst(aliasCopy.Get(), 0);
        mList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

        // Generate the mips
        aliasBarrier[0].Aliasing.pResourceBefore = aliasCopy.Get();
        aliasBarrier[0].Aliasing.pResourceAfter = resourceCopy.Get();

        aliasBarrier[1].Transition.pResource = resourceCopy.Get();
        aliasBarrier[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        aliasBarrier[1].Transition.StateAfter = originalState;

        mList->ResourceBarrier(2, aliasBarrier);
        GenerateMips_UnorderedAccessPath(resourceCopy.Get());

        // Direct copy back RGB to BGR
        aliasBarrier[0].Aliasing.pResourceBefore = resourceCopy.Get();
        aliasBarrier[0].Aliasing.pResourceAfter = aliasCopy.Get();

        aliasBarrier[1].Transition.pResource = aliasCopy.Get();
        aliasBarrier[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        aliasBarrier[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;

        aliasBarrier[2].Transition.pResource = resource;
        aliasBarrier[2].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
        aliasBarrier[2].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;

        mList->ResourceBarrier(3, aliasBarrier);

        // Copy the entire resource back
        mList->CopyResource(resource, aliasCopy.Get());
        TransitionResource(mList.Get(), resource, D3D12_RESOURCE_STATE_COPY_DEST, originalState);

        // Track these object lifetimes on the GPU
        mTrackedObjects.push_back(heap);
        mTrackedObjects.push_back(resourceCopy);
        mTrackedObjects.push_back(aliasCopy);
        mTrackedObjects.push_back(resource);
    }

    struct UploadBatch
    {
        std::vector<ComPtr<ID3D12DeviceChild>>  TrackedObjects;
        std::vector<SharedGraphicsResource>     TrackedMemoryResources;
        ComPtr<ID3D12GraphicsCommandList>       CommandList;
        ComPtr<ID3D12Fence>                     Fence;
        ScopedHandle                            GpuCompleteEvent;

        UploadBatch() noexcept {}
    };

    ComPtr<ID3D12Device>                        mDevice;
    ComPtr<ID3D12CommandAllocator>              mCmdAlloc;
    ComPtr<ID3D12GraphicsCommandList>           mList;
    std::unique_ptr<GenerateMipsResources>      mGenMipsResources;

    std::vector<ComPtr<ID3D12DeviceChild>>      mTrackedObjects;
    std::vector<SharedGraphicsResource>         mTrackedMemoryResources;

    D3D12_COMMAND_LIST_TYPE                     mCommandType;
    bool                                        mInBeginEndBlock;
    bool                                        mTypedUAVLoadAdditionalFormats;
    bool                                        mStandardSwizzle64KBSupported;
};



// Public constructor.
ResourceUploadBatch::ResourceUploadBatch(_In_ ID3D12Device* device) noexcept(false)
    : pImpl(std::make_unique<Impl>(device))
{
}


ResourceUploadBatch::ResourceUploadBatch(ResourceUploadBatch&&) noexcept = default;
ResourceUploadBatch& ResourceUploadBatch::operator= (ResourceUploadBatch&&) noexcept = default;
ResourceUploadBatch::~ResourceUploadBatch() = default;


void ResourceUploadBatch::Begin(D3D12_COMMAND_LIST_TYPE commandType)
{
    pImpl->Begin(commandType);
}


_Use_decl_annotations_
void ResourceUploadBatch::Upload(
    ID3D12Resource* resource,
    uint32_t subresourceIndexStart,
    const D3D12_SUBRESOURCE_DATA* subRes,
    uint32_t numSubresources)
{
    pImpl->Upload(resource, subresourceIndexStart, subRes, numSubresources);
}


_Use_decl_annotations_
void ResourceUploadBatch::Upload(
    ID3D12Resource* resource,
    const SharedGraphicsResource& buffer
)
{
    pImpl->Upload(resource, buffer);
}



void ResourceUploadBatch::GenerateMips(_In_ ID3D12Resource* resource)
{
    pImpl->GenerateMips(resource);
}


_Use_decl_annotations_
void ResourceUploadBatch::Transition(
    ID3D12Resource* resource,
    D3D12_RESOURCE_STATES stateBefore,
    D3D12_RESOURCE_STATES stateAfter)
{
    pImpl->Transition(resource, stateBefore, stateAfter);
}


std::future<void> ResourceUploadBatch::End(_In_ ID3D12CommandQueue* commandQueue)
{
    return pImpl->End(commandQueue);
}


bool __cdecl ResourceUploadBatch::IsSupportedForGenerateMips(DXGI_FORMAT format) noexcept
{
    return pImpl->IsSupportedForGenerateMips(format);
}
