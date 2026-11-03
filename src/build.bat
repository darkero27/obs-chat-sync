@echo off
setlocal
echo ============================================================
echo         Building OBS Auto Chat Sync from Source
echo ============================================================
echo.

:: Detect Visual Studio
set "VCVARS="
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
    set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" (
    set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
)

if "%VCVARS%"=="" (
    echo [ERROR] MSVC 64-bit compiler (vcvars64.bat) not found.
    echo         Please install Visual Studio 2022 with C++ Build Tools.
    pause
    exit /b 1
)

echo [*] Initializing MSVC x64 environment...
call "%VCVARS%" >nul

echo [*] Compiling obs-chat-sync.dll...
if not exist "%~dp0..\bin\64bit" mkdir "%~dp0..\bin\64bit"
cl /LD /O2 /W3 /Fe:"%~dp0..\bin\64bit\obs-chat-sync.dll" ^
   "%~dp0obs_chat_sync.c" ^
   "%~dp0lib\obs.lib" ^
   "%~dp0lib\obs-frontend-api.lib" ^
   user32.lib gdi32.lib comctl32.lib dwmapi.lib

if %errorlevel% neq 0 (
    echo [ERROR] DLL Build failed!
    pause
    exit /b 1
)

del "%~dp0..\bin\64bit\obs-chat-sync.exp" >nul 2>&1
del "%~dp0..\bin\64bit\obs-chat-sync.lib" >nul 2>&1
del "%~dp0obs_chat_sync.obj" >nul 2>&1

echo [*] Compiling OBS-Chat-Sync-Installer.exe...
if not exist "%~dp0..\release" mkdir "%~dp0..\release"

cd /d "%~dp0"
rc /fo "%~dp0installer.res" "%~dp0installer.rc"
cl /O2 /W3 /utf-8 "%~dp0installer.c" "%~dp0installer.res" /Fe:"%~dp0..\release\OBS-Chat-Sync-Installer.exe" /link /SUBSYSTEM:WINDOWS

if %errorlevel% neq 0 (
    echo [ERROR] Installer Build failed!
    pause
    exit /b 1
)

del "%~dp0installer.res" >nul 2>&1
del "%~dp0installer.obj" >nul 2>&1

:: Copy releases
copy /y "%~dp0..\bin\64bit\obs-chat-sync.dll" "%~dp0..\release\obs-chat-sync.dll" >nul
copy /y "%~dp0..\scripts\obs_chat_sync.lua" "%~dp0..\release\obs_chat_sync.lua" >nul

echo.
echo [SUCCESS] Built obs-chat-sync.dll & OBS-Chat-Sync-Installer.exe successfully!
echo           Release directory: %~dp0..\release
echo.
