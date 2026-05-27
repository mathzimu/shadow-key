@echo off
REM ShadowKey - Windows Build Setup Script (VS2022)
REM Usage: run from the project root directory

echo === ShadowKey Build Setup ===

REM Check for CMake
where cmake >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake not found. Install CMake 3.20+ first.
    exit /b 1
)

REM Check for OpenCV
if "%OpenCV_DIR%"=="" (
    if exist "C:\opencv\build" (
        set "OpenCV_DIR=C:\opencv\build"
    ) else (
        echo WARNING: OpenCV_DIR not set. Set it to your OpenCV build directory.
        echo Example: set OpenCV_DIR=C:\tools\opencv\build
    )
)

REM Configure
echo Configuring ShadowKey with CMake...
cmake -B build -G "Visual Studio 17 2022" -A x64
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake configuration failed.
    exit /b 1
)

REM Build
echo Building ShadowKey...
cmake --build build --config Release
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Build failed.
    exit /b 1
)

echo === Build successful! ===
echo Output: build\Release\ShadowKey.exe
