@echo off
setlocal EnableExtensions DisableDelayedExpansion
cd /d "%~dp0"

set "PROJ=%CD%"
set "DEPS=%PROJ%\.deps"
set "BUILD=%PROJ%\build-windows-x64"
set "OUT=%PROJ%\dist"
set "STAGE=%BUILD%\stage"
set "VERSION=1.2.0"

for /f "usebackq delims=" %%V in (`powershell -NoProfile -Command "$j = Get-Content -Raw '%PROJ%\mod.json' ^| ConvertFrom-Json; $j.version" 2^>nul`) do set "VERSION=%%V"
set "ARCHIVE=%OUT%\better-aiming-%VERSION%.zip"

set "SDK="
if defined FOXHOLLOW_SOURCE_DIR if exist "%FOXHOLLOW_SOURCE_DIR%\port\include\foxhollow_mod_api.h" set "SDK=%FOXHOLLOW_SOURCE_DIR%"
if defined SDK goto :sdk_found
if exist "%PROJ%\.deps\foxhollow\port\include\foxhollow_mod_api.h" set "SDK=%PROJ%\.deps\foxhollow"
if defined SDK goto :sdk_found
if exist "%PROJ%\foxhollow\port\include\foxhollow_mod_api.h" set "SDK=%PROJ%\foxhollow"
if defined SDK goto :sdk_found
if exist "%PROJ%\foxhollow-main\port\include\foxhollow_mod_api.h" set "SDK=%PROJ%\foxhollow-main"
if defined SDK goto :sdk_found
if exist "%PROJ%\foxhollow-sdk\port\include\foxhollow_mod_api.h" set "SDK=%PROJ%\foxhollow-sdk"
if defined SDK goto :sdk_found
if exist "%PROJ%\..\foxhollow\port\include\foxhollow_mod_api.h" set "SDK=%PROJ%\..\foxhollow"
if defined SDK goto :sdk_found
if exist "%PROJ%\..\foxhollow-main\port\include\foxhollow_mod_api.h" set "SDK=%PROJ%\..\foxhollow-main"
if defined SDK goto :sdk_found
if exist "%PROJ%\..\foxhollow-sdk\port\include\foxhollow_mod_api.h" set "SDK=%PROJ%\..\foxhollow-sdk"
if defined SDK goto :sdk_found
goto :download_sdk

:download_sdk
where git.exe >nul 2>nul
if errorlevel 1 goto :missing_git
if not exist "%DEPS%" mkdir "%DEPS%"
if errorlevel 1 goto :dependency_failed
set "SDK=%DEPS%\foxhollow"
if exist "%SDK%" rmdir /s /q "%SDK%"
echo.
echo [SETUP] Foxhollow source was not found locally.
echo [SETUP] Downloading the official Foxhollow source and submodules...
git clone --depth 1 --recurse-submodules --shallow-submodules https://github.com/JackPriceBurns/foxhollow.git "%SDK%"
if errorlevel 1 goto :dependency_failed
if not exist "%SDK%\port\include\foxhollow_mod_api.h" goto :dependency_failed
if not exist "%SDK%\extern\aurora\include\dolphin\pad.h" goto :dependency_failed

:sdk_found
if exist "%SDK%\extern\aurora\include\dolphin\pad.h" goto :sdk_ready
where git.exe >nul 2>nul
if errorlevel 1 goto :missing_submodules
git -C "%SDK%" submodule update --init --recursive
if errorlevel 1 goto :dependency_failed
if not exist "%SDK%\extern\aurora\include\dolphin\pad.h" goto :dependency_failed

:sdk_ready
echo.
echo ============================================================
echo Better Aiming %VERSION% - Windows x64 Build
echo ============================================================
echo.
echo [OK] Project root: "%PROJ%"
echo [OK] Foxhollow source: "%SDK%"

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto :missing_vs
set "INFOFILE=%TEMP%\better_aiming_vs_%RANDOM%_%RANDOM%.txt"
"%VSWHERE%" -latest -products * -version "[17.0,19.0)" -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath > "%INFOFILE%"
set "VSROOT="
if exist "%INFOFILE%" set /p "VSROOT="<"%INFOFILE%"
if exist "%INFOFILE%" del /q "%INFOFILE%" >nul 2>nul
if not defined VSROOT goto :missing_vs
if not exist "%VSROOT%\VC\Auxiliary\Build\vcvars64.bat" goto :missing_vs

"%VSWHERE%" -latest -products * -version "[17.0,19.0)" -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationVersion > "%INFOFILE%"
set "VSVER="
if exist "%INFOFILE%" set /p "VSVER="<"%INFOFILE%"
if exist "%INFOFILE%" del /q "%INFOFILE%" >nul 2>nul
if not defined VSVER goto :missing_vs
set "VSMAJOR=%VSVER:~0,2%"
set "GENERATOR="
if "%VSMAJOR%"=="18" set "GENERATOR=Visual Studio 18 2026"
if "%VSMAJOR%"=="17" set "GENERATOR=Visual Studio 17 2022"
if not defined GENERATOR goto :unsupported_vs

echo [OK] Visual Studio %VSVER%: "%VSROOT%"

set "CLANGCL="
set "CLANG_IN_VS=0"
if exist "%VSROOT%\VC\Tools\Llvm\x64\bin\clang-cl.exe" set "CLANGCL=%VSROOT%\VC\Tools\Llvm\x64\bin\clang-cl.exe"
if exist "%VSROOT%\VC\Tools\Llvm\bin\clang-cl.exe" if not defined CLANGCL set "CLANGCL=%VSROOT%\VC\Tools\Llvm\bin\clang-cl.exe"
if defined CLANGCL set "CLANG_IN_VS=1"
if not defined CLANGCL where clang-cl.exe >nul 2>nul
if not defined CLANGCL if not errorlevel 1 set "CLANGCL=clang-cl.exe"
if not defined CLANGCL goto :missing_clang
echo [OK] ClangCL: %CLANGCL%

