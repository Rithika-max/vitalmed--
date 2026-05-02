@echo off
setlocal EnableDelayedExpansion

cd /d "%~dp0"

echo Starting Vetalmed backend and frontend...
set BACKEND_DIR=%~dp0core
set BUILD_DIR=%~dp0build_core
set FRONTEND_DIR=%~dp0frontend
set BACKEND_PORT=3003
set VCPKG_TOOLCHAIN=%~dp0vcpkg\scripts\buildsystems\vcpkg.cmake

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

set BACKEND_EXE=%BUILD_DIR%\erp_ai_backend.exe
if not exist "%BACKEND_EXE%" (
    echo Building backend...
    if exist "%VCPKG_TOOLCHAIN%" (
        cmake -S "%BACKEND_DIR%" -B "%BUILD_DIR%" -G Ninja -DCMAKE_TOOLCHAIN_FILE="%VCPKG_TOOLCHAIN%" -DCMAKE_BUILD_TYPE=Release
    ) else (
        cmake -S "%BACKEND_DIR%" -B "%BUILD_DIR%" -G Ninja -DCMAKE_BUILD_TYPE=Release
    )
    if errorlevel 1 (
        echo ERROR: CMake configuration failed.
        goto END
    )
    cmake --build "%BUILD_DIR%" --config Release
    if errorlevel 1 (
        echo ERROR: Backend build failed.
        goto END
    )
)

if not exist "%BACKEND_EXE%" if exist "%BUILD_DIR%\Release\erp_ai_backend.exe" set BACKEND_EXE=%BUILD_DIR%\Release\erp_ai_backend.exe

if not exist "%BACKEND_EXE%" (
    echo ERROR: Backend executable not found: %BACKEND_EXE%
    echo Please build the backend with CMake or inspect the build folder.
    goto END
)

echo Starting C++ backend on port %BACKEND_PORT%...
start "Vetalmed Backend" cmd /k "cd /d "%BUILD_DIR%" && "%BACKEND_EXE%""

echo Waiting for backend health on http://localhost:%BACKEND_PORT%/health ...
powershell -NoProfile -Command "^ 
$success = $false; ^
for ($i = 0; $i -lt 30; $i++) { ^
  try { ^
    $r = Invoke-RestMethod -Uri 'http://localhost:%BACKEND_PORT%/health' -TimeoutSec 2; ^
    if ($r.backend -eq 'running') { $success = $true; break } ^
  } catch {}; ^
  Start-Sleep -Seconds 1; ^
}; ^
if (-not $success) { exit 1 } else { exit 0 }"
if errorlevel 1 (
    echo ERROR: Backend failed to become healthy on port %BACKEND_PORT%.
    echo Check storage\logs\backend.log and ensure no other process is using port %BACKEND_PORT%.
    goto END
)

echo C++ Backend running: http://localhost:%BACKEND_PORT%
cd /d "%FRONTEND_DIR%"
if not exist node_modules (
    echo Installing frontend dependencies...
    npm install
    if errorlevel 1 (
        echo ERROR: npm install failed.
        goto END
    )
)
echo Starting frontend...
start "Vetalmed Frontend" cmd /k "cd /d "%FRONTEND_DIR%" && npm run dev"
echo Frontend running: http://localhost:3000

:END
endlocal
