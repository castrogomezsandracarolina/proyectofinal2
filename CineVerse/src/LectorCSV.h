#pragma once
#include "Pelicula.h"
#include "Texto.h"
#include <fstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class LectorCSV {
    static bool leerFila(std::istream& archivo, std::vector<std::string>& fila) {
        fila.clear();
        std::string campo;
        bool dentro = false, hayDatos = false;
        char c;
        while (archivo.get(c)) {
            hayDatos = true;
            if (c == '"') {
                if (dentro && archivo.peek() == '"') { archivo.get(c); campo += '"'; }
                else dentro = !dentro;
            } else if (c == ',' && !dentro) {
                fila.push_back(std::move(campo)); campo.clear();
            } else if ((c == '\n' || c == '\r') && !dentro) {
                if (c == '\r' && archivo.peek() == '\n') archivo.get(c);
                fila.push_back(std::move(campo));
                return true;
            } else campo += c;
        }
        if (dentro) return false; // Fila incompleta o comillas sin cerrar.
        if (!hayDatos && fila.empty() && campo.empty()) return false;
        fila.push_back(std::move(campo));
        return true;
    }

public:
    static std::vector<Pelicula> leer_peliculas(const std::string& ruta) {
        std::ifstream archivo(ruta, std::ios::binary);
        std::vector<Pelicula> peliculas;
        if (!archivo) return peliculas;
        std::vector<std::string> fila;
        if (!leerFila(archivo, fila)) return peliculas;
        if (!fila.empty() && fila[0].size() >= 3 &&
            static_cast<unsigned char>(fila[0][0]) == 0xEF &&
            static_cast<unsigned char>(fila[0][1]) == 0xBB &&
            static_cast<unsigned char>(fila[0][2]) == 0xBF) fila[0].erase(0, 3);
        std::unordered_map<std::string, size_t> columnas;
        for (size_t i = 0; i < fila.size(); ++i) {
            std::string nombre = Texto::normalizar(fila[i]);
            std::string clave;
            for (char c : nombre) if (c != ' ') clave += c;
            columnas[clave] = i;
        }
        const char* nombres[] = {"releaseyear", "title", "originethnicity", "director", "cast", "genre", "wikipage", "plot"};
        size_t posiciones[8]{};
        for (size_t i = 0; i < 8; ++i) {
            auto it = columnas.find(nombres[i]);
            if (it == columnas.end()) return peliculas;
            posiciones[i] = it->second;
        }
        int id = 1;
        while (leerFila(archivo, fila)) {
            if (fila.size() == 1 && fila[0].empty()) continue;
            bool completa = true;
            for (size_t pos : posiciones) if (pos >= fila.size()) { completa = false; break; }
            if (!completa) continue;
            Pelicula p;
            p.id = id++;
            p.año = std::move(fila[posiciones[0]]);
            p.titulo = std::move(fila[posiciones[1]]);
            p.origen = std::move(fila[posiciones[2]]);
            p.director = std::move(fila[posiciones[3]]);
            p.elenco = std::move(fila[posiciones[4]]);
            p.genero = std::move(fila[posiciones[5]]);
            p.link_wiki = std::move(fila[posiciones[6]]);
            p.sinopsis = std::move(fila[posiciones[7]]);
            if (!p.titulo.empty()) peliculas.push_back(std::move(p));
        }
        // Los identificadores deben coincidir con la posicion en el vector.
        for (size_t i = 0; i < peliculas.size(); ++i) peliculas[i].id = static_cast<int>(i + 1);
        return peliculas;
    }
};
