# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required (VERSION 3.18)

set(DIRECTXTK12_VERSION 1.4.9)

project (DirectXTK12
  VERSION ${DIRECTXTK12_VERSION}
  DESCRIPTION "DirectX Tool Kit for DirectX 12"
  HOMEPAGE_URL "https://go.microsoft.com/fwlink/?LinkID=615561"
  LANGUAGES CXX)

option(BUILD_XAUDIO_WIN10 "Build for XAudio 2.9" ON)
option(BUILD_XAUDIO_REDIST "Build for XAudio2Redist" OFF)

option(ENABLE_CODE_ANALYSIS "Use Static Code Analysis on build" OFF)

option(USE_PREBUILT_SHADERS "Use externally built HLSL shaders" OFF)

option(BUILD_DXIL_SHADERS "Use DXC Shader Model 6 for shaders" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/CMake")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/CMake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/CMake")

if (DEFINED VCPKG_TARGET_ARCHITECTURE)
    set(DIRECTX_ARCH ${VCPKG_TARGET_ARCHITECTURE})
elseif(CMAKE_GENERATOR_PLATFORM MATCHES "^[Ww][Ii][Nn]32$")
    set(DIRECTX_ARCH x86)
elseif(CMAKE_GENERATOR_PLATFORM MATCHES "^[Xx]64$")
    set(DIRECTX_ARCH x64)
elseif(CMAKE_GENERATOR_PLATFORM MATCHES "^[Aa][Rr][Mm]$")
    set(DIRECTX_ARCH arm)
elseif(CMAKE_GENERATOR_PLATFORM MATCHES "^[Aa][Rr][Mm]64$")
    set(DIRECTX_ARCH arm64)
endif()

include(GNUInstallDirs)

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

if(MINGW)
   set(BUILD_XAUDIO_WIN10 OFF)
endif()

if(WINDOWS_STORE OR BUILD_XAUDIO_WIN10 OR BUILD_XAUDIO_REDIST)
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
else()
    file(TO_CMAKE_PATH ${COMPILED_SHADERS} COMPILED_SHADERS)
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
    if (BUILD_DXIL_SHADERS AND VCPKG_TOOLCHAIN)
        message("INFO: Using VCPKG for DirectXShaderCompiler (${VCPKG_HOST_TRIPLET}).")
        find_program(DIRECTX_DXC_TOOL DXC.EXE
          REQUIRED NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH
          HINTS ${DIRECTX_DXC_PATH})
    endif()
    add_custom_command(
        OUTPUT "${COMPILED_SHADERS}/SpriteEffect_SpriteVertexShader.inc"
        MAIN_DEPENDENCY "${PROJECT_SOURCE_DIR}/Src/Shaders/CompileShaders.cmd"
        DEPENDS ${SHADER_SOURCES}
        COMMENT "Generating HLSL shaders..."
        COMMAND ${CMAKE_COMMAND} -E env CompileShadersOutput="${COMPILED_SHADERS}" $<$<BOOL:${DIRECTX_DXC_TOOL}>:DirectXShaderCompiler=${DIRECTX_DXC_TOOL}> CompileShaders.cmd ARGS ${ShaderOpts} > "${COMPILED_SHADERS}/compileshaders.log"
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/Src/Shaders"
        USES_TERMINAL)
endif()

