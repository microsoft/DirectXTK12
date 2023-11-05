# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required (VERSION 3.20)

set(DIRECTXTK12_VERSION 1.5.6)

if(WINDOWS_STORE OR (DEFINED XBOX_CONSOLE_TARGET))
  set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
endif()

project (DirectXTK12
  VERSION ${DIRECTXTK12_VERSION}
  DESCRIPTION "DirectX Tool Kit for DirectX 12"
  HOMEPAGE_URL "https://go.microsoft.com/fwlink/?LinkID=615561"
  LANGUAGES CXX)

option(BUILD_XAUDIO_WIN10 "Build for XAudio 2.9" ON)
option(BUILD_XAUDIO_REDIST "Build for XAudio2Redist" OFF)

option(BUILD_GAMEINPUT "Build for GameInput" OFF)
option(BUILD_WGI "Build for Windows.Gaming.Input" OFF)
option(BUILD_XINPUT "Build for XInput" OFF)

# https://devblogs.microsoft.com/cppblog/spectre-mitigations-in-msvc/
option(ENABLE_SPECTRE_MITIGATION "Build using /Qspectre for MSVC" OFF)

option(DISABLE_MSVC_ITERATOR_DEBUGGING "Disable iterator debugging in Debug configurations with the MSVC CRT" OFF)

option(ENABLE_CODE_ANALYSIS "Use Static Code Analysis on build" OFF)

option(USE_PREBUILT_SHADERS "Use externally built HLSL shaders" OFF)

option(BUILD_DXIL_SHADERS "Use DXC Shader Model 6 for shaders" ON)

option(NO_WCHAR_T "Use legacy wide-character as unsigned short" OFF)

option(BUILD_FUZZING "Build for fuzz testing" OFF)

option(BUILD_MIXED_DX11 "Support linking with DX11 version of toolkit" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/CMake")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/CMake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/CMake")

if(DEFINED VCPKG_TARGET_ARCHITECTURE)
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

if(XBOX_CONSOLE_TARGET STREQUAL "scarlett")
  set(BUILD_GAMEINPUT ON)
  set(BUILD_SCARLETT_SHADERS ON)
  set(BUILD_XAUDIO_REDIST OFF)
elseif(XBOX_CONSOLE_TARGET STREQUAL "xboxone")
  set(BUILD_GAMEINPUT ON)
  set(BUILD_XBOXONE_SHADERS ON)
  set(BUILD_XAUDIO_REDIST OFF)
elseif(XBOX_CONSOLE_TARGET STREQUAL "durango")
  set(BUILD_GAMEINPUT OFF)
  set(BUILD_WGI OFF)
  set(BUILD_XINPUT OFF)
  set(BUILD_XBOXONE_SHADERS ON)
  set(BUILD_DXIL_SHADERS OFF)
  set(BUILD_XAUDIO_WIN10 OFF)
  set(BUILD_XAUDIO_WIN8 ON)
elseif(WINDOWS_STORE)
  set(BUILD_GAMEINPUT OFF)
  set(BUILD_WGI ON)
  set(BUILD_XAUDIO_REDIST OFF)
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
    Inc/GeometricPrimitive.h
    Inc/GraphicsMemory.h
    Inc/Model.h
    Inc/PostProcess.h
    Inc/PrimitiveBatch.h
    Inc/RenderTargetState.h
    Inc/ResourceUploadBatch.h
    Inc/ScreenGrab.h
    Inc/SpriteBatch.h
    Inc/SpriteFont.h
    Inc/VertexTypes.h
    Inc/WICTextureLoader.h)

set(LIBRARY_SOURCES
    Src/AlphaTestEffect.cpp
    Src/BasicEffect.cpp
    Src/BasicPostProcess.cpp
    Src/BufferHelpers.cpp
    Src/CommonStates.cpp
    Src/d3dx12.h
    Src/DDSTextureLoader.cpp
    Src/DebugEffect.cpp
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
    Src/GeometricPrimitive.cpp
    Src/GraphicsMemory.cpp
    Src/LinearAllocator.cpp
    Src/LinearAllocator.h
    Src/Model.cpp
    Src/ModelLoadCMO.cpp
    Src/ModelLoadSDKMESH.cpp
    Src/ModelLoadVBO.cpp
    Src/NormalMapEffect.cpp
    Src/PBREffect.cpp
    Src/PBREffectFactory.cpp
    Src/pch.h
    Src/PrimitiveBatch.cpp
    Src/ResourceUploadBatch.cpp
    Src/ScreenGrab.cpp
    Src/SkinnedEffect.cpp
    Src/SpriteBatch.cpp
    Src/SpriteFont.cpp
    Src/ToneMapPostProcess.cpp
    Src/VertexTypes.cpp
    Src/WICTextureLoader.cpp)

