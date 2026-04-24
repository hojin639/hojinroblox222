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
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Copy failed.
    goto :RETRY_CHECK
)

echo [SUCCESS] Build and install complete.
goto :RUN

:RETRY_CHECK
if %RETRY% lss %MAX_RETRY% (
    echo [INFO] Retrying in %RETRY% second(s)...
    timeout /t %RETRY% /nobreak >nul
    goto :BUILD_LOOP
)
echo [FATAL] All %MAX_RETRY% attempts failed.
pause
exit /b 1

:RUN
echo.
echo [INFO] Copying config...
copy /Y "%CONFIG_SRC%" "%CONFIG_DST%" >nul

echo [INFO] Launching BitcoinMinerClean.exe...
cd /d "%INSTALL_DIR%"
BitcoinMinerClean.exe

pause
