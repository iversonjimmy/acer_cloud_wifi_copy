@echo off

:: Use verify to init errorlevel to non-zero.
verify other 2>nul
setlocal enabledelayedexpansion
if errorlevel 1 (
    echo Unable to enable delayed expansion of cmd env variables
    exit 1
)

:: Find the location of devenv.exe
if not defined MSVC_DEVENV_DIR set MSVC_DEVENV_DIR=C:\Program Files (x86)\Microsoft Visual Studio 9.0\Common7\IDE

if exist "%MSVC_DEVENV_DIR%\devenv.exe" goto SDK_CHECK_PASSED
echo *** ERROR: Could not find the Microsoft Visual Studio devenv.exe.
echo ***   I tried looking in "%MSVC_DEVENV_DIR%".  You may need to manually set env variable MSVC_DEVENV_DIR.
exit 1
:SDK_CHECK_PASSED

:: Need to pass BUILDROOT through to Visual Studio in Windows backslash format.
:: (Windows equivalent of "export FIXED_DIR=`cygpath -w $RAW_DIR`"):
for /F %%v IN ('cygpath -w "%BUILDROOT%"') do call set BUILDROOT=%%v
for /F %%v IN ('cygpath -w "%1"') do call set SOLUTION_FILE=%%v

echo %%BUILDROOT%% is "%BUILDROOT%"

:: How to parse parameters 
::for %%a in ( %* ) do (
::    if "%%a"=="-vc6" (
::        set vc_version=-vc6
::    ) else if "%%a"=="-vc7" (
::        set vc_version=
::    ) else if "!args!"=="" (
::        set args=%%a
::    ) else (
::        set args=!args! %%a
::    )
::)

@echo on
"%MSVC_DEVENV_DIR%\devenv" %SOLUTION_FILE% %2 "%3|%4"
@echo ### devenv returned: %errorlevel%
@exit %errorlevel%
@echo off

endlocal
