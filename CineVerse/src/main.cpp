#include "Catalogo.h"
#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>

using Reloj = std::chrono::steady_clock;
namespace fs = std::filesystem;

void menu() {
    std::cout << "\n=============================================\n"
              << "           CINEVERSE - PELICULAS\n"
              << "=============================================\n"
              << "[1] Buscar palabras, frases, subpalabras y tags\n"
              << "[2] Ver mas tarde\n"
              << "[3] Salir\n"
              << "[4] Mis Me gusta\n"
              << "[5] Comparar busqueda lineal / paralela / Trie\n"
              << "[6] Estadisticas del arbol\n"
              << "[7] Historial de peliculas abiertas\n"
              << "[8] Peliculas marcadas como vistas\n"
              << "[9] Explorar catalogo\n"
              << "[10] Mi biblioteca / estadisticas\n"
              << "[11] Borrar historial (sin borrar listas)\n"
              << "[12] Recomendaciones personalizadas\n"
              << "Opcion: ";
}

std::string fecha(std::int64_t tiempo) {
    if (tiempo <= 0) return "-";
    const std::time_t t = static_cast<std::time_t>(tiempo);
    std::tm loc{};
#ifdef _WIN32
    localtime_s(&loc, &t);
#else
    localtime_r(&t, &loc);
#endif
    std::ostringstream s;
    s << std::put_time(&loc, "%d/%m/%Y %H:%M");
    return s.str();
}
void advertirGuardado() {
    auto& app = Catalogo::instancia();
    if (!app.perfilGuardadoCorrectamente())
        std::cerr << "No se pudo guardar el perfil en: " << app.rutaArchivoPerfil().string() << '\n';
}
void detallePelicula(int id) {
    auto& app = Catalogo::instancia();
    const auto* p = app.obtener(id);
    if (!p) return;
    app.registrarVisita(id);
    advertirGuardado();
    std::cout << "\n---------------------------------------------\n"
              << "TITULO: " << p->titulo << " (" << p->año << ")\n"
              << "ORIGEN: " << p->origen << "\n"
              << "DIRECTOR: " << p->director << "\n"
              << "GENERO: " << p->genero << "\n"
              << "ELENCO: " << p->elenco << "\n"
              << "SINOPSIS: " << p->sinopsis << "\n"
              << "WIKIPEDIA: " << p->link_wiki << '\n'
              << "---------------------------------------------\n";
    while (true) {
        std::cout << "\nEstado: " << (app.esFavorito(id) ? "[Me gusta] " : "")
                  << (app.esPendiente(id) ? "[Ver mas tarde] " : "")
                  << (app.estaVista(id) ? "[Ya vista] " : "")
                  << "\nAbierta " << app.vecesAbierta(id) << " veces | Ultima: "
                  << fecha(app.ultimaApertura(id)) << '\n';
        std::cout << "[1] Like / quitar Like   [2] Ver mas tarde / quitar\n"
                  << "[3] Marcar como vista / no vista   [4] Quitar del historial\n"
                  << "[0] Volver: ";
        std::string accion;
        if (!std::getline(std::cin, accion) || accion == "0") return;
        if (accion == "1") app.alternarFavorito(id);
        else if (accion == "2") app.alternarPendiente(id);
        else if (accion == "3") app.alternarVista(id);
        else if (accion == "4") app.quitarDelHistorial(id);
        else { std::cout << "Opcion no valida.\n"; continue; }
        advertirGuardado();
    }
}

