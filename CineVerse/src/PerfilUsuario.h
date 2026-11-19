#pragma once
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

// Estado persistente de cada pelicula: no depende del orden de las filas del CSV.
struct EstadoPelicula {
    bool meGusta = false;
    bool verMasTarde = false;
    bool yaVista = false;
    std::uint64_t vecesAbierta = 0;
    std::int64_t ultimaApertura = 0;
};

class PerfilUsuario {
    std::filesystem::path archivo;
    std::unordered_map<std::string, EstadoPelicula> estados;

public:
    static std::filesystem::path rutaPorDefecto() {
#ifdef _WIN32
        if (const char* p = std::getenv("LOCALAPPDATA"))
            return std::filesystem::path(p) / "CineVerse" / "perfil_cv2.txt";
        if (const char* p = std::getenv("APPDATA"))
            return std::filesystem::path(p) / "CineVerse" / "perfil_cv2.txt";
#else
        if (const char* p = std::getenv("XDG_DATA_HOME"))
            return std::filesystem::path(p) / "cineverse" / "perfil_cv2.txt";
#endif
        if (const char* p = std::getenv("USERPROFILE"))
            return std::filesystem::path(p) / ".cineverse" / "perfil_cv2.txt";
        if (const char* p = std::getenv("HOME"))
            return std::filesystem::path(p) / ".local" / "share" / "cineverse" / "perfil_cv2.txt";
        return std::filesystem::path("perfil_cv2.txt");
    }

    PerfilUsuario() : archivo(rutaPorDefecto()) {}
    void establecerRuta(const std::filesystem::path& ruta) { archivo = ruta; }
    const std::filesystem::path& ruta() const { return archivo; }
    const std::unordered_map<std::string, EstadoPelicula>& todos() const { return estados; }
    void limpiarMemoria() { estados.clear(); }
    const EstadoPelicula& consultar(const std::string& clave) const {
        static const EstadoPelicula vacio;
        auto it = estados.find(clave);
        return it == estados.end() ? vacio : it->second;
    }
    EstadoPelicula& editar(const std::string& clave) { return estados[clave]; }

    bool cargar() {
        estados.clear();
        std::ifstream f(archivo, std::ios::binary);
        if (!f) {
            // Si Windows se cerro entre mover el viejo perfil a .bak y
            // completar el reemplazo, recuperar la copia anterior.
            auto respaldo = archivo;
            respaldo += ".bak";
            std::error_code ec;
            if (std::filesystem::exists(respaldo)) {
                std::filesystem::copy_file(respaldo, archivo,
                    std::filesystem::copy_options::overwrite_existing, ec);
                if (!ec) f.open(archivo, std::ios::binary);
            }
            if (!f) return !std::filesystem::exists(archivo) && !std::filesystem::exists(respaldo);
        }
        std::string cabecera;
        if (!(f >> cabecera) || cabecera != "CINEVERSE_PERFIL_2") return false;
        std::string linea;
        std::getline(f, linea);
        while (std::getline(f, linea)) {
            if (linea.empty()) continue;
            std::istringstream entrada(linea);
            std::string clave;
            int like, pendiente, vista;
            EstadoPelicula e;
            if (!(entrada >> std::quoted(clave) >> like >> pendiente >> vista
                  >> e.vecesAbierta >> e.ultimaApertura)) continue;
            if (clave.empty() || clave.size() > 200 || like < 0 || like > 1 || pendiente < 0 || pendiente > 1 || vista < 0 || vista > 1) continue;
            e.meGusta = like == 1;
            e.verMasTarde = pendiente == 1;
            e.yaVista = vista == 1;
            estados[std::move(clave)] = e;
        }
        return true;
    }

    bool guardar() const {
        std::error_code ec;
        if (archivo.has_parent_path()) {
            std::filesystem::create_directories(archivo.parent_path(), ec);
            if (ec) return false;
        }
        auto temporal = archivo;
        temporal += ".tmp";
        {
            std::ofstream f(temporal, std::ios::binary | std::ios::trunc);
            if (!f) return false;
            f << "CINEVERSE_PERFIL_2\n";
            for (const auto& [clave, e] : estados) {
                if (!e.meGusta && !e.verMasTarde && !e.yaVista && !e.vecesAbierta) continue;
                f << std::quoted(clave) << '\t' << int(e.meGusta) << '\t' << int(e.verMasTarde)
                  << '\t' << int(e.yaVista) << '\t' << e.vecesAbierta << '\t'
                  << e.ultimaApertura << '\n';
            }
            f.flush();
            if (!f.good()) return false;
        }
        std::filesystem::rename(temporal, archivo, ec); // Reemplazo atomico en Linux.
        if (!ec) return true;
        //En Windows rename() no reemplaza un destino existente. Conservamos copia.
        auto respaldo = archivo;
        respaldo += ".bak";
        ec.clear();
        std::filesystem::remove(respaldo, ec);
        ec.clear();
        if (std::filesystem::exists(archivo)) {
            std::filesystem::rename(archivo, respaldo, ec);
            if (ec) return false;
        }
        ec.clear();
        std::filesystem::rename(temporal, archivo, ec);
        if (ec) {
            std::error_code errorRestaurar;
            if (std::filesystem::exists(respaldo)) std::filesystem::rename(respaldo, archivo, errorRestaurar);
            return false;
        }
        std::filesystem::remove(respaldo, ec);
        return true;
    }
};