set(SHADER_SOURCES
    Src/Shaders/AlphaTestEffect.fx
    Src/Shaders/BasicEffect.fx
    Src/Shaders/DebugEffect.fx
    Src/Shaders/DualTextureEffect.fx
    Src/Shaders/EnvironmentMapEffect.fx
    Src/Shaders/GenerateMips.hlsl
    Src/Shaders/NormalMapEffect.fx
    Src/Shaders/PBREffect.fx
    Src/Shaders/PostProcess.fx
    Src/Shaders/RootSig.fxh
    Src/Shaders/SkinnedEffect.fx
    Src/Shaders/SpriteEffect.fx
    Src/Shaders/ToneMap.fx)

# These source files are identical in both DX11 and DX12 version.
if(NOT BUILD_MIXED_DX11)
    set(LIBRARY_HEADERS ${LIBRARY_HEADERS}
      Inc/GamePad.h
      Inc/Keyboard.h
      Inc/Mouse.h
      Inc/SimpleMath.h
      Inc/SimpleMath.inl)

    set(LIBRARY_SOURCES ${LIBRARY_SOURCES}
      Src/BinaryReader.cpp
      Src/GamePad.cpp
      Src/Geometry.cpp
      Src/Keyboard.cpp
      Src/Mouse.cpp
      Src/SimpleMath.cpp)
endif()

set(LIBRARY_SOURCES ${LIBRARY_SOURCES}
    Src/AlignedNew.h
    Src/Bezier.h
    Src/BinaryReader.h
    Src/DDS.h
    Src/DemandCreate.h
    Src/Geometry.h
    Src/LoaderHelpers.h
    Src/PlatformHelpers.h
    Src/SDKMesh.h
    Src/SharedResourcePool.h
    Src/vbo.h
    Src/TeapotData.inc)

set(SHADER_SOURCES ${SHADER_SOURCES}
    Src/Shaders/Common.fxh
    Src/Shaders/Lighting.fxh
    Src/Shaders/PBRCommon.fxh
    Src/Shaders/PixelPacking_Velocity.hlsli
    Src/Shaders/Skinning.fxh
    Src/Shaders/Structures.fxh
    Src/Shaders/Utilities.fxh)

if(MINGW)
   set(BUILD_XAUDIO_WIN10 OFF)
endif()

if(WINDOWS_STORE OR BUILD_XAUDIO_WIN10 OR BUILD_XAUDIO_WIN8 OR BUILD_XAUDIO_REDIST)
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

if(NOT COMPILED_SHADERS)
    if(USE_PREBUILT_SHADERS)
        message(FATAL_ERROR "ERROR: Using prebuilt shaders requires the COMPILED_SHADERS variable is set.")
    endif()
    set(COMPILED_SHADERS ${CMAKE_CURRENT_BINARY_DIR}/Shaders/Compiled)
    file(MAKE_DIRECTORY ${COMPILED_SHADERS})
else()
    file(TO_CMAKE_PATH ${COMPILED_SHADERS} COMPILED_SHADERS)
endif()

set(LIBRARY_SOURCES ${LIBRARY_SOURCES}
    ${COMPILED_SHADERS}/SpriteEffect_SpriteVertexShader.inc)

if(BUILD_SCARLETT_SHADERS)
    message(STATUS "Using Shader Model 6 for Xbox Series X|S for shaders")
    set(ShaderOpts gxdk scarlett)
elseif(BUILD_XBOXONE_SHADERS)
    if(BUILD_DXIL_SHADERS)
        message(STATUS "Using Shader Model 6 for Xbox One for shaders")
        set(ShaderOpts gxdk)
    else()
        message(STATUS "Using Shader Model 5.1 for Xbox One for shaders")
        set(ShaderOpts xbox)
    endif()
elseif(BUILD_DXIL_SHADERS)
    message(STATUS "Using Shader Model 6.0 (DXC.EXE) for shaders.")
    set(ShaderOpts dxil)
