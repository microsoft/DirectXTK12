# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required (VERSION 3.11)

set(DIRECTXTK12_VERSION 1.4.8)

project (DirectXTK12
  VERSION ${DIRECTXTK12_VERSION}
  DESCRIPTION "DirectX Tool Kit for DirectX 12"
  HOMEPAGE_URL "https://go.microsoft.com/fwlink/?LinkID=615561"
  LANGUAGES CXX)

option(BUILD_XAUDIO_WIN10 "Build for XAudio 2.9" ON)

option(ENABLE_CODE_ANALYSIS "Use Static Code Analysis on build" OFF)

option(USE_PREBUILT_SHADERS "Use externally built HLSL shaders" OFF)

option(BUILD_DXIL_SHADERS "Use DXC Shader Model 6 for shaders" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/CMake")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/CMake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/CMake")

#--- Library
set(LIBRARY_HEADERS
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
    Inc/WICTextureLoader.h)

set(LIBRARY_SOURCES
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
    Src/DDS.h
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
    Src/ModelLoadCMO.cpp
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
    Src/Shaders/Skinning.fxh
    Src/Shaders/SpriteEffect.fx
    Src/Shaders/Structures.fxh
    Src/Shaders/ToneMap.fx
    Src/Shaders/Utilities.fxh)

if(BUILD_XAUDIO_WIN10)
    set(LIBRARY_HEADERS ${LIBRARY_HEADERS}
        Inc/Audio.h)

    set(LIBRARY_SOURCES ${LIBRARY_SOURCES}
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

if (NOT COMPILED_SHADERS)
    if (USE_PREBUILT_SHADERS)
        message(FATAL_ERROR "ERROR: Using prebuilt shaders requires the COMPILED_SHADERS variable is set")
    endif()
    set(COMPILED_SHADERS ${CMAKE_CURRENT_BINARY_DIR}/Shaders/Compiled)
    file(MAKE_DIRECTORY ${COMPILED_SHADERS})
endif()

set(LIBRARY_SOURCES ${LIBRARY_SOURCES}
    ${COMPILED_SHADERS}/SpriteEffect_SpriteVertexShader.inc)

if(BUILD_DXIL_SHADERS)
    message("INFO: Using Shader Model 6.0 (DXC.EXE) for shaders.")
    set(ShaderOpts "dxil")
else()
    message("INFO: Using Shader Model 5.1 (FXC.EXE) for shaders.")
endif()

if (NOT USE_PREBUILT_SHADERS)
    add_custom_command(
        OUTPUT "${COMPILED_SHADERS}/SpriteEffect_SpriteVertexShader.inc"
        MAIN_DEPENDENCY "${PROJECT_SOURCE_DIR}/Src/Shaders/CompileShaders.cmd"
        DEPENDS ${SHADER_SOURCES}
        COMMENT "Generating HLSL shaders..."
        COMMAND set CompileShadersOutput=${COMPILED_SHADERS}
        COMMAND CompileShaders.cmd ARGS ${ShaderOpts}
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/Src/Shaders"
        USES_TERMINAL)
endif()

add_library(${PROJECT_NAME} STATIC ${LIBRARY_SOURCES} ${LIBRARY_HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE ${COMPILED_SHADERS})

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.16")
    target_precompile_headers(${PROJECT_NAME} PRIVATE Src/pch.h)
endif()

source_group(Audio REGULAR_EXPRESSION Audio/*.*)
source_group(Inc REGULAR_EXPRESSION Inc/*.*)
source_group(Src REGULAR_EXPRESSION Src/*.*)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Inc>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}>)

if(BUILD_XAUDIO_WIN10)
    message("INFO: Using DirectX Tool Kit for Audio on XAudio 2.9.")
    target_include_directories(${PROJECT_NAME} PRIVATE Src Audio)
endif()

if(MSVC)
    # Use max Warning Level
    string(REPLACE "/W3 " "/Wall " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
    string(REPLACE "/W3 " "/Wall " CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
    string(REPLACE "/W3 " "/Wall " CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})

    # Model uses dynamic_cast, so we need /GR (Enable RTTI)

    # Library needs /EHsc (Enable C++ exceptions)
endif()

if (VCPKG_TOOLCHAIN)
    message("INFO: Using VCPKG for DirectX-Headers and DirectXMath.")
    find_package(directx-headers CONFIG REQUIRED)
    find_package(directxmath CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE Microsoft::DirectX-Headers Microsoft::DirectXMath)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USING_DIRECTX_HEADERS)
endif()

#--- Package
include(CMakePackageConfigHelpers)

string(TOLOWER ${PROJECT_NAME} PACKAGE_NAME)

write_basic_package_version_file(
  ${PROJECT_BINARY_DIR}/cmake/${PACKAGE_NAME}-config-version.cmake
  VERSION ${DIRECTXTK12_VERSION}
  COMPATIBILITY AnyNewerVersion)

install(TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}-targets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin)

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/.nuget/${PROJECT_NAME}-config.cmake.in
  ${PROJECT_BINARY_DIR}/cmake/${PACKAGE_NAME}-config.cmake
  INSTALL_DESTINATION cmake/})

install(EXPORT ${PROJECT_NAME}-targets
  FILE ${PROJECT_NAME}-targets.cmake
  NAMESPACE Microsoft::
  DESTINATION cmake/)

install(FILES ${LIBRARY_HEADERS}
    DESTINATION include/${PROJECT_NAME})

install(FILES
    ${PROJECT_BINARY_DIR}/cmake/${PACKAGE_NAME}-config.cmake
    ${PROJECT_BINARY_DIR}/cmake/${PACKAGE_NAME}-config-version.cmake
  DESTINATION cmake/)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /fp:fast)

    if (${CMAKE_SIZEOF_VOID_P} EQUAL "4" AND (NOT ${CMAKE_VS_PLATFORM_NAME} MATCHES "arm"))
        target_compile_options(${PROJECT_NAME} PRIVATE /arch:SSE2)
    endif()
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wpedantic -Wextra)
endif()
if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(${PROJECT_NAME} PRIVATE /permissive- /JMC- /Zc:__cplusplus)

   if(ENABLE_CODE_ANALYSIS)
      target_compile_options(${PROJECT_NAME} PRIVATE /analyze)
   endif()

    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.26)
        target_compile_options(${PROJECT_NAME} PRIVATE /Zc:preprocessor /wd5104 /wd5105)
    endif()
endif()

if(WIN32)
    # Windows 10 is used here
    target_compile_definitions(${PROJECT_NAME} PRIVATE _UNICODE UNICODE _WIN32_WINNT=0x0A00)
endif()

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
