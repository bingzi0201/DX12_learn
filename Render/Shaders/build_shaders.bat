@echo off

setlocal

set SHADER_DIR=%~dp0

set OUTDIR=%SHADER_DIR%Compiled
if not exist "%OUTDIR%" (
    mkdir "%OUTDIR%"
)

set FXC=fxc.exe

echo ================================================
echo Building shaders to "%OUTDIR%"
echo ================================================

rem -------------------------------
rem IBLEnvironment.hlsl
rem -------------------------------
echo [IBLEnvironment] VS...
"%FXC%" /T vs_5_0 /E VS   "%SHADER_DIR%IBLEnvironment.hlsl" /Fo "%OUTDIR%\IBLEnvironmentVS.cso"
if errorlevel 1 goto :error

echo [IBLEnvironment] PS...
"%FXC%" /T ps_5_0 /E PS   "%SHADER_DIR%IBLEnvironment.hlsl" /Fo "%OUTDIR%\IBLEnvironmentPS.cso"
if errorlevel 1 goto :error

rem -------------------------------
rem IBLIrradiance.hlsl
rem -------------------------------
echo [IBLIrradiance] VS...
"%FXC%" /T vs_5_0 /E VS   "%SHADER_DIR%IBLIrradiance.hlsl" /Fo "%OUTDIR%\IBLIrradianceVS.cso"
if errorlevel 1 goto :error

echo [IBLIrradiance] PS...
"%FXC%" /T ps_5_0 /E PS   "%SHADER_DIR%IBLIrradiance.hlsl" /Fo "%OUTDIR%\IBLIrradiancePS.cso"
if errorlevel 1 goto :error

rem -------------------------------
rem IBLPrefilterEnv.hlsl
rem -------------------------------
echo [IBLPrefilterEnv] VS...
"%FXC%" /T vs_5_0 /E VS   "%SHADER_DIR%IBLPrefilterEnv.hlsl" /Fo "%OUTDIR%\IBLPrefilterEnvVS.cso"
if errorlevel 1 goto :error

echo [IBLPrefilterEnv] PS...
"%FXC%" /T ps_5_0 /E PS   "%SHADER_DIR%IBLPrefilterEnv.hlsl" /Fo "%OUTDIR%\IBLPrefilterEnvPS.cso"
if errorlevel 1 goto :error

rem -------------------------------
rem BasePassSky.hlsl
rem -------------------------------
echo [BasePassSky] VS...
"%FXC%" /T vs_5_0 /E VS   "%SHADER_DIR%BasePassSky.hlsl" /Fo "%OUTDIR%\BasePassSkyVS.cso"
if errorlevel 1 goto :error

echo [BasePassSky] PS...
"%FXC%" /T ps_5_0 /E PS   "%SHADER_DIR%BasePassSky.hlsl" /Fo "%OUTDIR%\BasePassSkyPS.cso"
if errorlevel 1 goto :error

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
