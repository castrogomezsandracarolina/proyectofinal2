#pragma once
#include "Pelicula.h"
#include "Texto.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// Campos visibles de busqueda. Texto busca titulo O sinopsis.
enum class CampoBusqueda { Texto, Director, Elenco, Genero, Origen };

struct PeliculaNormalizada {
    std::string titulo, sinopsis, director, elenco, genero, origen;
    const std::string& campo(CampoBusqueda c) const {
        switch (c) {
            case CampoBusqueda::Director: return director;
            case CampoBusqueda::Elenco: return elenco;
            case CampoBusqueda::Genero: return genero;
            case CampoBusqueda::Origen: return origen;
            default: return titulo;
        }
    }
};

// Suffix Trie de CARACTERES: cada nodo, salvo la raiz, almacena UNA letra o un numero.
// Se indexan los sufijos de cada PALABRA unica, no cada caracter de cada sinopsis.
// Los terminales apuntan a palabras del vocabulario; sus postings apuntan a peliculas.
// child/sibling evitan el gran gasto de 36 punteros por nodo.
class IndiceSufijos {
    enum : int { Titulo = 0, Sinopsis = 1, Director = 2, Elenco = 3, Genero = 4, Origen = 5 };
    struct Termino {
        std::string palabra;
        std::array<std::vector<int>, 6> apariciones;
    };
    struct Nodo {
        int hijo = -1, hermano = -1, primerFinal = -1;
        char caracter = '\0';
    };
    struct Final { int termino, siguiente; };
    std::vector<Termino> vocabulario;
    std::vector<Nodo> nodos{1};
    std::vector<Final> finales;

    int obtenerOCrearHijo(int padre, char letra) {
        for (int n = nodos[padre].hijo; n != -1; n = nodos[n].hermano)
            if (nodos[n].caracter == letra) return n;
        const int indice = static_cast<int>(nodos.size());
        const int previo = nodos[padre].hijo;
        nodos.emplace_back();
        nodos[indice].caracter = letra;
        nodos[indice].hermano = previo;
        nodos[padre].hijo = indice;
        return indice;
    }
    void insertarSufijo(int idTermino, std::size_t desde) {
        const std::string& palabra = vocabulario[idTermino].palabra;
        int nodo = 0;
        for (std::size_t j = desde; j < palabra.size(); ++j)
            nodo = obtenerOCrearHijo(nodo, palabra[j]);
        const int fin = static_cast<int>(finales.size());
        finales.push_back({idTermino, nodos[nodo].primerFinal});
        nodos[nodo].primerFinal = fin;
    }
    static int indiceCampo(CampoBusqueda c) {
        switch (c) {
            case CampoBusqueda::Director: return Director;
            case CampoBusqueda::Elenco: return Elenco;
            case CampoBusqueda::Genero: return Genero;
            case CampoBusqueda::Origen: return Origen;
            default: return Titulo;
        }
    }
public:
    void construir(const std::vector<Pelicula>& peliculas, std::vector<PeliculaNormalizada>& cache) {
        vocabulario.clear(); nodos.clear(); nodos.emplace_back(); finales.clear();
        cache.clear(); cache.reserve(peliculas.size());
        std::unordered_map<std::string, int> diccionario;
        // Reducimos memoria: el vocabulario conserva una sola copia de cada palabra.
        for (const auto& p : peliculas) {
            PeliculaNormalizada t{Texto::normalizar(p.titulo), Texto::normalizar(p.sinopsis),
                Texto::normalizar(p.director), Texto::normalizar(p.elenco),
                Texto::normalizar(p.genero), Texto::normalizar(p.origen)};
            const std::array<const std::string*, 6> campos = {&t.titulo, &t.sinopsis, &t.director,
                                                               &t.elenco, &t.genero, &t.origen};
            for (int campo = 0; campo < 6; ++campo) {
                std::unordered_set<std::string> unicas;
                for (auto& palabra : Texto::palabras(*campos[campo])) unicas.insert(std::move(palabra));
                for (const auto& palabra : unicas) {
                    auto [it, nueva] = diccionario.try_emplace(palabra, static_cast<int>(vocabulario.size()));
                    if (nueva) vocabulario.push_back({palabra, {}});
                    vocabulario[it->second].apariciones[campo].push_back(p.id);
                }
            }
            cache.push_back(std::move(t));
        }
        diccionario.clear(); diccionario.rehash(0);
        // Cada terminacion de una palabra tiene su propia ruta de letras individuales.
        for (int id = 0; id < static_cast<int>(vocabulario.size()); ++id) {
            const std::string& palabra = vocabulario[id].palabra;
            for (std::size_t pos = 0; pos < palabra.size(); ++pos) insertarSufijo(id, pos);
        }
    }
    std::vector<int> buscar(const std::string& subpalabra,
                            CampoBusqueda campo = CampoBusqueda::Texto) const {
        if (subpalabra.empty() || subpalabra.find(' ') != std::string::npos) return {};
        int nodo = 0;
        for (const char letra : subpalabra) {
            int encontrado = -1;
            for (int h = nodos[nodo].hijo; h != -1; h = nodos[h].hermano) {
                if (nodos[h].caracter == letra) { encontrado = h; break; }
            }
            if (encontrado == -1) return {};
            nodo = encontrado;
        }
        std::vector<int> resultado, pendientes{nodo};
        while (!pendientes.empty()) {
            const int actual = pendientes.back(); pendientes.pop_back();
            for (int f = nodos[actual].primerFinal; f != -1; f = finales[f].siguiente) {
                const auto& p = vocabulario[finales[f].termino].apariciones;
                auto agregar = [&resultado](const std::vector<int>& ids) {
                    resultado.insert(resultado.end(), ids.begin(), ids.end());
                };
                if (campo == CampoBusqueda::Texto) { agregar(p[Titulo]); agregar(p[Sinopsis]); }
                else agregar(p[indiceCampo(campo)]);
            }
            for (int hijo = nodos[actual].hijo; hijo != -1; hijo = nodos[hijo].hermano)
                pendientes.push_back(hijo);
        }
        std::sort(resultado.begin(), resultado.end());
        resultado.erase(std::unique(resultado.begin(), resultado.end()), resultado.end());
        return resultado;
    }
    std::size_t totalPalabras() const { return vocabulario.size(); }
    std::size_t totalNodos() const { return nodos.size(); }
};
