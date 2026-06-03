@set VCPKG_BINARY_SOURCES=clear
@set VCPKG_ROOT=%cd%
@if %1.==xbox. goto xbox
@if %1.==clang. goto clang
.\vcpkg install directxtk12[core]:x86-windows
@if errorlevel 1 goto error
.\vcpkg install directxtk12[xaudio2-9]:x86-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk12[xaudio2redist]:x86-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk12[xinput]:x86-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk12[spectre]:x86-windows-static-md --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk12:x86-windows-static
@if errorlevel 1 goto error
.\vcpkg install directxtk12:x86-windows-static-md
@if errorlevel 1 goto error
.\vcpkg install directxtk12[xaudio2redist]:x86-windows-static-md --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk12[core]:x64-windows
@if errorlevel 1 goto error
.\vcpkg install directxtk12[tools]:x64-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk12[xaudio2-9]:x64-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk12[xaudio2redist]:x64-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk12[xinput]:x64-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk12[gameinput]:x64-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk12[spectre]:x64-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk12:x64-windows-static
@if errorlevel 1 goto error
.\vcpkg install directxtk12:x64-windows-static-md
@if errorlevel 1 goto error
.\vcpkg install directxtk12[gameinput]:x64-windows-static-md --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk12[xaudio2redist]:x64-windows-static-md --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk12[core]:arm64-windows
@if errorlevel 1 goto error
.\vcpkg install directxtk12[tools]:arm64-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk12[xinput]:arm64-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk12[xaudio2redist]:arm64-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk12[spectre]:arm64-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk12:arm64-windows-static
@if errorlevel 1 goto error
.\vcpkg install directxtk12:arm64-windows-static-md
@if errorlevel 1 goto error
.\vcpkg install directxtk12[xaudio2redist]:arm64-windows-static-md --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk12[core]:arm64ec-windows
@if errorlevel 1 goto error
.\vcpkg install directxtk12[tools]:arm64ec-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk12[xaudio2redist]:arm64ec-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk12[spectre]:arm64ec-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk12:x86-uwp
@if errorlevel 1 goto error
.\vcpkg install directxtk12:x64-uwp
@if errorlevel 1 goto error
.\vcpkg install directxtk12:arm64-uwp
@if errorlevel 1 goto error
@where /Q x86_64-w64-mingw32-g++.exe
@if errorlevel 1 goto skipgcc64
.\vcpkg install directxtk12:x64-mingw-dynamic
@if errorlevel 1 goto error
.\vcpkg install directxtk12[xaudio2redist]:x64-mingw-dynamic --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk12[gameinput]:x64-mingw-dynamic --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk12[tools]:x64-mingw-dynamic --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk12:x64-mingw-static
@if errorlevel 1 goto error
.\vcpkg install directxtk12[xaudio2redist]:x64-mingw-static --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk12[xinput]:x64-mingw-static --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk12[gameinput]:x64-mingw-static --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk12[tools]:x64-mingw-static --recurse
@if errorlevel 1 goto error
:skipgcc64
@where /Q i686-w64-mingw32-g++.exe
@if errorlevel 1 goto skipgcc32
.\vcpkg install directxtk12:x86-mingw-dynamic
@if errorlevel 1 goto error
.\vcpkg install directxtk12[xaudio2redist]:x86-mingw-dynamic --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk12[xinput]:x86-mingw-dynamic --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk12:x86-mingw-static
@if errorlevel 1 goto error
.\vcpkg install directxtk12[xaudio2redist]:x86-mingw-static --recurse
@if errorlevel 1 goto error
:skipgcc32
goto finish
:xbox
.\vcpkg install directxtk12:x64-xbox-scarlett
@if errorlevel 1 goto error
.\vcpkg install directxtk12:x64-xbox-scarlett-static
@if errorlevel 1 goto error
.\vcpkg install directxtk12:x64-xbox-xboxone
@if errorlevel 1 goto error
.\vcpkg install directxtk12:x64-xbox-xboxone-static
@if errorlevel 1 goto error
@goto finish
:clang
.\vcpkg install directxtk12[core]:x64-clangcl-dynamic
@if errorlevel 1 goto error
.\vcpkg install directxtk12[tools]:x64-clangcl-dynamic --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk12[xaudio2-9]:x64-clangcl-dynamic --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk12[xaudio2redist]:x64-clangcl-dynamic --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk12:x64-clangcl-static
@if errorlevel 1 goto error
.\vcpkg install directxtk12[core]:arm64-clangcl-dynamic
@if errorlevel 1 goto error
.\vcpkg install directxtk12[tools]:arm64-clangcl-dynamic --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk12[xaudio2redist]:arm64-clangcl-dynamic --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk12:arm64-clangcl-static
@if errorlevel 1 goto error
.\vcpkg install directxtk12:x64-clangcl-uwp
@if errorlevel 1 goto error
.\vcpkg install directxtk12:arm64-clangcl-uwp
@if errorlevel 1 goto error
@if "%GXDKLatest%."=="." goto finish
.\vcpkg install directxtk12:x64-clangcl-scarlett
@if errorlevel 1 goto error
.\vcpkg install directxtk12:x64-clangcl-scarlett-static
@if errorlevel 1 goto error
.\vcpkg install directxtk12:x64-clangcl-xboxone
@if errorlevel 1 goto error
.\vcpkg install directxtk12:x64-clangcl-xboxone-static
@if errorlevel 1 goto error
:finish
@echo SUCCEEDED
@if %1.==xbox. goto eof
@if %1.==clang. goto eof
@where /Q x86_64-w64-mingw32-g++.exe
@if NOT errorlevel 1 goto gcc64
@echo .
@echo . Run for MinGW64
@echo .   .\vcpkg install directxtk12:x64-mingw-dynamic
@echo .   .\vcpkg install directxtk12[xaudio2redist]:x64-mingw-dynamic --recurse
@echo .   .\vcpkg install directxtk12[gameinput]:x64-mingw-dynamic --recurse
@echo .   .\vcpkg install directxtk12[tools]:x64-mingw-dynamic --recurse
@echo .   .\vcpkg install directxtk12:x64-mingw-static
@echo .   .\vcpkg install directxtk12[xaudio2redist]:x64-mingw-static --recurse
@echo .   .\vcpkg install directxtk12[gameinput]:x64-mingw-static --recurse
@echo .   .\vcpkg install directxtk12[tools]:x64-mingw-static --recurse
:gcc64
@where /Q i686-w64-mingw32-g++.exe
@if NOT errorlevel 1 goto gcc32
@echo .
@echo . Run for MinGW32
@echo .   .\vcpkg install directxtk12:x86-mingw-dynamic
@echo .   .\vcpkg install directxtk12[xaudio2redist]:x86-mingw-dynamic --recurse
@echo .   .\vcpkg install directxtk12:x86-mingw-static
@echo .   .\vcpkg install directxtk12[xaudio2redist]:x86-mingw-static --recurse
:gcc32
@goto eof
:error
@echo FAILED
:eof
