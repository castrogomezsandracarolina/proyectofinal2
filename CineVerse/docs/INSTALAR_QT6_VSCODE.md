# Abrir CineVerseQt en VS Code — Windows 11 y Qt 6

## Instalar y comprobar las herramientas

1. En el instalador de Qt selecciona **Qt 6.12.0 → MinGW 15.1.0 64-bit** (o tu variante de Qt para MSVC si utilizas el compilador MSVC). Para usar Qt 6 Widgets, el módulo Widgets forma parte de la distribución Qt para escritorio.
2. En **Build Tools** comprueba que tienes **CMake**, **Ninja** y **el compilador de MinGW compatible con tu paquete de Qt**. El texto `MinGW 15.1` junto a Qt señala con qué toolchain se construyeron esas bibliotecas. En algunas versiones del instalador aparecen por separado paquetes de compilador de otro número; **no uses mezclas de versiones sin verificar compatibilidad**.
3. En VS Code instala las extensiones **C/C++** y **CMake Tools**. Abre la carpeta `CineVerse`, no un archivo suelto.
4. Ve a `Ctrl+Shift+P` → **CMake: Select a Kit**. Selecciona el kit correspondiente al compilador instalado que sea compatible con tu Qt. No selecciones MSVC para una instalación de Qt-MinGW.

## Configurar el objetivo gráfico

En `Ctrl+Shift+P` → **CMake: Edit User-Local CMake Kits** puedes añadir o corregir el compilador si hace falta. El proyecto incluye un preset **`qt6-windows`** (abre `CMake: Select Configure Preset`), que requiere Ninja y una instalación en `C:/Qt/6.12.0/mingw_64`. Con el kit compatible activo, configura las opciones:

- `CINEVERSE_CON_QT=ON`
- `CMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64` (adapta esta ruta al directorio de Qt que contiene `lib/cmake/Qt6/Qt6Config.cmake`).

También puedes abrir una terminal integrada y ejecutar:

```powershell
cmake -S . -B build-qt -DCINEVERSE_CON_QT=ON -DCMAKE_PREFIX_PATH="C:/Qt/6.12.0/mingw_64"
cmake --build build-qt --config Release --target CineVerseQt
```

**Si aparece un error del generador** porque `build-qt` se configuró antes con otro kit, elimina solamente la carpeta `build-qt` y vuelve a ejecutar la configuración con el kit correcto.

Para ejecutar desde VS Code, selecciona el objetivo **CineVerseQt** y pulsa **Run**. El programa busca el CSV cerca de su ubicación; si no lo encuentra, usa **Abrir archivo CSV** y selecciona `data/wiki_movie_plots_deduped.csv`.

## Errores frecuentes

- `Could not find Qt6Config.cmake`: `CMAKE_PREFIX_PATH` apunta a una carpeta equivocada. Debe ser la carpeta del paquete de Qt que contiene `lib/cmake/Qt6`.
- `undefined reference` o error de DLL al ejecutar: compilador/arquitectura/Qt no compatibles. Usa un kit de la misma arquitectura y familia que tu paquete Qt.
- `cmake: command not found`: agrega CMake al PATH o usa el CMake que instala Qt desde la extensión CMake Tools.
- No aparece `CineVerseQt`: revisa que `CINEVERSE_CON_QT=ON`; el backend `CineVerse` se compila sin Qt por defecto.
- Si el EXE se ejecuta fuera de VS Code y faltan DLL de Qt, utiliza la herramienta `windeployqt` del mismo paquete de Qt o configura correctamente el entorno de ejecución.

## Qué hará la app

Inicio → lista de Ver más tarde y recomendaciones. Buscar → consultas por título/sinopsis y por tag; elegir Trie/Lineal/Paralela; 5 resultados por página. En los detalles puedes guardar Me gusta, Ver más tarde y Ya vista. Historial registra aperturas. Todo se recupera al iniciar porque lo guarda automáticamente el perfil en `%LOCALAPPDATA%\CineVerse\perfil_cv2.txt`.
