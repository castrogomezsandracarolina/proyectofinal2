#pragma once
#include "IndiceSufijos.h"
#include "Texto.h"
#include <algorithm>
#include <future>
#include <iterator>
#include <memory>
#include <string>
#include <thread>
#include <vector>

enum class ModoBusqueda { Trie, Lineal, Paralela };

struct DatosBusqueda {
    const IndiceSufijos& indice;
    const std::vector<PeliculaNormalizada>& textos;
    unsigned hilos = 0;
    CampoBusqueda campo = CampoBusqueda::Texto;
};

// Una frase A B devuelve A O B (lo solicita expresamente el enunciado).
// Esta funcion es la semantica compartida por las tres estrategias.
inline bool coincide(const PeliculaNormalizada& p, const std::vector<std::string>& palabras,
                     CampoBusqueda campo) {
    if (campo == CampoBusqueda::Texto) {
        for (const auto& q : palabras)
            if (p.titulo.find(q) != std::string::npos || p.sinopsis.find(q) != std::string::npos)
                return true;
    } else {
        const auto& texto = p.campo(campo);
        for (const auto& q : palabras) if (texto.find(q) != std::string::npos) return true;
    }
    return false;
}

// Strategy: la plataforma intercambia motores sin cambiar la interfaz.
class EstrategiaBusqueda {
public:
    virtual ~EstrategiaBusqueda() = default;
    virtual std::vector<int> buscar(const std::string& consulta) const = 0;
};
class BusquedaLineal final : public EstrategiaBusqueda {
    const std::vector<PeliculaNormalizada>& textos;
    CampoBusqueda campo;
public:
    explicit BusquedaLineal(const std::vector<PeliculaNormalizada>& datos,
                            CampoBusqueda c = CampoBusqueda::Texto) : textos(datos), campo(c) {}
    std::vector<int> buscar(const std::string& q) const override {
        const auto palabras = Texto::palabras(q);
        std::vector<int> resultado;
        if (palabras.empty()) return resultado;
        for (std::size_t i = 0; i < textos.size(); ++i)
            if (coincide(textos[i], palabras, campo)) resultado.push_back(static_cast<int>(i + 1));
        return resultado;
    }
};
class BusquedaTrie final : public EstrategiaBusqueda {
    const IndiceSufijos& indice;
    CampoBusqueda campo;
public:
    BusquedaTrie(const IndiceSufijos& arbol, const std::vector<PeliculaNormalizada>&,
                 CampoBusqueda c = CampoBusqueda::Texto) : indice(arbol), campo(c) {}
    std::vector<int> buscar(const std::string& q) const override {
        const auto palabras = Texto::palabras(q);
        std::vector<int> todos;
        for (const auto& palabra : palabras) {
            const auto parte = indice.buscar(palabra, campo); // Busqueda REAL por el arbol.
            std::vector<int> unidos;
            std::set_union(todos.begin(), todos.end(), parte.begin(), parte.end(),
                           std::back_inserter(unidos));
            todos = std::move(unidos);
        }
        return todos;
    }
};
class BusquedaParalela final : public EstrategiaBusqueda {
    const std::vector<PeliculaNormalizada>& textos;
    unsigned hilos;
    CampoBusqueda campo;
public:
    BusquedaParalela(const std::vector<PeliculaNormalizada>& datos, unsigned cantidad,
                     CampoBusqueda c = CampoBusqueda::Texto) : textos(datos), hilos(cantidad), campo(c) {}
    std::vector<int> buscar(const std::string& q) const override {
        const auto palabras = Texto::palabras(q);
        if (palabras.empty()) return {};
        unsigned n = hilos ? hilos : std::thread::hardware_concurrency();
        n = std::clamp(n, 1u, 8u);
        n = std::min(n, static_cast<unsigned>(std::max(std::size_t(1), textos.size())));
        if (n == 1 || textos.size() < 100) return BusquedaLineal(textos, campo).buscar(q);
        std::vector<std::future<std::vector<int>>> tareas;
        for (unsigned i = 0; i < n; ++i) {
            const auto inicio = textos.size() * i / n;
            const auto fin = textos.size() * (i + 1) / n;
            tareas.emplace_back(std::async(std::launch::async, [this, &palabras, inicio, fin] {
                std::vector<int> encontrados;
                for (std::size_t j = inicio; j < fin; ++j)
                    if (coincide(textos[j], palabras, campo)) encontrados.push_back(static_cast<int>(j + 1));
                return encontrados;
            }));
        }
        std::vector<int> resultado;
        for (auto& tarea : tareas) {
            auto grupo = tarea.get();
            resultado.insert(resultado.end(), grupo.begin(), grupo.end());
        }
        return resultado;
    }
};

// Factory Method: cada creador concreto implementa el metodo virtual crear().
class CreadorBusqueda {
public:
    virtual ~CreadorBusqueda() = default;
    virtual std::unique_ptr<EstrategiaBusqueda> crear(const DatosBusqueda& d) const = 0;
    std::vector<int> ejecutar(const DatosBusqueda& d, const std::string& q) const {
        return crear(d)->buscar(q);
    }
};
class CreadorTrie final : public CreadorBusqueda {
public:
    std::unique_ptr<EstrategiaBusqueda> crear(const DatosBusqueda& d) const override {
        return std::make_unique<BusquedaTrie>(d.indice, d.textos, d.campo);
    }
};
class CreadorLineal final : public CreadorBusqueda {
public:
    std::unique_ptr<EstrategiaBusqueda> crear(const DatosBusqueda& d) const override {
        return std::make_unique<BusquedaLineal>(d.textos, d.campo);
    }
};
class CreadorParalelo final : public CreadorBusqueda {
public:
    std::unique_ptr<EstrategiaBusqueda> crear(const DatosBusqueda& d) const override {
        return std::make_unique<BusquedaParalela>(d.textos, d.hilos, d.campo);
    }
};