set "CMAKEEXE="
if exist "%VSROOT%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" set "CMAKEEXE=%VSROOT%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if not defined CMAKEEXE where cmake.exe >nul 2>nul
if not defined CMAKEEXE if not errorlevel 1 set "CMAKEEXE=cmake.exe"
if not defined CMAKEEXE goto :missing_cmake
echo [OK] CMake: %CMAKEEXE%
"%CMAKEEXE%" --version
if errorlevel 1 goto :missing_cmake

set "BUILD_KIND=VS"
"%CMAKEEXE%" --help | findstr /C:"%GENERATOR%" >nul
if errorlevel 1 set "BUILD_KIND=Ninja"
if "%CLANG_IN_VS%"=="0" set "BUILD_KIND=Ninja"
if "%BUILD_KIND%"=="Ninja" goto :prepare_ninja
goto :clean_build

:prepare_ninja
if exist "%VSROOT%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe" set "PATH=%VSROOT%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%PATH%"
where ninja.exe >nul 2>nul
if errorlevel 1 goto :missing_ninja
call "%VSROOT%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 goto :vcvars_failed

:clean_build
if exist "%BUILD%" rmdir /s /q "%BUILD%"
if exist "%BUILD%" goto :cannot_clean
if exist "%OUT%" rmdir /s /q "%OUT%"
if exist "%OUT%" goto :cannot_clean
mkdir "%OUT%"
if errorlevel 1 goto :cannot_clean

echo.
echo [1/4] Configuring...
if "%BUILD_KIND%"=="Ninja" goto :configure_ninja
"%CMAKEEXE%" -S "%PROJ%" -B "%BUILD%" -G "%GENERATOR%" -A x64 -T ClangCL "-DFOXHOLLOW_SOURCE_DIR:PATH=%SDK%" -DBETTER_AIMING_PLATFORM=windows-amd64
if errorlevel 1 goto :configure_failed
goto :configured

:configure_ninja
"%CMAKEEXE%" -S "%PROJ%" -B "%BUILD%" -G Ninja "-DCMAKE_C_COMPILER:FILEPATH=%CLANGCL%" -DCMAKE_BUILD_TYPE=Release "-DFOXHOLLOW_SOURCE_DIR:PATH=%SDK%" -DBETTER_AIMING_PLATFORM=windows-amd64
if errorlevel 1 goto :configure_failed

:configured
echo.
echo [2/4] Building mod.dll...
"%CMAKEEXE%" --build "%BUILD%" --config Release --parallel
if errorlevel 1 goto :build_failed

echo.
echo [3/4] Creating installable mod structure...
"%CMAKEEXE%" --install "%BUILD%" --config Release --prefix "%STAGE%"
if errorlevel 1 goto :install_failed
if not exist "%STAGE%\mod.json" goto :missing_output
if not exist "%STAGE%\lib\windows-amd64\mod.dll" goto :missing_output

echo.
echo [4/4] Creating ZIP...
if exist "%ARCHIVE%" del /q "%ARCHIVE%"
pushd "%STAGE%"
"%CMAKEEXE%" -E tar cf "%ARCHIVE%" --format=zip mod.json lib
if errorlevel 1 goto :zip_failed
popd
if not exist "%ARCHIVE%" goto :missing_output

echo.
echo [OK] Windows mod created:
echo %ARCHIVE%
explorer "%OUT%"
pause
exit /b 0

:zip_failed
popd
echo [ERROR] ZIP creation failed.
goto :failed

:missing_git
echo [ERROR] Git was not found and no Foxhollow source checkout is available.
echo Install Git for Windows or set FOXHOLLOW_SOURCE_DIR to an existing Foxhollow checkout.
goto :failed

:dependency_failed
echo [ERROR] The Foxhollow source could not be prepared.
echo Check the Git output above or set FOXHOLLOW_SOURCE_DIR manually.
goto :failed

:missing_submodules
echo [ERROR] The Foxhollow checkout is missing required submodules and Git was not found.
echo Clone Foxhollow recursively or install Git for Windows.
goto :failed

:missing_vs
echo [ERROR] Visual Studio 2022 or 2026 with Desktop development with C++ was not found.
goto :failed

:unsupported_vs
echo [ERROR] Unsupported Visual Studio version: %VSVER%
goto :failed

:missing_clang
echo [ERROR] clang-cl.exe was not found.
echo Install C++ Clang tools for Windows in the Visual Studio Installer.
goto :failed

:missing_cmake
echo [ERROR] CMake was not found.
goto :failed

:missing_ninja
echo [ERROR] The Visual Studio generator is unavailable and Ninja was not found.
goto :failed

:vcvars_failed
echo [ERROR] Visual Studio compiler environment could not be initialized.
goto :failed

:cannot_clean
echo [ERROR] Build output could not be cleaned.
goto :failed

:configure_failed
echo [ERROR] CMake configuration failed.
goto :failed

:build_failed
echo [ERROR] Compilation failed.
goto :failed

:install_failed
echo [ERROR] CMake install step failed.
goto :failed

:missing_output
echo [ERROR] Expected mod output is missing.
goto :failed

:failed
echo.
echo [NOT CREATED] No Windows release archive was produced.
pause
exit /b 1
