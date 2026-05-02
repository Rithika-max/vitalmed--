@echo off
REM Debug and test VetalMed Backend Setup for Windows

echo === VetalMed Backend Debugging (Windows) ===
echo.

REM 1. Check if backend is running
echo 1. Checking if backend is running on port 8080...
curl -s http://localhost:8080/health >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo    ✓ Backend is running on port 8080
    echo    Response:
    curl -s http://localhost:8080/health
) else (
    echo    ✗ Backend is NOT running on port 8080
    echo.
    echo    To build and run backend:
    echo    cd core
    echo    mkdir build
    echo    cd build
    echo    cmake ..
    echo    cmake --build .
    echo    Debug\erp_ai_backend.exe
)

echo.

REM 2. Check if backend is running on port 8081 (alternate)
echo 2. Checking if backend is running on port 8081...
curl -s http://localhost:8081/health >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo    ✓ Backend is running on port 8081
    echo    NOTE: Frontend expects port 8081, this is correct!
) else (
    echo    ✗ Backend is NOT running on port 8081
)

echo.

REM 3. Check storage folders
echo 3. Checking storage folders...
if exist "storage\uploads" (
    echo    ✓ storage\uploads exists
) else (
    echo    ✗ storage\uploads missing - will be created on first upload
)

if exist "storage\extracted" (
    echo    ✓ storage\extracted exists
) else (
    echo    ✗ storage\extracted missing - will be created on first upload
)

if exist "storage\logs" (
    echo    ✓ storage\logs exists
    echo    Recent logs:
    for /l %%n in (1,1,5) do (
        for /f "skip=%%n" %%A in ('type "storage\logs\backend.log"') do (
            echo    %%A
        )
    )
) else (
    echo    ✗ storage\logs missing - will be created on backend startup
)

echo.

REM 4. Check datasets
echo 4. Checking datasets...
curl -s http://localhost:8080/datasets 2>nul

echo.
echo === End of Debug Report ===
