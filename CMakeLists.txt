# DirectX Tool Kit for DirectX 12
#
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.
#
# http://go.microsoft.com/fwlink/?LinkID=615561

cmake_minimum_required (VERSION 3.11)

project (DirectXTK12 LANGUAGES CXX)

option(BUILD_XAUDIO_WIN10 "Build for XAudio 2.9" ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/CMake")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/CMake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/CMake")

set(LIBRARY_SOURCES
    Inc/BufferHelpers.h
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
    Src/AlignedNew.h
    Src/AlphaTestEffect.cpp
    Src/BasicEffect.cpp
    Src/BasicPostProcess.cpp
    Src/Bezier.h
    Src/BinaryReader.cpp
    Src/BinaryReader.h
    Src/BufferHelpers.cpp
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
    Src/SharedResourcePool.h
    Src/SimpleMath.cpp
    Src/SkinnedEffect.cpp
    Src/SpriteBatch.cpp
    Src/SpriteFont.cpp
    Src/TeapotData.inc
    Src/ToneMapPostProcess.cpp
    Src/vbo.h
    Src/VertexTypes.cpp
    Src/WICTextureLoader.cpp)

set(SHADER_SOURCES
    Src/Shaders/AlphaTestEffect.fx
    Src/Shaders/BasicEffect.fx
    Src/Shaders/Common.fxh
    Src/Shaders/DebugEffect.fx
    Src/Shaders/DualTextureEffect.fx
    Src/Shaders/EnvironmentMapEffect.fx
    Src/Shaders/GenerateMips.hlsl
    Src/Shaders/Lighting.fxh
    Src/Shaders/NormalMapEffect.fx
    Src/Shaders/PBRCommon.fxh
    Src/Shaders/PBREffect.fx
    Src/Shaders/PixelPacking_Velocity.hlsli
    Src/Shaders/PostProcess.fx
    Src/Shaders/RootSig.fxh
    Src/Shaders/SkinnedEffect.fx
    Src/Shaders/SpriteEffect.fx
    Src/Shaders/Structures.fxh
    Src/Shaders/ToneMap.fx
    Src/Shaders/Utilities.fxh)

if(BUILD_XAUDIO_WIN10 MATCHES ON)
    set(LIBRARY_SOURCES ${LIBRARY_SOURCES}
        Inc/Audio.h
        Audio/AudioEngine.cpp
        Audio/DynamicSoundEffectInstance.cpp
        Audio/SoundCommon.cpp
        Audio/SoundCommon.h
        Audio/SoundEffect.cpp
        Audio/SoundEffectInstance.cpp
        Audio/SoundStreamInstance.cpp
        Audio/WaveBank.cpp
        Audio/WaveBankReader.cpp
        Audio/WaveBankReader.h
        Audio/WAVFileReader.cpp
        Audio/WAVFileReader.h)
endif()

add_library(${PROJECT_NAME} STATIC ${LIBRARY_SOURCES} Src/Shaders/Compiled/SpriteEffect_SpriteVertexShader.inc)

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.16")
    target_precompile_headers(${PROJECT_NAME} PRIVATE Src/pch.h)
endif()

add_custom_command(
    OUTPUT "${CMAKE_SOURCE_DIR}/Src/Shaders/Compiled/SpriteEffect_SpriteVertexShader.inc"
    MAIN_DEPENDENCY "${CMAKE_SOURCE_DIR}/Src/Shaders/CompileShaders.cmd"
    DEPENDS ${SHADER_SOURCES}
    COMMENT "Generating HLSL shaders..."
    COMMAND "CompileShaders.cmd"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Src/Shaders"
    USES_TERMINAL)

source_group(Audio REGULAR_EXPRESSION Audio/*.*)
source_group(Inc REGULAR_EXPRESSION Inc/*.*)
source_group(Src REGULAR_EXPRESSION Src/*.*)

target_include_directories(${PROJECT_NAME} PUBLIC Inc)
target_include_directories(${PROJECT_NAME} PRIVATE Src)

if(BUILD_XAUDIO_WIN10 MATCHES ON)
    target_include_directories(${PROJECT_NAME} PRIVATE Audio)
endif()

if(MSVC)
    # Use max Warning Level
    string(REPLACE "/W3 " "/Wall " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
    string(REPLACE "/W3 " "/Wall " CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
    string(REPLACE "/W3 " "/Wall " CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})

    # Model uses dynamic_cast, so we need /GR (Enable RTTI)

    # Library needs /EHsc (Enable C++ exceptions)
endif()

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /fp:fast)

    if (${CMAKE_SIZEOF_VOID_P} EQUAL "4")
        target_compile_options(${PROJECT_NAME} PRIVATE /arch:SSE2)
    endif()
endif()

if ( CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
    target_compile_options(${PROJECT_NAME} PRIVATE -Wpedantic -Wextra)
endif()
if ( CMAKE_CXX_COMPILER_ID MATCHES "MSVC" )
    target_compile_options(${PROJECT_NAME} PRIVATE /permissive- /JMC- /Zc:__cplusplus)

    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.26)
        target_compile_options(${PROJECT_NAME} PRIVATE /Zc:preprocessor /wd5104 /wd5105)
    endif()
endif()

if(WIN32)
    # Windows 10 is used here
    target_compile_definitions(${PROJECT_NAME} PRIVATE _UNICODE UNICODE _WIN32_WINNT=0x0A00)
endif()