else()
    message(STATUS "Using Shader Model 5.1 (FXC.EXE) for shaders.")
    set(ShaderOpts "")
endif()

if(NOT USE_PREBUILT_SHADERS)
    if(BUILD_DXIL_SHADERS AND VCPKG_TOOLCHAIN AND (NOT BUILD_SCARLETT_SHADERS) AND (NOT BUILD_XBOXONE_SHADERS))
        message(STATUS "Using VCPKG for DirectXShaderCompiler (${VCPKG_HOST_TRIPLET}).")
        find_program(DIRECTX_DXC_TOOL DXC.EXE
          REQUIRED NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH NO_DEFAULT_PATH
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

if(NOT MINGW)
    target_precompile_headers(${PROJECT_NAME} PRIVATE Src/pch.h)
endif()

source_group(Audio REGULAR_EXPRESSION Audio/*.*)
source_group(Inc REGULAR_EXPRESSION Inc/*.*)
source_group(Src REGULAR_EXPRESSION Src/*.*)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Inc>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/directxtk12>)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_11)

if(WINDOWS_STORE OR BUILD_XAUDIO_WIN10 OR BUILD_XAUDIO_WIN8 OR BUILD_XAUDIO_REDIST)
    target_include_directories(${PROJECT_NAME} PRIVATE Audio)
endif()

if(MINGW)
    find_package(directxmath CONFIG REQUIRED)
    find_package(directx-headers CONFIG REQUIRED)
else()
    find_package(directxmath CONFIG QUIET)
    find_package(directx-headers CONFIG QUIET)
endif()

if(directxmath_FOUND)
    message(STATUS "Using DirectXMath package")
    target_link_libraries(${PROJECT_NAME} PUBLIC Microsoft::DirectXMath)
endif()

if(directx-headers_FOUND)
    message(STATUS "Using DirectX-Headers package")
    target_link_libraries(${PROJECT_NAME} PUBLIC Microsoft::DirectX-Headers)
    target_compile_definitions(${PROJECT_NAME} PUBLIC USING_DIRECTX_HEADERS)
endif()

if(BUILD_XAUDIO_REDIST AND (NOT BUILD_XAUDIO_WIN10) AND (NOT BUILD_XAUDIO_WIN8) AND (NOT WINDOWS_STORE))
    message(STATUS "Using XAudio2Redist for DirectX Tool Kit for Audio.")
    find_package(xaudio2redist CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC Microsoft::XAudio2Redist)
    target_compile_definitions(${PROJECT_NAME} PUBLIC USING_XAUDIO2_REDIST)
endif()

include(CheckIncludeFileCXX)

if(BUILD_GAMEINPUT)
    message(STATUS "Using GameInput for GamePad/Keyboard/Mouse.")
    set(CMAKE_REQUIRED_QUIET ON)
    CHECK_INCLUDE_FILE_CXX(GameInput.h GAMEINPUT_HEADER)
    if(NOT GAMEINPUT_HEADER)
        message(FATAL_ERROR "Microsoft GDK required to build GameInput. See https://aka.ms/gdk")
    endif()
    target_compile_definitions(${PROJECT_NAME} PUBLIC USING_GAMEINPUT)
elseif(BUILD_WGI)
    message(STATUS "Using Windows.Gaming.Input for GamePad.")
    target_compile_definitions(${PROJECT_NAME} PUBLIC USING_WINDOWS_GAMING_INPUT)
elseif(BUILD_XINPUT)
    message(STATUS "Using XInput for GamePad.")
    target_compile_definitions(${PROJECT_NAME} PUBLIC USING_XINPUT)
endif()

if(DEFINED XBOX_CONSOLE_TARGET)
    message(STATUS "Building for Xbox Console Target: ${XBOX_CONSOLE_TARGET}")
    set(CMAKE_REQUIRED_QUIET ON)
    if(XBOX_CONSOLE_TARGET STREQUAL "durango")
        CHECK_INCLUDE_FILE_CXX(xdk.h XDKLegacy_HEADER)
        if(NOT XDKLegacy_HEADER)
            message(FATAL_ERROR "Legacy Xbox One XDK required to build for Durango.")
        endif()
        target_compile_definitions(${PROJECT_NAME} PUBLIC WINAPI_FAMILY=WINAPI_FAMILY_TV_TITLE _XBOX_ONE _TITLE MONOLITHIC=1)
    else()
        CHECK_INCLUDE_FILE_CXX(gxdk.h GXDK_HEADER)
        if(NOT GXDK_HEADER)
            message(FATAL_ERROR "Microsoft GDK with Xbox Extensions required to build for Xbox. See https://aka.ms/gdkx")
        endif()
        target_compile_definitions(${PROJECT_NAME} PUBLIC WINAPI_FAMILY=WINAPI_FAMILY_GAMES)
        if(XBOX_CONSOLE_TARGET STREQUAL "scarlett")
            CHECK_INCLUDE_FILE_CXX(d3d12_xs.h D3D12XS_HEADER)
            if(NOT D3D12XS_HEADER)
                message(FATAL_ERROR "Microsoft GDK with Xbox Extensions environment needs to be set for Xbox Series X|S.")
            endif()
            target_compile_definitions(${PROJECT_NAME} PUBLIC _GAMING_XBOX _GAMING_XBOX_SCARLETT)
        elseif(XBOX_CONSOLE_TARGET STREQUAL "xboxone")
            CHECK_INCLUDE_FILE_CXX(d3d12_x.h D3D12X_HEADER)
            if(NOT D3D12X_HEADER)
                message(FATAL_ERROR "Microsoft GDK with Xbox Extensions environment needs to be set for Xbox One.")
            endif()
            target_compile_definitions(${PROJECT_NAME} PUBLIC _GAMING_XBOX _GAMING_XBOX_XBOXONE)
        else()
            message(FATAL_ERROR "Unknown XBOX_CONSOLE_TARGET")
        endif()
    endif()
elseif(WINDOWS_STORE)
    target_compile_definitions(${PROJECT_NAME} PUBLIC WINAPI_FAMILY=WINAPI_FAMILY_APP)
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

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/build/${PROJECT_NAME}-config.cmake.in
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

# Model uses dynamic_cast, so we need /GR (Enable RTTI)
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /Wall /GR /fp:fast "$<$<NOT:$<CONFIG:DEBUG>>:/guard:cf>")
    target_link_options(${PROJECT_NAME} PRIVATE /DYNAMICBASE /NXCOMPAT /INCREMENTAL:NO)

    if((CMAKE_SIZEOF_VOID_P EQUAL 4) AND (NOT ${DIRECTX_ARCH} MATCHES "^arm"))
      target_link_options(${PROJECT_NAME} PRIVATE /SAFESEH)
    endif()

    if(ENABLE_SPECTRE_MITIGATION
       AND (MSVC_VERSION GREATER_EQUAL 1913)
       AND (NOT WINDOWS_STORE)
       AND (NOT (CMAKE_CXX_COMPILER_ID MATCHES "Clang")))
      message(STATUS "Building Spectre-mitigated libraries.")
      target_compile_options(${PROJECT_NAME} PRIVATE "/Qspectre")
    endif()

    if((MSVC_VERSION GREATER_EQUAL 1924)
       AND ((NOT (CMAKE_CXX_COMPILER_ID MATCHES "Clang")) OR (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16.0)))
      target_compile_options(${PROJECT_NAME} PRIVATE /ZH:SHA_256)
    endif()

    if((MSVC_VERSION GREATER_EQUAL 1928)
       AND (CMAKE_SIZEOF_VOID_P EQUAL 8)
       AND ((NOT (CMAKE_CXX_COMPILER_ID MATCHES "Clang")) OR (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13.0)))
      target_compile_options(${PROJECT_NAME} PRIVATE "$<$<NOT:$<CONFIG:DEBUG>>:/guard:ehcont>")
      target_link_options(${PROJECT_NAME} PRIVATE "$<$<NOT:$<CONFIG:DEBUG>>:/guard:ehcont>")
    endif()

    if(NO_WCHAR_T)
      message(STATUS "Using non-native wchar_t as unsigned short")
      target_compile_options(${PROJECT_NAME} PRIVATE "/Zc:wchar_t-")
    endif()
else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE $<IF:$<CONFIG:DEBUG>,_DEBUG,NDEBUG>)
endif()

if(XBOX_CONSOLE_TARGET STREQUAL "scarlett")
    target_compile_options(${PROJECT_NAME} PRIVATE $<IF:$<CXX_COMPILER_ID:MSVC>,/favor:AMD64 /arch:AVX2,-march=znver2>)
elseif(XBOX_CONSOLE_TARGET MATCHES "(xboxone|durango)")
    target_compile_options(${PROJECT_NAME} PRIVATE $<IF:$<CXX_COMPILER_ID:MSVC>,/favor:AMD64 /arch:AVX,-march=btver2>)
elseif(NOT (${DIRECTX_ARCH} MATCHES "^arm"))
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(ARCH_SSE2 $<$<CXX_COMPILER_ID:MSVC>:/arch:SSE2> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-msse2>)
    else()
        set(ARCH_SSE2 $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-msse2>)
    endif()

    target_compile_options(${PROJECT_NAME} PRIVATE ${ARCH_SSE2})
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wpedantic -Wextra)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16.0)
        target_compile_options(${PROJECT_NAME} PRIVATE "-Wno-unsafe-buffer-usage")
    endif()
elseif(MINGW)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wno-ignored-attributes)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(${PROJECT_NAME} PRIVATE /sdl /permissive- /JMC- /Zc:__cplusplus /Zc:inline)

    if(ENABLE_CODE_ANALYSIS)
        target_compile_options(${PROJECT_NAME} PRIVATE /analyze)
    endif()

    if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
      message(STATUS "Building using Whole Program Optimization")
      target_compile_options(${PROJECT_NAME} PRIVATE /Gy /Gw)
    endif()

    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.26)
        target_compile_options(${PROJECT_NAME} PRIVATE /Zc:preprocessor /wd5104 /wd5105)
    endif()

    if((CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.27) AND (NOT (${DIRECTX_ARCH} MATCHES "^arm")))
        target_link_options(${PROJECT_NAME} PRIVATE /CETCOMPAT)
    endif()

    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.28)
        target_compile_options(${PROJECT_NAME} PRIVATE /Zc:lambda)
    endif()

    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.34)
        target_compile_options(${PROJECT_NAME} PRIVATE /wd5262)
    endif()

    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.35)
        if(NOT (DEFINED XBOX_CONSOLE_TARGET))
          target_compile_options(${PROJECT_NAME} PRIVATE /Zc:checkGwOdr $<$<VERSION_GREATER_EQUAL:${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION},10.0.22000>:/Zc:templateScope>)
        endif()
    endif()

    if (BUILD_FUZZING AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.32)
        target_compile_options(${PROJECT_NAME} PRIVATE /fsanitize=address /fsanitize-coverage=inline-8bit-counters /fsanitize-coverage=edge /fsanitize-coverage=trace-cmp /fsanitize-coverage=trace-div)
        target_link_libraries(${PROJECT_NAME} PRIVATE sancov.lib)
    endif()
endif()

if(WIN32)
    if (XBOX_CONSOLE_TARGET STREQUAL "durango")
        set(WINVER 0x0602)
    else()
        set(WINVER 0x0A00)
    endif()

    target_compile_definitions(${PROJECT_NAME} PRIVATE _UNICODE UNICODE _WIN32_WINNT=${WINVER})

    if(WINDOWS_STORE OR BUILD_XAUDIO_WIN10)
      message(STATUS "Using DirectX Tool Kit for Audio on XAudio 2.9 (Windows 10/Windows 11).")
    endif()

    if(DISABLE_MSVC_ITERATOR_DEBUGGING)
      target_compile_definitions(${PROJECT_NAME} PRIVATE _ITERATOR_DEBUG_LEVEL=0)
    endif()
endif()

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

#--- Test suite
include(CTest)
if(BUILD_TESTING AND WIN32 AND (NOT WINDOWS_STORE) AND (NOT (DEFINED XBOX_CONSOLE_TARGET))
   AND (EXISTS "${CMAKE_CURRENT_LIST_DIR}/Tests/CMakeLists.txt"))
  enable_testing()
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/Tests)
elseif(BUILD_FUZZING AND WIN32 AND (NOT WINDOWS_STORE) AND (NOT (DEFINED XBOX_CONSOLE_TARGET))
   AND (EXISTS "${CMAKE_CURRENT_LIST_DIR}/Tests/fuzzloaders/CMakeLists.txt"))
  message(STATUS "Building for fuzzing")
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/Tests/fuzzloaders)
endif()
