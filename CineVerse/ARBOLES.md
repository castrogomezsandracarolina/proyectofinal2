# Elección del árbol: Trie de sufijos con nodos de caracteres

## Justificación

El enunciado pide buscar **palabras, frases y subpalabras** usando un árbol cuyos nodos contengan caracteres alfabéticos o numéricos. Un Trie simple de prefijos encontraría `barco`, pero no `arc` dentro de `barco`. Por eso usamos un **Trie de sufijos**. Para reducir redundancia, primero creamos un vocabulario único con las palabras de todos los campos relevantes, y solo entonces insertamos sus sufijos. Un **índice invertido** enlaza cada término del vocabulario con los identificadores de las películas en las que aparece.

A diferencia de la versión anterior, **ahora cada nodo guarda un solo carácter (`Nodo::caracter`)**, no fragmentos en aristas. Esto sigue literalmente el requisito sobre los valores de los nodos.

## Ejemplo

Para la palabra `barco` se insertan cinco rutas desde la raíz:

```text
raíz
 ├─ b - a - r - c - o [fin: "barco"]
 ├─ a - r - c - o     [fin: "barco"]
 ├─ r - c - o         [fin: "barco"]
 ├─ c - o             [fin: "barco"]
 └─ o                 [fin: "barco"]
```

En la implementación las rutas con caracteres comunes comparten nodos. Los hijos están enlazados como lista mediante `hijo` y `hermano`, con un carácter por nodo. Los terminales guardan IDs de términos y los términos mantienen seis listas invertidas: título, sinopsis, director, elenco, género y origen.

## Construcción

1. `LectorCSV::leer_peliculas()` lee encabezados, campos entrecomillados, comas y saltos de línea en la sinopsis.
2. `Texto::normalizar()` convierte a minúsculas y normaliza algunas letras acentuadas. `Texto::palabras()` separa por delimitadores.
3. `IndiceSufijos::construir()` guarda palabras únicas de cada película/campo y sus posting lists de películas.
4. `IndiceSufijos::insertarSufijo()` recorre los caracteres desde cada posición hasta el final de una palabra, crea nodos según sea necesario y registra el ID de término terminal.

## Búsqueda

1. Una palabra o subpalabra avanza **carácter por carácter** desde la raíz. Si falta una arista, no hay coincidencias.
2. Si existe la ruta, se recorre el subárbol, se recuperan las palabras terminadas y se unen sus listas invertidas eliminando repeticiones.
3. Para una frase, `BusquedaTrie::buscar()` hace una **unión** (OR) de las películas encontradas para cada término. Así, `barco fantasma` incluye películas con cualquiera de las dos palabras.
4. Para etiquetas se consultan las posting lists del campo elegido. Para búsqueda general se unen título y sinopsis.
5. `Catalogo::puntaje()` calcula una puntuación una sola vez por película: más aciertos, mayor puntuación; coincidencias exactas de título y frases reciben bonos. Se ordena por puntuación descendente e ID ascendente.

## Características y limitaciones

- Nodo: carácter, primer hijo, siguiente hermano y primer terminal. Se evita reservar una tabla de 36/256 punteros para cada nodo.
- El índice acelera las consultas repetidas, a cambio de mayor uso de memoria al cargar el CSV. No se indexan frases completas, sino palabras y todos sus sufijos.
- Para textos no latinos la normalización UTF-8 es parcial; las grafías originales se conservan para visualización.
- La búsqueda de subpalabras trabaja dentro de cada palabra, no cruzando un espacio. Las frases tienen semántica OR por requisito, no coincidencia literal obligatoria.
- Ejemplo con el CSV incluido: **34 886 películas**, **156 691 palabras**, **1 173 066 nodos**.
