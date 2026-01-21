@echo off
REM Shader compilation script for NVRHI Triangle Demo
REM Supports both Slang (slangc) and DXC (dxc) compilers
REM
REM Slang: https://github.com/shader-slang/slang/releases
REM DXC: Included with Windows SDK or https://github.com/microsoft/DirectXShaderCompiler/releases

setlocal

echo ============================================
echo NVRHI Triangle Shader Compilation
echo ============================================
echo.

REM Try Slang compiler first
where slangc >nul 2>nul
if %ERRORLEVEL% equ 0 (
    echo Found Slang compiler, using Slang...
    goto compile_slang
)

REM Try DXC compiler as fallback
where dxc >nul 2>nul
if %ERRORLEVEL% equ 0 (
    echo Found DXC compiler, using DXC...
    goto compile_dxc
)

REM Neither compiler found
echo ERROR: No shader compiler found!
echo.
echo Please install one of the following:
echo   1. Slang compiler (slangc): https://github.com/shader-slang/slang/releases
echo   2. DXC compiler (dxc): Included with Windows SDK or
echo      https://github.com/microsoft/DirectXShaderCompiler/releases
echo.
echo Add the compiler to your PATH environment variable.
pause
exit /b 1

:compile_slang
echo.
echo Compiling with Slang (triangle.slang)...
echo.

echo [1/2] Compiling vertex shader...
slangc triangle.slang -profile sm_6_0 -target dxil -entry vsMain -stage vertex -o triangle_vs.dxil
if %ERRORLEVEL% neq 0 (
    echo ERROR: Failed to compile vertex shader
    pause
    exit /b 1
)
echo       OK

echo [2/2] Compiling pixel shader...
slangc triangle.slang -profile sm_6_0 -target dxil -entry psMain -stage fragment -o triangle_ps.dxil
if %ERRORLEVEL% neq 0 (
    echo ERROR: Failed to compile pixel shader
    pause
    exit /b 1
)
echo       OK

goto success

:compile_dxc
echo.
echo Compiling with DXC (triangle.hlsl)...
echo.

echo [1/2] Compiling vertex shader...
dxc -T vs_6_0 -E vsMain triangle.hlsl -Fo triangle_vs.dxil
if %ERRORLEVEL% neq 0 (
    echo ERROR: Failed to compile vertex shader
    pause
    exit /b 1
)
echo       OK

echo [2/2] Compiling pixel shader...
dxc -T ps_6_0 -E psMain triangle.hlsl -Fo triangle_ps.dxil
if %ERRORLEVEL% neq 0 (
    echo ERROR: Failed to compile pixel shader
    pause
    exit /b 1
)
echo       OK

goto success

:success
echo.
echo ============================================
echo Shader compilation successful!
echo ============================================
echo Output files:
echo   - triangle_vs.dxil (vertex shader)
echo   - triangle_ps.dxil (pixel shader)
echo.
pause
