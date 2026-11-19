#pragma once
#include "Busquedas.h"
#include "LectorCSV.h"
#include "Pelicula.h"
#include "PerfilUsuario.h"
#include "Texto.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// Observer: al cambiar el perfil o cargar peliculas se avisa a las pantallas.
class Observador {
public:
    virtual ~Observador() = default;
    virtual void actualizar() = 0;
};

// Singleton: un catalogo compartido por terminal y, despues, interfaz Qt.
class Catalogo {
    std::vector<Pelicula> datos;
    std::vector<PeliculaNormalizada> normalizados;
    IndiceSufijos indice;
    std::vector<std::string> claves;
    PerfilUsuario perfil;
    std::vector<Observador*> observadores;
    bool ultimoGuardado = true;
    bool rutaPersonalizada = false;

    Catalogo() = default;

    static std::string clavePelicula(const Pelicula& p) {
        // Dos hashes de 64 bits separados: claves estables aunque se reordene el CSV.
        std::uint64_t a = 14695981039346656037ULL;
        std::uint64_t b = 7809847782465536322ULL;
        auto acumular = [&](const std::string& valor) {
            for (unsigned char c : valor) {
                a = (a ^ c) * 1099511628211ULL;
                b = (b ^ (c + 1U)) * 1099511628211ULL;
            }
            a = (a ^ 0xFFU) * 1099511628211ULL;
            b = (b ^ 0xFEU) * 1099511628211ULL;
        };
        acumular(p.link_wiki);
        acumular(p.año);
        acumular(p.titulo);
        acumular(p.origen);
        acumular(p.director);
        std::ostringstream salida;
        salida << std::hex << std::setfill('0') << std::setw(16) << a << std::setw(16) << b;
        return salida.str();
    }

    void notificar() {
        auto copia = observadores;
        for (auto* o : copia)
            if (o && std::find(observadores.begin(), observadores.end(), o) != observadores.end())
                o->actualizar();
    }
    bool guardarYNotificar() {
        ultimoGuardado = perfil.guardar();
        notificar();
        return ultimoGuardado;
    }
    bool valido(int id) const { return id > 0 && static_cast<std::size_t>(id) <= claves.size(); }
    const EstadoPelicula& estado(int id) const {
        static const EstadoPelicula vacio;
        return valido(id) ? perfil.consultar(claves[static_cast<size_t>(id - 1)]) : vacio;
    }
    EstadoPelicula& editar(int id) { return perfil.editar(claves[static_cast<size_t>(id - 1)]); }

    // Importa el formato de favoritos de la version anterior, solo si todavia
    // no existe el perfil nuevo y el CSV es identico (firma CINEVERSE1).
    void importarListasAnteriores() {
        if (rutaPersonalizada || std::filesystem::exists(perfil.ruta())) return;
        std::ifstream f("cineverse_listas.txt");
        if (!f) return;
        std::uint64_t firma = 14695981039346656037ULL;
        auto procesar = [&firma](const std::string& s) {
            for (unsigned char c : s) { firma ^= c; firma *= 1099511628211ULL; }
            firma ^= 0xFF; firma *= 1099511628211ULL;
        };
        for (const auto& p : datos) { procesar(p.titulo); procesar(p.año); procesar(p.director); }
        std::string version;
        std::uint64_t firmaArchivo;
        if (!(f >> version >> firmaArchivo) || version != "CINEVERSE1" || firmaArchivo != firma) return;
        char tipo;
        int id;
        bool cambios = false;
        while (f >> tipo >> id) {
            if (!valido(id)) continue;
            if (tipo == 'F') { editar(id).meGusta = true; cambios = true; }
            if (tipo == 'P') { editar(id).verMasTarde = true; cambios = true; }
        }
        if (cambios) ultimoGuardado = perfil.guardar();
    }

