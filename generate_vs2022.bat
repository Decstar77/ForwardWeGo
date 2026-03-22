@echo off

where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo ERROR: CMake is not installed or not in your PATH.
    echo.
    echo Install it from https://cmake.org/download/
    echo Or via winget:  winget install Kitware.CMake
    echo.
    echo Make sure to select "Add CMake to the system PATH" during installation.
    pause
    exit /b 1
)

echo Generating Visual Studio 2022 solution...
cmake -B build -G "Visual Studio 17 2022" -A x64
if %errorlevel% neq 0 (
    echo.
    echo ERROR: CMake generation failed.
    pause
    exit /b 1
)

echo.
echo Solution generated: build\atto.sln
pause
