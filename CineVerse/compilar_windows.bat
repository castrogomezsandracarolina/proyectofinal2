@echo off
setlocal
cd /d "%~dp0"
if exist "C:\Qt\Tools\CMake_64\bin\cmake.exe" set "PATH=C:\Qt\Tools\CMake_64\bin;%PATH%"
if exist "C:\Qt\Tools\mingw1310_64\bin\g++.exe" set "PATH=C:\Qt\Tools\mingw1310_64\bin;%PATH%"
where cmake >nul 2>&1
if errorlevel 1 (
    echo ERROR: No se encuentra CMake. Instala CMake o usa CMake Tools desde VS Code.
    pause
    exit /b 1
)
where g++ >nul 2>&1
if errorlevel 1 (
    echo ERROR: No se encuentra MinGW g++. Selecciona MinGW en el kit de VS Code.
    pause
    exit /b 1
)
cmake -S . -B build-windows -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 (
    pause
    exit /b 1
)
cmake --build build-windows -j 4
if errorlevel 1 (
    pause
    exit /b 1
)
ctest --test-dir build-windows --output-on-failure
if errorlevel 1 (
    pause
    exit /b 1
)
echo.
echo Compilacion correcta. Ejecutando CineVerse con el CSV real.
"build-windows\CineVerse.exe"
pause
