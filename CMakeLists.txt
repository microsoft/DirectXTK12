# DirectX Tool Kit for DirectX 12
#
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.
#
# http://go.microsoft.com/fwlink/?LinkID=615561

cmake_minimum_required (VERSION 3.8)
project (DirectXTK_CMake LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/CMake")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/CMake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/CMake")

add_library (directxtk12 STATIC
    Inc/Audio.h
    Inc/CommonStates.h
    Inc/DDSTextureLoader.h
    Inc/DescriptorHeap.h
    Inc/DirectXHelpers.h
    Inc/Effects.h
    Inc/EffectPipelineStateDescription.h
    Inc/GamePad.h
    Inc/GeometricPrimitive.h
    Inc/GraphicsMemory.h
    Inc/Keyboard.h
    Inc/Model.h
    Inc/Mouse.h
    Inc/PostProcess.h
    Inc/PrimitiveBatch.h
    Inc/RenderTargetState.h
    Inc/ResourceUploadBatch.h
    Inc/ScreenGrab.h
    Inc/SimpleMath.h
    Inc/SimpleMath.inl
    Inc/SpriteBatch.h
    Inc/SpriteFont.h
    Inc/VertexTypes.h
    Inc/WICTextureLoader.h
    Audio/AudioEngine.cpp
    Audio/DynamicSoundEffectInstance.cpp
    Audio/SoundCommon.cpp
    Audio/SoundCommon.h
    Audio/SoundEffect.cpp
    Audio/SoundEffectInstance.cpp
    Audio/WaveBank.cpp
    Audio/WaveBankReader.cpp
    Audio/WaveBankReader.h
    Audio/WAVFileReader.cpp
    Audio/WAVFileReader.h
    Src/AlignedNew.h
    Src/AlphaTestEffect.cpp
    Src/BasicEffect.cpp
    Src/BasicPostProcess.cpp
    Src/Bezier.h
    Src/BinaryReader.cpp
    Src/BinaryReader.h
    Src/CommonStates.cpp
    Src/d3dx12.h
    Src/dds.h
    Src/DDSTextureLoader.cpp
    Src/DebugEffect.cpp
    Src/DemandCreate.h
    Src/DescriptorHeap.cpp
    Src/DirectXHelpers.cpp
    Src/DualPostProcess.cpp
    Src/DualTextureEffect.cpp
    Src/EffectCommon.cpp
    Src/EffectCommon.h
    Src/EffectFactory.cpp
    Src/EffectPipelineStateDescription.cpp
    Src/EffectTextureFactory.cpp
    Src/EnvironmentMapEffect.cpp
    Src/GamePad.cpp
    Src/GeometricPrimitive.cpp
    Src/Geometry.h
    Src/Geometry.cpp
    Src/GraphicsMemory.cpp
    Src/Keyboard.cpp
    Src/LinearAllocator.cpp
    Src/LinearAllocator.h
    Src/LoaderHelpers.h
    Src/Model.cpp
    Src/ModelLoadSDKMESH.cpp
    Src/ModelLoadVBO.cpp
    Src/Mouse.cpp
    Src/NormalMapEffect.cpp
    Src/PBREffect.cpp
    Src/PBREffectFactory.cpp
    Src/pch.h
    Src/PlatformHelpers.h
    Src/PrimitiveBatch.cpp
    Src/ResourceUploadBatch.cpp
    Src/ScreenGrab.cpp
    Src/SDKMesh.h
    Src/Shaders
    Src/SharedResourcePool.h
    Src/SimpleMath.cpp
    Src/SkinnedEffect.cpp
    Src/SpriteBatch.cpp
    Src/SpriteFont.cpp
    Src/TeapotData.inc
    Src/ToneMapPostProcess.cpp
    Src/vbo.h
    Src/VertexTypes.cpp
    Src/WICTextureLoader.cpp
)

source_group(Audio REGULAR_EXPRESSION Audio/*.*)
source_group(Inc REGULAR_EXPRESSION Inc/*.*)
source_group(Src REGULAR_EXPRESSION Src/*.*)

target_include_directories( directxtk12 PUBLIC Inc )
target_include_directories( directxtk12 PRIVATE Audio Src )

target_compile_options( directxtk12 PRIVATE /EHsc /GR /fp:fast  )

if ( CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
    target_compile_options( directxtk12 PRIVATE -Wall -Wpedantic -Wextra )
    if (${CMAKE_SIZEOF_VOID_P} EQUAL "4")
        target_compile_options( directxtk12 PRIVATE /arch:SSE2 )
    endif()
endif()
if ( CMAKE_CXX_COMPILER_ID MATCHES "MSVC" )
    target_compile_options( directxtk12 PRIVATE /Wall /permissive- /Zc:__cplusplus )
endif()

# Windows 10 is used here
add_compile_definitions(_UNICODE UNICODE _WIN32_WINNT=0x0A00)
