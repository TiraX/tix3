@echo off
setlocal
set errorcount=0

call :build Win32 Debug "%~dp0..\ispc_texcomp\ispc_texcomp.vcxproj"
call :build x64   Debug "%~dp0..\ispc_texcomp\ispc_texcomp.vcxproj"
call :build Win32 Release "%~dp0..\ispc_texcomp\ispc_texcomp.vcxproj"
call :build x64   Release "%~dp0..\ispc_texcomp\ispc_texcomp.vcxproj"

call :build Win32 Debug "%~dp0test_astc\test_astc.sln"
call :build x64   Debug "%~dp0test_astc\test_astc.sln"
call :build Win32 Release "%~dp0test_astc\test_astc.sln"
call :build x64   Release "%~dp0test_astc\test_astc.sln"

call :build x86 Debug "%~dp0..\ISPC Texture Compressor\ISPC Texture Compressor.sln"
call :build x64 Debug "%~dp0..\ISPC Texture Compressor\ISPC Texture Compressor.sln"
call :build x86 Release "%~dp0..\ISPC Texture Compressor\ISPC Texture Compressor.sln"
call :build x64 Release "%~dp0..\ISPC Texture Compressor\ISPC Texture Compressor.sln"

echo.
if "%errorcount%"=="0" (echo PASS) else (echo %errorcount% FAILED)
exit /b %errorcount%

:build
    echo.
    echo -------------------------------------------------------------------------------------------------------
    echo msbuild /nologo /verbosity:minimal /p:Platform=%1 /p:Configuration=%2 %3
    msbuild /nologo /verbosity:minimal /p:Platform=%1 /p:Configuration=%2 %3
    if not "%errorlevel%"=="0" set /a errorcount=%errorcount%+1
    exit /b 0
