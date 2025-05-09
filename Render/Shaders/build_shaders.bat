@echo off
rem -----------------------------------------------------------------------------
rem build_shaders.bat
rem 在 Shaders 目录下运行，自动编译 VS/PS 两个入口点
rem -----------------------------------------------------------------------------

setlocal

rem 脚本所在目录（Shaders\）
set SHADER_DIR=%~dp0

rem 输出目录：Shaders\Compiled\
set OUTDIR=%SHADER_DIR%Compiled
if not exist "%OUTDIR%" (
    mkdir "%OUTDIR%"
)

rem FXC 可执行文件，假设已加入 PATH
set FXC=fxc.exe

echo ================================================
echo Building shaders to "%OUTDIR%"
echo ================================================

rem -------------------------------
rem BasePassDefault.hlsl
rem -------------------------------
echo [BasePassDefault] VS...
"%FXC%" /T vs_5_0 /E VS   "%SHADER_DIR%BasePassDefault.hlsl" /Fo "%OUTDIR%\BasePassDefaultVS.cso"
if errorlevel 1 goto :error

echo [BasePassDefault] PS...
"%FXC%" /T ps_5_0 /E PS   "%SHADER_DIR%BasePassDefault.hlsl" /Fo "%OUTDIR%\BasePassDefaultPS.cso"
if errorlevel 1 goto :error

rem -------------------------------
rem Primitive.hlsl
rem -------------------------------
echo [Primitive] VS...
"%FXC%" /T vs_5_0 /E VS   "%SHADER_DIR%Primitive.hlsl" /Fo "%OUTDIR%\PrimitiveVS.cso"
if errorlevel 1 goto :error

echo [Primitive] PS...
"%FXC%" /T ps_5_0 /E PS   "%SHADER_DIR%Primitive.hlsl" /Fo "%OUTDIR%\PrimitivePS.cso"
if errorlevel 1 goto :error

rem -------------------------------
rem DefferedLightng.hlsl
rem -------------------------------
echo [DefferedLightng] VS...
"%FXC%" /T vs_5_0 /E VS   "%SHADER_DIR%DeferredLighting.hlsl" /Fo "%OUTDIR%\DeferredLightingVS.cso"
if errorlevel 1 goto :error

echo [DefferedLightng] PS...
"%FXC%" /T ps_5_0 /E PS   "%SHADER_DIR%DeferredLighting.hlsl" /Fo "%OUTDIR%\DeferredLightingPS.cso"
if errorlevel 1 goto :error

rem -------------------------------
rem PostProcess.hlsl
rem -------------------------------
echo [PostProcess] VS...
"%FXC%" /T vs_5_0 /E VS   "%SHADER_DIR%PostProcess.hlsl" /Fo "%OUTDIR%\PostProcess.cso"
if errorlevel 1 goto :error

echo [PostProcess] PS...
"%FXC%" /T ps_5_0 /E PS   "%SHADER_DIR%PostProcess.hlsl" /Fo "%OUTDIR%\PostProcess.cso"
if errorlevel 1 goto :error

echo.
echo All shaders compiled successfully.
goto :eof

:error
echo.
echo !!! Shader compilation failed. See error messages above.
exit /b 1
