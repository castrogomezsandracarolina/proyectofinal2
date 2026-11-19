#include "Catalogo.h"
#include <algorithm>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    namespace fs = std::filesystem;
    if (argc < 2) { std::cerr << "Falta la ruta del CSV real\n"; return 2; }
    const auto rutaPerfil = fs::temp_directory_path() / "cineverse_real_perfil_pruebas.txt";
    fs::remove(rutaPerfil);
    fs::remove(rutaPerfil.string() + ".bak");
    auto& c = Catalogo::instancia();
    c.establecerArchivoListas(rutaPerfil.string());
    auto inicio = std::chrono::steady_clock::now();
    assert(c.cargar(argv[1]));
    assert(c.peliculas().size() == 34886);
    assert(c.obtener(1)->titulo == "Kansas Saloon Smashers");
    assert(c.obtener(2)->titulo == "Love by the Light of the Moon");
    assert(c.totalPalabras() > 100000);
    assert(c.totalNodos() > c.totalPalabras());
    std::cout << "CSV real: " << c.peliculas().size() << " peliculas, "
              << c.totalPalabras() << " palabras, " << c.totalNodos() << " nodos.\n";
    for (const std::string& q : {"moon", "smasher", "policeman", "action", "king",
                                "city", "romance", "comedy", "cinema", "mystery",
                                "life", "water", "love", "detective", "espana",
                                "war", "true love", "barco fantasma", "sin resultado xyz987"}) {
        const auto trie = c.buscar(q, ModoBusqueda::Trie);
        const auto lineal = c.buscar(q, ModoBusqueda::Lineal);
        const auto paralelo = c.buscar(q, ModoBusqueda::Paralela, 4);
        if (trie != lineal || lineal != paralelo) {
            std::cerr << "DIFERENCIA entre buscadores para: " << q << "\n";
            return 3;
        }
    }
    for (auto campo : {CampoBusqueda::Director, CampoBusqueda::Elenco, CampoBusqueda::Genero, CampoBusqueda::Origen}) {
        for (const auto& palabra : {"american", "horror", "hitchcock", "chaplin", "love war"}) {
            const auto a = c.buscarPorTag(palabra, campo, ModoBusqueda::Trie);
            const auto b = c.buscarPorTag(palabra, campo, ModoBusqueda::Lineal);
            const auto d = c.buscarPorTag(palabra, campo, ModoBusqueda::Paralela, 4);
            assert(a == b && b == d);
        }
    }
    auto luna = c.buscar("love by the light of the moon");
    assert(!luna.empty() && luna[0] == 2);
    assert(c.alternarFavorito(2));
    assert(c.alternarPendiente(2));
    assert(c.alternarVista(2));
    assert(c.registrarVisita(2));
    assert(c.registrarVisita(1));
    assert(c.registrarVisita(2));
    assert(c.historial().front() == 1 || c.historial().front() == 2);
    assert(c.vecesAbierta(2) == 2);
    assert(c.lista(true).size() == 1);
    assert(c.lista(false).size() == 1);
    assert(c.listaVistas().size() == 1);
    assert(fs::exists(rutaPerfil));
    assert(c.cargar(argv[1]));  // Simula cerrar y abrir: vuelve a leer del disco.
    assert(c.esFavorito(2) && c.esPendiente(2) && c.estaVista(2));
    assert(c.vecesAbierta(2) == 2 && c.vecesAbierta(1) == 1);
    assert(c.historial().size() == 2);
    assert(c.quitarDelHistorial(1));
    assert(c.historial().size() == 1);
    assert(c.borrarHistorial());
    assert(c.historial().empty());
    assert(c.esFavorito(2) && c.esPendiente(2));
    assert(c.cargar(argv[1]));
    assert(c.historial().empty() && c.esFavorito(2));
    fs::remove(rutaPerfil);
    const auto segundos = std::chrono::duration<double>(std::chrono::steady_clock::now() - inicio).count();
    std::cout << "OK: CSV REAL, Trie/lineal/paralela iguales, historial y estado tras reinicio. "
              << "Tiempo: " << segundos << " s\n";
}
