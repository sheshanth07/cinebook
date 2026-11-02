@echo off
setlocal enabledelayedexpansion

echo ===================================================
echo   CineBook - Building C++ Backend (Windows MSVC)
echo ===================================================

rem Check if cl is already in path
where cl >nul 2>nul
if %errorlevel% equ 0 (
    goto COMPILE
)

rem Search for vcvars64.bat in standard installation paths
set "VCVARS="
if exist "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
    set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
    set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
    set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
)

if "%VCVARS%"=="" (
    echo [Error] Microsoft Visual C++ compiler not found. Please install Visual Studio Build Tools.
    exit /b 1
)

echo [Info] Initializing MSVC environment: %VCVARS%
call "%VCVARS%"

:COMPILE
echo [Info] Compiling CineBook C++ backend...

rem Compile sqlite3 first as object file if not already compiled to speed up builds
if not exist "sqlite3.obj" (
    echo [Info] Compiling SQLite3 engine...
    cl /nologo /O2 /c backend\src\sqlite3.c /Ibackend\include
)

echo [Info] Linking CineBook application...
cl /nologo /O2 /std:c++17 /EHsc /D_CRT_SECURE_NO_WARNINGS /Ibackend\include ^
    backend\src\main.cpp ^
    backend\src\auth_service.cpp ^
    backend\src\database.cpp ^
    backend\src\movie_service.cpp ^
    backend\src\booking_service.cpp ^
    backend\src\admin_service.cpp ^
    backend\src\http_server.cpp ^
    sqlite3.obj ^
    ws2_32.lib ^
    /Fe:cinebook_server.exe

if %errorlevel% neq 0 (
    echo.
    echo [FAILURE] Build failed. Review errors above.
    exit /b %errorlevel%
)

echo.
echo ===================================================
echo [SUCCESS] Build succeeded! Executable: cinebook_server.exe
echo Run "run.bat" or ".\cinebook_server.exe" to start.
echo ===================================================