void mostrarPaginas(const std::vector<int>& resultados) {
    if (resultados.empty()) { std::cout << "No se encontraron peliculas.\n"; return; }
    auto& app = Catalogo::instancia();
    size_t pagina = 0;
    while (true) {
        const size_t inicio = pagina * 5;
        const size_t fin = std::min(inicio + 5, resultados.size());
        std::cout << "\n--- Resultados " << inicio + 1 << " a " << fin
                  << " de " << resultados.size() << " ---\n";
        for (size_t i = inicio; i < fin; ++i) {
            const auto* p = app.obtener(resultados[i]);
            if (!p) continue;
            std::cout << '[' << i - inicio + 1 << "] " << p->titulo << " (" << p->año << ") "
                      << (app.esFavorito(p->id) ? "<3 " : "")
                      << (app.esPendiente(p->id) ? "[Pendiente] " : "")
                      << (app.estaVista(p->id) ? "[Vista]" : "") << '\n';
        }
        std::cout << "[S] Siguiente   [A] Anterior   [1-5] Ver detalle   [0] Volver: ";
        std::string opcion;
        if (!std::getline(std::cin, opcion) || opcion == "0") return;
        if ((opcion == "s" || opcion == "S") && fin < resultados.size()) ++pagina;
        else if ((opcion == "a" || opcion == "A") && pagina > 0) --pagina;
        else if (opcion.size() == 1 && opcion[0] >= '1' && opcion[0] <= '5') {
            size_t pos = inicio + static_cast<size_t>(opcion[0] - '1');
            if (pos < fin) detallePelicula(resultados[pos]);
            else std::cout << "Esa posicion no esta en esta pagina.\n";
        } else std::cout << "Opcion no valida o no hay mas paginas.\n";
    }
}

void comparar(const std::string& consulta) {
    auto& app = Catalogo::instancia();
    struct Fila { const char* nombre; ModoBusqueda modo; };
    const Fila modos[] = {{"Lineal", ModoBusqueda::Lineal},
                          {"Paralela", ModoBusqueda::Paralela},
                          {"Trie de caracteres", ModoBusqueda::Trie}};
    std::vector<int> referencia;
    std::cout << "\nComparacion para: " << consulta << '\n';
    for (const auto& f : modos) {
        auto inicio = Reloj::now();
        auto resultado = app.buscar(consulta, f.modo);
        auto fin = Reloj::now();
        const auto ms = std::chrono::duration<double, std::milli>(fin - inicio).count();
        std::cout << std::left << std::setw(18) << f.nombre
                  << "  " << std::fixed << std::setprecision(3) << ms
                  << " ms | " << resultado.size() << " resultados";
        if (referencia.empty() && f.modo == ModoBusqueda::Lineal) referencia = resultado;
        else std::cout << (referencia == resultado ? " | Coinciden" : " | REVISAR: resultados diferentes");
        std::cout << '\n';
    }
    std::cout << "Los tiempos dependen de la consulta y de tu computadora.\n";
}

std::string localizarCSV(int argc, char** argv) {
    if (argc > 1) return argv[1];
    std::vector<fs::path> candidatos;
    const fs::path cwd = fs::current_path();
    for (auto raiz : {cwd, cwd.parent_path(), cwd.parent_path().parent_path()}) {
        candidatos.push_back(raiz / "data/wiki_movie_plots_deduped.csv");
        candidatos.push_back(raiz / "wiki_movie_plots_deduped.csv");
    }
    if (argc > 0) {
        const fs::path exe = fs::absolute(argv[0]).parent_path();
        for (auto raiz : {exe, exe.parent_path(), exe.parent_path().parent_path()}) {
            candidatos.push_back(raiz / "data/wiki_movie_plots_deduped.csv");
            candidatos.push_back(raiz / "wiki_movie_plots_deduped.csv");
        }
    }
    for (const auto& candidato : candidatos)
        if (fs::exists(candidato)) return candidato.string();
    return "data/wiki_movie_plots_deduped.csv";
}

