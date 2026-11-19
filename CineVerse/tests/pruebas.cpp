#include "Catalogo.h"
#include "LectorCSV.h"
#include "Plataforma.h"
#include "Texto.h"
#include <algorithm>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <random>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

struct Contador : Observador {
    int cambios = 0;
    void actualizar() override { ++cambios; }
};

int main() {
    namespace fs = std::filesystem;
    const auto carpeta = fs::temp_directory_path() / "cineverse_pruebas_2026";
    fs::create_directories(carpeta);
    const auto csv = carpeta / "peliculas.csv";
    const auto listas = carpeta / "listas_prueba.txt";
    fs::remove(listas);
    {
        std::ofstream f(csv, std::ios::binary);
        f << "\xEF\xBB\xBF" "Title,Release Year,Origin/Ethnicity,Director,Cast,Genre,Wiki Page,Plot\r\n";
        f << "El Barco,2001,Peru,Mar,Actor,Drama,web,Un barco navega el mar\r\n";
        f << "Embarcación misteriosa,2002,Peru,Sol,Actriz,Terror,web,La cámara está vacía\r\n";
        f << "\"Hola, \"\"Mundo\"\"\",2003,Peru,Leo,Actor,Comedia,web,\"Una linea\r\ny otra linea\"\r\n";
        f << "El árbol mágico,2004,Peru,Luz,Actor,Fantasia,web,Arbolito mágico\r\n";
        f << "The Matrix,1999,USA,Lana,Actor,Ciencia ficción,web,Una historia digital\r\n";
        for (int i = 0; i < 201; ++i)
            f << "Pelicula " << i << ",2010,Peru,Autor,Actor,Drama,web,Un viaje sencillo\r\n";
        f << "fila,incorrecta\n";
    }
    auto datos = LectorCSV::leer_peliculas(csv.string());
    assert(datos.size() == 206);
    assert(datos[2].titulo == "Hola, \"Mundo\"");
    assert(datos[2].sinopsis.find("\r\ny otra linea") != std::string::npos);
    assert(Texto::normalizar("ÁRBOL y Cámara") == "arbol y camara");

    auto& catalogo = Catalogo::instancia();
    catalogo.establecerArchivoListas(listas.string());
    Contador obs;
    catalogo.suscribir(&obs);
    catalogo.suscribir(&obs); // Observer evita registros duplicados.
    assert(catalogo.cargar(csv.string()));
    assert(obs.cambios == 1);
    assert(catalogo.peliculas().size() == 206);
    assert(catalogo.totalPalabras() > 20);
    assert(catalogo.totalNodos() > 1);
    assert(&catalogo == &Catalogo::instancia());

    for (const std::string& q : {"bar", "barco", "barc", "cámara", "arbol", "arbolito",
                                 "mundo", "linea", "barco navega", "viaje sencillo", "Pelicula",
                                 "0", "historia digital", "inexistente", "ÁRBOL", "mágico"}) {
        auto a = catalogo.buscar(q, ModoBusqueda::Trie);
        auto b = catalogo.buscar(q, ModoBusqueda::Lineal);
        auto c = catalogo.buscar(q, ModoBusqueda::Paralela, 4);
        if (a != b || b != c) {
            std::cerr << "Distintos resultados para: " << q << "\n";
            return 1;
        }
    }
    auto barcos = catalogo.buscar("barc");
    assert(barcos.size() == 2);
    assert(barcos[0] == 1);
    assert(catalogo.buscar("camara")[0] == 2);
    assert(catalogo.buscar("mundo")[0] == 3);
    assert(catalogo.buscar("arbol")[0] == 4);
    assert(catalogo.buscar("barco navega")[0] == 1);
    assert(catalogo.buscar("zzzinventado12345 plmxyz987654").empty());
    // OR entre palabras de frase; no requiere que aparezcan juntas.
    auto frase = catalogo.buscar("barco vacia");
    assert(std::find(frase.begin(), frase.end(), 1) != frase.end());
    assert(std::find(frase.begin(), frase.end(), 2) != frase.end());
    assert(!catalogo.buscarPorTag("Mar", CampoBusqueda::Director).empty());
    assert(catalogo.buscarPorTag("Mar", CampoBusqueda::Director)[0] == 1);
    assert(catalogo.buscarPorTag("terror", CampoBusqueda::Genero)[0] == 2);
    assert(catalogo.buscarPorTag("actriz", CampoBusqueda::Elenco)[0] == 2);
    assert(catalogo.buscarPorTag("peru", CampoBusqueda::Origen).size() == 205);
    for (auto campo : {CampoBusqueda::Director, CampoBusqueda::Elenco, CampoBusqueda::Genero, CampoBusqueda::Origen}) {
        for (const auto& q : {"mar", "actor", "drama", "peru", "desconocido otro"}) {
            auto t = catalogo.buscarPorTag(q, campo, ModoBusqueda::Trie);
            auto l = catalogo.buscarPorTag(q, campo, ModoBusqueda::Lineal);
            auto pp = catalogo.buscarPorTag(q, campo, ModoBusqueda::Paralela, 4);
            assert(t == l && l == pp);
        }
    }

    Plataforma baseOriginal;
    assert(baseOriginal.total_peliculas() == 206);
    assert(baseOriginal.buscar("mundo").size() == 1);
    assert(catalogo.alternarFavorito(2));
    assert(catalogo.alternarPendiente(2));
    assert(!catalogo.alternarFavorito(2));
    assert(catalogo.alternarFavorito(2));
    assert(!catalogo.alternarFavorito(99999));
    assert(obs.cambios == 5);
    assert(catalogo.cargar(csv.string()));
    assert(catalogo.esFavorito(2) && catalogo.esPendiente(2));
    assert(obs.cambios == 6);
    // Comparacion exhaustiva: substrings sobre palabras aleatorias.
    std::mt19937 gen(2026);
    std::uniform_int_distribution<int> letra(0, 5);
    std::uniform_int_distribution<int> largo(3, 12);
    std::vector<Pelicula> muestra;
    for (int i = 0; i < 140; ++i) {
        Pelicula p; p.id = i + 1;
        for (int j = 0, n = largo(gen); j < n; ++j) p.titulo += char('a' + letra(gen));
        for (int j = 0, n = largo(gen); j < n; ++j) p.sinopsis += char('a' + letra(gen));
        muestra.push_back(std::move(p));
    }
    IndiceSufijos indicePrueba;
    std::vector<PeliculaNormalizada> textosPrueba;
    indicePrueba.construir(muestra, textosPrueba);
    BusquedaLineal lineal(textosPrueba);
    for (const auto& p : muestra) {
        for (size_t ini = 0; ini < p.titulo.size(); ++ini) {
            for (size_t lon = 1; lon <= 4 && ini + lon <= p.titulo.size(); ++lon) {
                std::string q = p.titulo.substr(ini, lon);
                auto t = indicePrueba.buscar(q);
                auto l = lineal.buscar(q);
                assert(t == l);
            }
        }
    }
    catalogo.desuscribir(&obs);
    // La persistencia sigue a la pelicula, no al numero de fila del CSV.
    const auto csvA = carpeta / "orden_a.csv";
    const auto csvB = carpeta / "orden_b.csv";
    const std::string cabecera = "Release Year,Title,Origin/Ethnicity,Director,Cast,Genre,Wiki Page,Plot\n";
    const std::string a = "2000,Estrella azul,Peru,Pedro,Ana,Drama,https://wiki/estrella,Historia azul\n";
    const std::string b = "2001,Sol naranja,Peru,Luisa,Juan,Comedia,https://wiki/sol,Historia naranja\n";
    { std::ofstream f(csvA); f << cabecera << a << b; }
    { std::ofstream f(csvB); f << cabecera << b << a; }
    assert(catalogo.cargar(csvA.string()));
    assert(catalogo.alternarFavorito(1));
    assert(catalogo.alternarPendiente(1));
    assert(catalogo.alternarVista(1));
    assert(catalogo.registrarVisita(1));
    assert(catalogo.cargar(csvB.string()));
    assert(catalogo.obtener(2)->titulo == "Estrella azul");
    assert(catalogo.esFavorito(2) && catalogo.esPendiente(2));
    assert(catalogo.estaVista(2) && catalogo.vecesAbierta(2) == 1);
    assert(!catalogo.esFavorito(1) && !catalogo.estaVista(1));
    fs::remove_all(carpeta);
    std::cout << "OK: parser CSV, normalizacion, Trie, Strategy, Factory Method, "
                 "Singleton, Observer, paralelismo y persistencia.\n";
}