    // Ranking propio: mas terminos hallados, titulo antes que sinopsis,
    // coincidencia exacta antes que subpalabra, y frase completa como bono.
    static int puntaje(const PeliculaNormalizada& p, const std::string& q,
                       CampoBusqueda campo) {
        const auto palabras = Texto::palabras(q);
        int valor = 0;
        if (campo == CampoBusqueda::Texto) {
            for (const auto& palabra : palabras) {
                if (p.titulo.find(palabra) != std::string::npos) {
                    valor += 120;
                    if (Texto::contienePalabra(p.titulo, palabra)) valor += 35;
                }
                if (p.sinopsis.find(palabra) != std::string::npos) {
                    valor += 18;
                    if (Texto::contienePalabra(p.sinopsis, palabra)) valor += 8;
                }
            }
            if (p.titulo == q) valor += 450;
            else if (p.titulo.find(q) != std::string::npos) valor += 160;
            if (p.sinopsis.find(q) != std::string::npos) valor += 40;
        } else {
            const auto& texto = p.campo(campo);
            for (const auto& palabra : palabras) {
                if (texto.find(palabra) != std::string::npos) {
                    valor += 50;
                    if (Texto::contienePalabra(texto, palabra)) valor += 25;
                }
            }
            if (texto == q) valor += 250;
            else if (texto.find(q) != std::string::npos) valor += 120;
        }
        return valor;
    }

    std::vector<int> buscarInterno(const std::string& texto, ModoBusqueda modo,
                                   CampoBusqueda campo, unsigned hilos) const {
        const std::string q = Texto::normalizar(texto);
        if (Texto::palabras(q).empty() || datos.empty()) return {};
        DatosBusqueda contexto{indice, normalizados, hilos, campo};
        std::unique_ptr<CreadorBusqueda> creador;
        switch (modo) {
            case ModoBusqueda::Trie: creador = std::make_unique<CreadorTrie>(); break;
            case ModoBusqueda::Lineal: creador = std::make_unique<CreadorLineal>(); break;
            case ModoBusqueda::Paralela: creador = std::make_unique<CreadorParalelo>(); break;
        }
        std::vector<int> resultados = creador->ejecutar(contexto, q);
        // IMPORTANTE: calcular el ranking UNA VEZ por pelicula, no dentro del
        // comparador de sort (en consultas comunes reduce drasticamente el tiempo).
        std::vector<std::pair<int, int>> valorados;
        valorados.reserve(resultados.size());
        for (int id : resultados) valorados.emplace_back(puntaje(normalizados[id - 1], q, campo), id);
        std::sort(valorados.begin(), valorados.end(), [](const auto& a, const auto& b) {
            return a.first != b.first ? a.first > b.first : a.second < b.second;
        });
        resultados.clear();
        resultados.reserve(valorados.size());
        for (const auto& [puntos, id] : valorados) resultados.push_back(id);
        return resultados;
    }

public:
    static Catalogo& instancia() { static Catalogo c; return c; }
    Catalogo(const Catalogo&) = delete;
    Catalogo& operator=(const Catalogo&) = delete;

    bool cargar(const std::string& ruta) {
        auto nuevo = LectorCSV::leer_peliculas(ruta);
        if (nuevo.empty()) return false;
        std::vector<PeliculaNormalizada> cache;
        IndiceSufijos arbol;
        arbol.construir(nuevo, cache);
        std::vector<std::string> nuevasClaves;
        nuevasClaves.reserve(nuevo.size());
        for (const auto& p : nuevo) nuevasClaves.push_back(clavePelicula(p));
        if (!perfil.cargar()) return false;
        datos = std::move(nuevo);
        normalizados = std::move(cache);
        indice = std::move(arbol);
        claves = std::move(nuevasClaves);
        ultimoGuardado = true;
        importarListasAnteriores();
        notificar();
        return true;
    }
    const std::vector<Pelicula>& peliculas() const { return datos; }
    const Pelicula* obtener(int id) const {
        return valido(id) ? &datos[static_cast<size_t>(id - 1)] : nullptr;
    }
    size_t totalPalabras() const { return indice.totalPalabras(); }
    size_t totalNodos() const { return indice.totalNodos(); }
    std::filesystem::path rutaArchivoPerfil() const { return perfil.ruta(); }
    bool perfilGuardadoCorrectamente() const { return ultimoGuardado; }
    // Conserva el nombre del metodo del ZIP previo para no romper su API.
    void establecerArchivoListas(const std::string& ruta) {
        perfil.establecerRuta(ruta);
        rutaPersonalizada = true;
    }