add_library(${PROJECT_NAME} STATIC ${LIBRARY_SOURCES} ${LIBRARY_HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE ${COMPILED_SHADERS} Src)

if ((${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.16") AND (NOT MINGW))
    target_precompile_headers(${PROJECT_NAME} PRIVATE Src/pch.h)
endif()

source_group(Audio REGULAR_EXPRESSION Audio/*.*)
source_group(Inc REGULAR_EXPRESSION Inc/*.*)
source_group(Src REGULAR_EXPRESSION Src/*.*)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Inc>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/directxtk12>)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_11)

if(WINDOWS_STORE OR BUILD_XAUDIO_WIN10 OR BUILD_XAUDIO_REDIST)
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

if (MINGW OR VCPKG_TOOLCHAIN)
    message("INFO: Using VCPKG for DirectX-Headers and DirectXMath (${VCPKG_TARGET_TRIPLET}).")
    find_package(directx-headers CONFIG REQUIRED)
    find_package(directxmath CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE Microsoft::DirectX-Headers Microsoft::DirectXMath)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USING_DIRECTX_HEADERS)
endif()

if (BUILD_XAUDIO_REDIST AND (NOT BUILD_XAUDIO_WIN10) AND (NOT WINDOWS_STORE))
    message("INFO: Using XAudio2Redist for DirectX Tool Kit for Audio.")
    target_compile_definitions(${PROJECT_NAME} PRIVATE USING_XAUDIO2_REDIST)

    find_package(xaudio2redist CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE Microsoft::XAudio2Redist)
endif()

#--- Package
include(CMakePackageConfigHelpers)

string(TOLOWER ${PROJECT_NAME} PACKAGE_NAME)

write_basic_package_version_file(
  ${PACKAGE_NAME}-config-version.cmake
  VERSION ${DIRECTXTK12_VERSION}
  COMPATIBILITY AnyNewerVersion)

install(TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}-targets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/.nuget/${PROJECT_NAME}-config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}-config.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PACKAGE_NAME})

install(EXPORT ${PROJECT_NAME}-targets
  FILE ${PROJECT_NAME}-targets.cmake
  NAMESPACE Microsoft::
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PACKAGE_NAME})

install(FILES ${LIBRARY_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/directxtk12)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}-config-version.cmake
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PACKAGE_NAME})

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /fp:fast "$<$<NOT:$<CONFIG:DEBUG>>:/guard:cf>")
    target_link_options(${PROJECT_NAME} PRIVATE /DYNAMICBASE /NXCOMPAT)

    if((${CMAKE_SIZEOF_VOID_P} EQUAL 4) AND (NOT ${DIRECTX_ARCH} MATCHES "^arm"))
        target_link_options(${PROJECT_NAME} PRIVATE /SAFESEH)
    endif()
endif()

if(NOT ${DIRECTX_ARCH} MATCHES "^arm")
    if (${CMAKE_SIZEOF_VOID_P} EQUAL "4")
        set(ARCH_SSE2 $<$<CXX_COMPILER_ID:MSVC>:/arch:SSE2> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-msse2>)
    else()
        set(ARCH_SSE2 $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-msse2>)
    endif()

    target_compile_options(${PROJECT_NAME} PRIVATE ${ARCH_SSE2})
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wpedantic -Wextra)
endif()
if(MINGW)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wno-ignored-attributes)
endif()
if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(${PROJECT_NAME} PRIVATE /sdl /permissive- /JMC- /Zc:__cplusplus)

    if(ENABLE_CODE_ANALYSIS)
        target_compile_options(${PROJECT_NAME} PRIVATE /analyze)
    endif()

    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.24)
        target_compile_options(${PROJECT_NAME} PRIVATE /ZH:SHA_256)
    endif()

    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.26)
        target_compile_options(${PROJECT_NAME} PRIVATE /Zc:preprocessor /wd5104 /wd5105)
    endif()

    if ((CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.27) AND (NOT (${DIRECTX_ARCH} MATCHES "^arm")))
        target_link_options(${PROJECT_NAME} PRIVATE /CETCOMPAT)
    endif()
endif()

if(WIN32)
    # Windows 10 is used here
    target_compile_definitions(${PROJECT_NAME} PRIVATE _UNICODE UNICODE _WIN32_WINNT=0x0A00)

    if(WINDOWS_STORE)
      target_compile_definitions(${PROJECT_NAME} PRIVATE WINAPI_FAMILY=WINAPI_FAMILY_APP)
    endif()

    if(WINDOWS_STORE OR BUILD_XAUDIO_WIN10)
        message("INFO: Using DirectX Tool Kit for Audio on XAudio 2.9 (Windows 10/Windows 11).")
    endif()
endif()

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
