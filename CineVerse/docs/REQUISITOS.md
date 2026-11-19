# Revisión de requisitos frente al enunciado

| Requisito | Implementación |
|---|---|
| CSV y preprocesamiento | `LectorCSV.h` y `Texto.h` |
| Árbol con caracteres en nodos | `IndiceSufijos.h`, `Nodo::caracter`, un carácter por nodo |
| Palabra y subpalabra | `IndiceSufijos::buscar` |
| Frase A y/o B | `BusquedaTrie::buscar`, `std::set_union` (OR) |
| Buscar por tag | `Catalogo::buscarPorTag`, Director, Elenco, Género, Origen |
| Cinco resultados y siguientes cinco | `main.cpp::mostrarPaginas`, `InterfazQt.cpp::mostrarResultados` |
| Algoritmo de importancia | `Catalogo::puntaje` |
| Sinopsis y acciones Me gusta / Ver más tarde | `main.cpp::detallePelicula`, `InterfazQt.cpp::mostrarDetalle` |
| Ver más tarde al iniciar | `main.cpp` al terminar de cargar, `InterfazQt.cpp::irA(Inicio)` |
| Recomendaciones en base a Likes | `Catalogo::recomendar` (género, director, elenco, sinopsis) |
| Cuatro patrones | Singleton, Strategy, Factory Method, Observer |
| Programación paralela | `BusquedaParalela::buscar`, `std::async` |
| Interfaz gráfica | `InterfazQt.cpp` (requiere verificar ejecución en Windows) |
| Todo en C++ | C++20 y Qt 6 Widgets (C++) |
| Documentación y GitHub | README, ARBOLES, GUIA_PARTE2, REQUISITOS. **Publicar repositorio en GitHub es una acción del equipo pendiente de confirmar**. |

**Notas:** la prueba con Qt no se pudo ejecutar en el entorno de desarrollo sin bibliotecas Qt 6. No confundir código de GUI ya escrito con aplicación ya comprobada en Windows. No hay reproducciones de video porque la base de datos solo incluye metadatos y sinopsis.