    std::vector<int> buscar(const std::string& texto, ModoBusqueda modo = ModoBusqueda::Trie,
                            unsigned hilos = 0) const {
        return buscarInterno(texto, modo, CampoBusqueda::Texto, hilos);
    }
    std::vector<int> buscar(const std::string& texto, bool usarTrie) const {
        return buscar(texto, usarTrie ? ModoBusqueda::Trie : ModoBusqueda::Lineal);
    }
    std::vector<int> buscarParalelo(const std::string& texto, unsigned hilos = 0) const {
        return buscar(texto, ModoBusqueda::Paralela, hilos);
    }
    std::vector<int> buscarPorTag(const std::string& texto, CampoBusqueda campo,
                                  ModoBusqueda modo = ModoBusqueda::Trie, unsigned hilos = 0) const {
        return buscarInterno(texto, modo, campo, hilos);
    }

    bool alternarFavorito(int id) {
        if (!valido(id)) return false;
        auto& e = editar(id);
        e.meGusta = !e.meGusta;
        bool nuevo = e.meGusta;
        guardarYNotificar();
        return nuevo;
    }
    bool alternarPendiente(int id) {
        if (!valido(id)) return false;
        auto& e = editar(id);
        e.verMasTarde = !e.verMasTarde;
        bool nuevo = e.verMasTarde;
        guardarYNotificar();
        return nuevo;
    }
    bool alternarVista(int id) {
        if (!valido(id)) return false;
        auto& e = editar(id);
        e.yaVista = !e.yaVista;
        bool nuevo = e.yaVista;
        guardarYNotificar();
        return nuevo;
    }
    bool registrarVisita(int id) {
        if (!valido(id)) return false;
        auto& e = editar(id);
        ++e.vecesAbierta;
        e.ultimaApertura = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        return guardarYNotificar();
    }
    bool quitarDelHistorial(int id) {
        if (!valido(id)) return false;
        auto& e = editar(id);
        e.vecesAbierta = 0;
        e.ultimaApertura = 0;
        return guardarYNotificar();
    }
    bool borrarHistorial() {
        for (const auto& clave : claves) {
            auto& e = perfil.editar(clave);
            e.vecesAbierta = 0;
            e.ultimaApertura = 0;
        }
        return guardarYNotificar();
    }
    bool esFavorito(int id) const { return estado(id).meGusta; }
    bool esPendiente(int id) const { return estado(id).verMasTarde; }
    bool estaVista(int id) const { return estado(id).yaVista; }
    std::uint64_t vecesAbierta(int id) const { return estado(id).vecesAbierta; }
    std::int64_t ultimaApertura(int id) const { return estado(id).ultimaApertura; }

