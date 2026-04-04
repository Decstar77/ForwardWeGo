@echo off
REM Build ForwardWeGo for Emscripten (WebAssembly)
REM Requires emsdk to be installed in vendor/emsdk

setlocal

REM Activate emsdk
call "%~dp0vendor\emsdk\emsdk_env.bat"

REM Configure with emcmake
call emcmake cmake -B build_web -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release

REM Build
cmake --build build_web

echo.
echo Build complete. Output in build_web/bin/
echo Open build_web/bin/atto.html in a browser to run.
endlocal