int main(int argc, char** argv) {
    auto& app = Catalogo::instancia();
    const std::string ruta = localizarCSV(argc, argv);
    std::cout << "Cargando CSV: " << ruta << '\n';
    const auto inicio = Reloj::now();
    if (!app.cargar(ruta)) {
        std::cerr << "No se pudo cargar el CSV o el archivo de perfil.\n"
                  << "Copia el CSV en data/ o pasalo como argumento: CineVerse ruta/datos.csv\n";
        return 1;
    }
    std::cout << "Peliculas: " << app.peliculas().size()
              << " | Palabras: " << app.totalPalabras()
              << " | Nodos del Trie: " << app.totalNodos()
              << " | Carga: " << std::fixed << std::setprecision(2)
              << std::chrono::duration<double>(Reloj::now() - inicio).count() << " s\n"
              << "Perfil que se guarda automaticamente en: " << app.rutaArchivoPerfil().string() << '\n';
    // Requisito de la primera parte: mostrar estas secciones al iniciar.
    const auto pendientesIniciales = app.lista(false);
    std::cout << "\n=== VER MAS TARDE (" << pendientesIniciales.size() << ") ===\n";
    if (pendientesIniciales.empty()) std::cout << "Aun no agregaste peliculas.\n";
    for (size_t i = 0; i < std::min(size_t(5), pendientesIniciales.size()); ++i)
        if (const auto* p = app.obtener(pendientesIniciales[i]))
            std::cout << " - " << p->titulo << " (" << p->año << ")\n";
    const auto sugerenciasIniciales = app.recomendar(5);
    std::cout << "\n=== RECOMENDACIONES SEGUN TUS ME GUSTA ===\n";
    if (sugerenciasIniciales.empty()) std::cout << "Marca Me gusta en peliculas para recibir sugerencias.\n";
    for (int id : sugerenciasIniciales)
        if (const auto* p = app.obtener(id)) std::cout << " - " << p->titulo << " (" << p->genero << ")\n";

    while (true) {
        menu();
        std::string opcion;
        if (!std::getline(std::cin, opcion) || opcion == "3") break;
        if (opcion == "1") {
            std::cout << "Palabra o frase: ";
            std::string texto;
            if (!std::getline(std::cin, texto)) break;
            std::cout << "Campo [1] Titulo/Sinopsis [2] Director [3] Elenco [4] Genero [5] Origen: ";
            std::string campo;
            if (!std::getline(std::cin, campo)) break;
            CampoBusqueda filtro = campo == "2" ? CampoBusqueda::Director :
                                   campo == "3" ? CampoBusqueda::Elenco :
                                   campo == "4" ? CampoBusqueda::Genero :
                                   campo == "5" ? CampoBusqueda::Origen : CampoBusqueda::Texto;
            std::cout << "Metodo [1] Trie [2] Lineal [3] Paralela (ENTER = Trie): ";
            std::string modo;
            if (!std::getline(std::cin, modo)) break;
            ModoBusqueda elegido = modo == "2" ? ModoBusqueda::Lineal :
                                  modo == "3" ? ModoBusqueda::Paralela : ModoBusqueda::Trie;
            mostrarPaginas(filtro == CampoBusqueda::Texto ? app.buscar(texto, elegido) :
                           app.buscarPorTag(texto, filtro, elegido));
        } else if (opcion == "2") mostrarPaginas(app.lista(false));
        else if (opcion == "4") mostrarPaginas(app.lista(true));
        else if (opcion == "5") {
            std::cout << "Consulta para medir: ";
            std::string texto;
            if (!std::getline(std::cin, texto)) break;
            comparar(texto);
        } else if (opcion == "6") {
            std::cout << "Peliculas: " << app.peliculas().size()
                      << " | Terminos unicos: " << app.totalPalabras()
                      << " | Nodos de caracteres: " << app.totalNodos() << '\n';
        } else if (opcion == "7") mostrarPaginas(app.historial());
        else if (opcion == "8") mostrarPaginas(app.listaVistas());
        else if (opcion == "9") {
            std::vector<int> ids;
            ids.reserve(app.peliculas().size());
            for (const auto& p : app.peliculas()) ids.push_back(p.id);
            mostrarPaginas(ids);
        } else if (opcion == "10") {
            std::cout << "Me gusta: " << app.lista(true).size()
                      << " | Ver mas tarde: " << app.lista(false).size()
                      << " | Ya vistas: " << app.listaVistas().size()
                      << " | Historial: " << app.historial().size() << " peliculas\n";
        } else if (opcion == "11") {
            std::cout << "Se borra el historial, pero NO Me gusta ni Ver mas tarde. Escribe SI: ";
            std::string confirmar;
            if (!std::getline(std::cin, confirmar)) break;
            if (confirmar == "SI") { app.borrarHistorial(); advertirGuardado(); }
        } else if (opcion == "12") {
            std::cout << "Recomendaciones basadas en los generos de tus Likes y Ver mas tarde:\n";
            mostrarPaginas(app.recomendar(30));
        } else std::cout << "Opcion no valida.\n";
    }
    std::cout << "Hasta pronto. Tus listas quedan guardadas.\n";
    return 0;
}
