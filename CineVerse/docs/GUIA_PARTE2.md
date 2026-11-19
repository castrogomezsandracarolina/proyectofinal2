

## 1. Cuatro patrones de diseño — código a mostrar

1. **Singleton:** `src/Catalogo.h`, constructor privado `Catalogo()`, `Catalogo::instancia()`, eliminación de copia. Un catálogo compartido por consola y Qt.
2. **Strategy:** `src/Busquedas.h`, interfaz `EstrategiaBusqueda::buscar()` y tres implementaciones `BusquedaTrie`, `BusquedaLineal`, `BusquedaParalela`. Cambiamos el motor sin modificar la GUI.
3. **Factory Method:** `src/Busquedas.h`, método virtual `CreadorBusqueda::crear()` y `CreadorTrie`/`CreadorLineal`/`CreadorParalelo`. Cada subtipo instancia su estrategia. `Catalogo::buscarInterno()` selecciona el creador.
4. **Observer:** `src/Catalogo.h` (`Observador`, `suscribir`, `notificar`, `desuscribir`) y `src/InterfazQt.cpp` (`Ventana::actualizar()`). Al dar Me gusta o guardar, se actualiza la pantalla.

## 2. Árbol — código a mostrar

`src/IndiceSufijos.h`: `Nodo` contiene un carácter más apuntadores como índices; `obtenerOCrearHijo`, `insertarSufijo`, `construir` y `buscar`. Explicar la palabra `barco`: insertamos `barco, arco, rco, co, o`. Si buscamos `arc`, seguimos a → r → c, consultamos terminales del subárbol y recuperamos los IDs de películas desde las listas invertidas.

`src/Busquedas.h`: `BusquedaTrie::buscar` usa `std::set_union` para una consulta de varias palabras. Mostrar que `barco fantasma` devuelve películas que tienen al menos una de esas palabras. Los tags se buscan por su lista invertida correspondiente. `src/Catalogo.h`: `puntaje` usa bonos de título, palabra completa y frase para ordenar por relevancia.

Justificación extendida: `ARBOLES.md`.

## 3. Programación paralela — código a mostrar

`BusquedaParalela::buscar` en `src/Busquedas.h`:
- Determina entre 1 y 8 trabajadores.
- Divide las películas en rangos disjuntos.
- Ejecuta cada rango con `std::async(std::launch::async, ...)`.
- Cada tarea construye su propio `vector<int>` de resultados.
- Une esos vectores con `std::future::get`, evitando escrituras concurrentes en el mismo vector.

**Demostración:** en la consola pulsa opción 5 e introduce `moon`; muestra resultados y tiempos de Lineal, Paralela y Trie. Explicar que la versión paralela compara contra la lineal; el Trie es un índice preconstruido. Las ganancias dependen del equipo y del texto consultado.

## 4. Interfaz gráfica — mostrar la app, no el código

Ejecutar `CineVerseQt`. Enseñar:
- **Inicio:** Ver más tarde y recomendaciones generadas a partir de los Likes.
- **Buscar:** buscar una palabra, una frase como `love war` y una subpalabra como `moon`.
- **Tags:** elegir Director, Elenco, Género u Origen y mostrar 5 resultados por página.
- **Detalle:** abrir una película, leer sinopsis, dar Me gusta y agregar Ver más tarde.
- **Persistencia:** cerrar, volver a abrir CineVerseQt y verificar que se conservaron las listas.
- **Historial:** ver el registro de aperturas; **Para ti:** recomendaciones basadas en género/director/elenco y términos de sinopsis.

**Atención:** el backend y el CSV real sí se probaron de forma automatizada. Para presentar la interfaz es indispensable compilar y ejecutar `CineVerseQt` en Windows/VS Code con tu Qt 6 instalado. No dar por validada la GUI hasta verla funcionar en tu computadora.

## Pruebas automatizadas

```bash
cmake -S . -B build -DCINEVERSE_CON_QT=OFF
cmake --build build
ctest --test-dir build --output-on-failure
```

Dos pruebas: unidad + dataset real. El segundo test carga 34 886 películas, compara búsqueda Trie/lineal/paralela con y sin tags, prueba favoritos/historial tras volver a cargar el CSV y comprueba que el archivo de perfil se mantenga.