    std::vector<int> lista(bool favoritos) const {
        std::vector<int> ids;
        for (size_t i = 0; i < claves.size(); ++i) {
            const auto& e = perfil.consultar(claves[i]);
            if (favoritos ? e.meGusta : e.verMasTarde) ids.push_back(static_cast<int>(i + 1));
        }
        return ids;
    }
    std::vector<int> listaVistas() const {
        std::vector<int> ids;
        for (size_t i = 0; i < claves.size(); ++i)
            if (perfil.consultar(claves[i]).yaVista) ids.push_back(static_cast<int>(i + 1));
        return ids;
    }
    std::vector<int> historial() const {
        std::vector<int> ids;
        for (size_t i = 0; i < claves.size(); ++i)
            if (perfil.consultar(claves[i]).vecesAbierta) ids.push_back(static_cast<int>(i + 1));
        std::sort(ids.begin(), ids.end(), [this](int a, int b) {
            auto x = ultimaApertura(a), y = ultimaApertura(b);
            return x != y ? x > y : a < b;
        });
        return ids;
    }
    // Algoritmo propio de recomendaciones basado EXCLUSIVAMENTE en los Likes.
    // Pondera coincidencias de genero, director y actores; excluye guardadas/vistas.
    std::vector<int> recomendar(std::size_t cantidad = 20) const {
        std::unordered_map<std::string, int> generosPreferidos, directoresPreferidos,
                                              actoresPreferidos, palabrasTrama;
        auto tokensGenero = [](const std::string& entrada) {
            std::unordered_set<std::string> distintos;
            for (const auto& palabra : Texto::palabras(Texto::normalizar(entrada)))
                if (palabra != "unknown" && palabra != "film" && palabra != "movie" &&
                    palabra != "and" && palabra != "na") distintos.insert(palabra);
            return distintos;
        };
        auto actores = [](const std::string& entrada) {
            std::unordered_set<std::string> distintos;
            std::istringstream f(entrada);
            std::string actor;
            while (std::getline(f, actor, ',')) {
                actor = Texto::normalizar(actor);
                if (!actor.empty() && actor != "unknown" && actor != "na") distintos.insert(actor);
            }
            return distintos;
        };
        std::size_t totalLikes = 0;
        for (const auto& p : datos) {
            if (!esFavorito(p.id)) continue;
            ++totalLikes;
            for (const auto& g : tokensGenero(p.genero)) ++generosPreferidos[g];
            const auto director = Texto::normalizar(p.director);
            if (!director.empty() && director != "unknown") ++directoresPreferidos[director];
            for (const auto& a : actores(p.elenco)) ++actoresPreferidos[a];
            // Si no hay metadatos (genre/director/cast desconocidos), la sinopsis
            // tambien sirve como fuente para recomendar contenido relacionado.
            static const std::unordered_set<std::string> comunes = {
                "after", "before", "their", "there", "which", "while", "where", "about",
                "because", "through", "another", "would", "could", "should", "people",
                "during", "later", "first", "other", "having", "without", "against",
                "between", "becomes", "finally", "years", "takes", "makes", "finds",
                "starts", "begins", "young", "story", "comes", "going", "being"
            };
            std::unordered_set<std::string> vistas;
            for (const auto& palabra : Texto::palabras(normalizados[p.id - 1].sinopsis))
                if (palabra.size() >= 6 && !comunes.count(palabra)) vistas.insert(palabra);
            for (const auto& palabra : vistas) ++palabrasTrama[palabra];
        }
        if (totalLikes == 0) return {};
        // Seleccionamos 20 terminos distintivos: frecuencia y longitud como aproximacion
        // sencilla de relevancia, sin utilizar librerias externas de recomendacion.
        std::vector<std::pair<std::string, int>> clavesTrama(palabrasTrama.begin(), palabrasTrama.end());
        std::sort(clavesTrama.begin(), clavesTrama.end(), [](const auto& a, const auto& b) {
            if (a.second != b.second) return a.second > b.second;
            if (a.first.size() != b.first.size()) return a.first.size() > b.first.size();
            return a.first < b.first;
        });
        if (clavesTrama.size() > 20) clavesTrama.resize(20);
        std::vector<std::pair<int, int>> valoradas;
        for (const auto& p : datos) {
            if (esFavorito(p.id) || esPendiente(p.id) || estaVista(p.id)) continue;
            int puntos = 0;
            for (const auto& g : tokensGenero(p.genero)) {
                if (const auto it = generosPreferidos.find(g); it != generosPreferidos.end())
                    puntos += 12 * it->second;
            }
            const auto director = Texto::normalizar(p.director);
            if (const auto it = directoresPreferidos.find(director); it != directoresPreferidos.end())
                puntos += 18 * it->second;
            for (const auto& actor : actores(p.elenco)) {
                if (const auto it = actoresPreferidos.find(actor); it != actoresPreferidos.end())
                    puntos += 7 * it->second;
            }
            int palabrasCoincidentes = 0;
            for (const auto& [palabra, frecuencia] : clavesTrama)
                if (normalizados[p.id - 1].sinopsis.find(palabra) != std::string::npos)
                    ++palabrasCoincidentes;
            puntos += std::min(24, palabrasCoincidentes * 3);
            if (puntos > 0) valoradas.emplace_back(puntos, p.id);
        }
        std::sort(valoradas.begin(), valoradas.end(), [](const auto& a, const auto& b) {
            return a.first != b.first ? a.first > b.first : a.second < b.second;
        });
        std::vector<int> resultado;
        for (std::size_t i = 0; i < cantidad && i < valoradas.size(); ++i)
            resultado.push_back(valoradas[i].second);
        return resultado;
    }

    void suscribir(Observador* o) {
        if (o && std::find(observadores.begin(), observadores.end(), o) == observadores.end())
            observadores.push_back(o);
    }
    void desuscribir(Observador* o) {
        observadores.erase(std::remove(observadores.begin(), observadores.end(), o), observadores.end());
    }
};
