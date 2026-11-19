@echo off
setlocal
cd /d "%~dp0"
echo CineVerse - Qt 6 Widgets
set "QT_ROOT=C:\Qt\6.12.0\mingw_64"
if not exist "%QT_ROOT%\lib\cmake\Qt6\Qt6Config.cmake" (
  echo No se encontro Qt 6 en %QT_ROOT%
  echo Edita QT_ROOT en este archivo o utiliza la opcion CMAKE_PREFIX_PATH desde VS Code.
  pause
  exit /b 1
)
if exist "C:\Qt\Tools\CMake_64\bin\cmake.exe" set "PATH=C:\Qt\Tools\CMake_64\bin;%PATH%"
if exist "C:\Qt\Tools\Ninja\ninja.exe" set "PATH=C:\Qt\Tools\Ninja;%PATH%"
if exist "C:\Qt\Tools\mingw1510_64\bin\g++.exe" set "PATH=C:\Qt\Tools\mingw1510_64\bin;%PATH%"
where g++ >nul 2>&1
if errorlevel 1 (
  echo Falta g++. Configura en VS Code el kit MinGW COMPATIBLE con el Qt instalado.
  pause
  exit /b 1
)
g++ --version | findstr /i "g++" 
where cmake >nul 2>&1
if errorlevel 1 (
  echo Falta CMake. Instala CMake o configura CMake Tools.
  pause
  exit /b 1
)
cmake -S . -B build-gui -G Ninja -DCMAKE_BUILD_TYPE=Release -DCINEVERSE_CON_QT=ON -DCMAKE_PREFIX_PATH="%QT_ROOT%"
if errorlevel 1 (
  echo ERROR: revisa que el compilador MinGW coincida con el de Qt 6.
  pause
  exit /b 1
)
cmake --build build-gui --target CineVerseQt -j 4
if errorlevel 1 (
  pause
  exit /b 1
)
echo.
echo Compilado. Para ejecutar el EXE fuera de VS Code puede ser necesario configurar DLLs de Qt.
echo Si la terminal tiene Qt en PATH, se abrira la interfaz:
"build-gui\CineVerseQt.exe"
pause
