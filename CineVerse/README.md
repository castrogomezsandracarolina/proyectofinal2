# CineVerse | Programación III — Proyecto final

Plataforma de catálogo de cine escrita en **C++20**. Lee un archivo CSV de películas, permite buscar en título/sinopsis y por etiquetas, muestra fichas con sinopsis, guarda la biblioteca personal y recomienda películas relacionadas. Es un catálogo de sinopsis; el archivo suministrado no contiene videos reproducibles.

Desarrollado a partir de la primera entrega del equipo; conserva la clase `Plataforma` como interfaz de compatibilidad.

## Integrantes
- Castro Gómez Sandra Carolina
- Miguel Ángel Flores Cárdenas

## Qué contiene esta versión

- Archivo `data/wiki_movie_plots_deduped.csv` incluido (34 886 películas).
- Lector CSV que admite comillas escapadas y saltos de línea en las sinopsis. Normalización de mayúsculas y algunas tildes, separación de palabras y eliminación de repeticiones por película/campo.
- **Trie de sufijos por caracteres** (`IndiceSufijos.h`): cada nodo guarda un solo carácter, con aristas a hijos/hermanos y listas invertidas que relacionan palabras con películas. Se buscan palabras y fragmentos dentro de palabras.
- **Frases con semántica O:** `barco fantasma` devuelve películas que contengan `barco`, `fantasma` o ambas, como pide la consigna. El ranking privilegia las que contienen más palabras y la frase contigua.
- Búsqueda por **director, elenco, género u origen** usando el índice del árbol, además de título/sinopsis.
- Ranking propio: coincidencias exactas en título, coincidencias parciales, sinopsis y bonos por frase completa. Resultados paginados de **5 en 5**.
- Ficha de película con sinopsis, Me gusta, Ver más tarde, Ya vista, historial de apertura y listas persistentes.
- Se muestran automáticamente **Ver más tarde** y **recomendaciones basadas en Me gusta** al iniciar la consola; en Qt aparecen en **Inicio**.
- Algoritmo propio de recomendaciones: coincidencia de género (12 puntos por Like), director (18), reparto (7) y palabras distintivas de las sinopsis (hasta 24 puntos). Se excluyen títulos favoritos, pendientes y vistos.
- Tres estrategias de búsqueda: Trie de caracteres, lineal y paralela con `std::async`, todas con la misma semántica.
- Cuatro patrones reales: Singleton, Strategy, Factory Method y Observer.
- **Interfaz gráfica Qt 6 Widgets** en `src/InterfazQt.cpp`: Inicio, Explorar, búsqueda por tags, selector de algoritmo, paginación, sinopsis, Me gusta, Ver más tarde, Ya vistas, Historial y Para ti.

## Requisitos

- C++20 y CMake 3.20 o superior.
- Compilador C++ de 64 bits (MinGW o MSVC) para la aplicación de consola.
- Para `CineVerseQt`: **Qt 6 Widgets** compatible con el mismo compilador que utiliza CMake. En Windows no mezcles Qt compilado para MinGW con MSVC ni con una versión incompatible de MinGW.

## En Visual Studio Code (primero consola, luego interfaz)

1. Descomprime el ZIP y abre **la carpeta `CineVerse`**, que contiene `CMakeLists.txt`.
2. Instala las extensiones **C/C++** y **CMake Tools**.
3. Selecciona **CMake: Select a Kit** y un compilador compatible de 64 bits.
4. Construye el backend con los comandos:

```powershell
cmake -S . -B build -DCINEVERSE_CON_QT=OFF
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Ejecuta `build/CineVerse.exe` (o `build/Release/CineVerse.exe`, dependiendo del generador). Si tienes MinGW y CMake correctamente en PATH, también puedes usar `compilar_windows.bat`.

### Activar Qt 6 en Visual Studio Code

En una terminal donde estén visibles el compilador correspondiente y CMake:

```powershell
cmake -S . -B build-qt -DCINEVERSE_CON_QT=ON -DCMAKE_PREFIX_PATH="C:/Qt/6.12.0/mingw_64"
cmake --build build-qt --config Release --target CineVerseQt
```

Ajusta la ruta de `CMAKE_PREFIX_PATH` si tu instalación es distinta. Abre el objetivo **CineVerseQt** para iniciar la aplicación con ventana. Si falla CMake con `Qt6Config.cmake not found`, revisa esa ruta; si falla con conflictos de bibliotecas/DLL, revisa el kit/compilador. Las instrucciones más detalladas están en [`docs/INSTALAR_QT6_VSCODE.md`](docs/INSTALAR_QT6_VSCODE.md).

**Nota de validación:** el backend se compiló y sus 2 suites de pruebas finalizaron correctamente con el CSV real. El frontend Qt se incluye como código C++ listo para integrar, pero **no se pudo compilar ni ejecutar en este entorno**, que no tiene Qt 6; debes comprobarlo con tu instalación de Windows antes de la entrega.

## Cómo usar el programa

**Consola:** al iniciar se muestran hasta 5 títulos de Ver más tarde y hasta 5 sugerencias. En el menú elige **1** para buscar, escribe palabra, frase o subpalabra, selecciona si es título/sinopsis o tag y selecciona el motor de búsqueda. Se muestran 5 resultados; pulsa `S` para la siguiente página, `A` para la anterior o 1–5 para abrir una ficha y guardar acciones. La opción 5 permite comparar tiempos de los tres motores.

**Qt:** la pantalla de Inicio contiene Ver más tarde y Para ti. Usa la barra para buscar y los menús desplegables para elegir campo y algoritmo. Pulsa una película para leer la sinopsis y usa los botones Me gusta, Ver más tarde y Ya vista. El botón **Abrir archivo CSV** permite cargar un catálogo personalizado. El campo `data/wiki_movie_plots_deduped.csv` se detecta automáticamente cuando se abre el proyecto desde una carpeta cercana.

### Perfil, favoritos e historial

El perfil se guarda automáticamente cada vez que se abre una ficha o cambia el estado de una película, sin necesidad de pulsar Guardar:

- Windows: `%LOCALAPPDATA%\CineVerse\perfil_cv2.txt`
- Linux: `~/.local/share/cineverse/perfil_cv2.txt` (o `$XDG_DATA_HOME/cineverse/perfil_cv2.txt`).

Los IDs persistentes se generan con atributos de cada película (título, año, enlace, origen y director), no con el número de fila. Así sobreviven a cambios de orden en el CSV; si cambia la identidad textual de una película, se tratará como otra. La consola permite borrar solo el historial sin borrar Likes ni listas.

## Archivos importantes

```text
CineVerse/
├─ CMakeLists.txt                  Configuración CMake y objetivo opcional CineVerseQt
├─ src/
│  ├─ main.cpp                    Menú de consola, cinco resultados por página
│  ├─ InterfazQt.cpp              Interfaz Qt 6 Widgets
│  ├─ Catalogo.h                  Singleton, Observer, ranking y recomendaciones
│  ├─ IndiceSufijos.h             Trie de caracteres, sufijos y listas invertidas
│  ├─ Busquedas.h                 Strategy, Factory Method y std::async
│  ├─ LectorCSV.h                 Parser CSV y preprocesamiento
│  ├─ Texto.h                     Normalización y tokens
│  ├─ PerfilUsuario.h             Persistencia de biblioteca e historial
│  ├─ Pelicula.h / Plataforma.h   Modelo e interfaz de compatibilidad
├─ data/wiki_movie_plots_deduped.csv
├─ tests/pruebas.cpp              Pruebas unitarias, OR, tags y persistencia
├─ tests/pruebas_dataset_real.cpp Pruebas integrales con el dataset real
├─ ARBOLES.md                     Justificación y funcionamiento del árbol
└─ docs/GUIA_PARTE2.md            Qué mostrar en la exposición
```

## Datos y comprobaciones

Con el CSV incluido: **34 886 películas**, **156 691 términos únicos** y **1 173 066 nodos de caracteres**. Dos suites de pruebas automatizadas aprobadas (compilador Linux C++20): equivalencia Trie/lineal/paralela, búsquedas por tags, paginación, CSV con comillas, tildes, favoritos persistentes y perfil tras recarga. El tamaño del índice en memoria depende del compilador y de la implementación de la biblioteca estándar. En la prueba integral, la carga y las búsquedas completaron los tests en aproximadamente 20 segundos en conjunto, **no es una medición de tu Windows**.

La documentación de [`ARBOLES.md`](ARBOLES.md) y [`docs/GUIA_PARTE2.md`](docs/GUIA_PARTE2.md) explica la elección de la estructura y dónde exponer el código de los 4 requisitos.
