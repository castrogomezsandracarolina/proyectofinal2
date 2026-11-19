#pragma once
#include "Catalogo.h"
#include <string>
#include <vector>

// Conserva las funciones publicas de la primera entrega.
// Internamente ahora delega las operaciones al Catalogo con Trie y patrones.
class Plataforma {
public:
    void cargar_datos(const std::string& ruta_csv) { Catalogo::instancia().cargar(ruta_csv); }
    int total_peliculas() const { return static_cast<int>(Catalogo::instancia().peliculas().size()); }
    std::vector<Pelicula*> buscar(const std::string& texto) {
        return convertir(Catalogo::instancia().buscar(texto));
    }
    std::vector<Pelicula*> buscar_paralelo(const std::string& texto) {
        return convertir(Catalogo::instancia().buscarParalelo(texto));
    }
    void dar_like(Pelicula* p) { if (p && !Catalogo::instancia().esFavorito(p->id)) Catalogo::instancia().alternarFavorito(p->id); }
    void agregar_ver_mas_tarde(Pelicula* p) { if (p && !Catalogo::instancia().esPendiente(p->id)) Catalogo::instancia().alternarPendiente(p->id); }
    std::vector<Pelicula*> obtener_likes() { return convertir(Catalogo::instancia().lista(true)); }
    std::vector<Pelicula*> obtener_ver_mas_tarde() { return convertir(Catalogo::instancia().lista(false)); }
private:
    static std::vector<Pelicula*> convertir(const std::vector<int>& ids) {
        std::vector<Pelicula*> resultado;
        for (int id : ids) {
            // La interfaz original trabaja con punteros: sus peliculas son
            // estables mientras no se cargue otro archivo CSV.
            if (const Pelicula* p = Catalogo::instancia().obtener(id))
                resultado.push_back(const_cast<Pelicula*>(p));
        }
        return resultado;
    }
};
