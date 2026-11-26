@echo off
setlocal
where cmake >nul 2>nul
if not errorlevel 1 (
    cmake --preset vs2022-win32-release
    if errorlevel 1 exit /b %errorlevel%
    cmake --build --preset build-release
    exit /b %errorlevel%
)
where msbuild >nul 2>nul
if errorlevel 1 (
    echo Neither CMake nor MSBuild is in PATH. Open "Developer Command Prompt for VS 2022".
    exit /b 1
)
msbuild DialogueMovementEnabler.sln /m /p:Configuration=Release /p:Platform=Win32
exit /b %errorlevel%
