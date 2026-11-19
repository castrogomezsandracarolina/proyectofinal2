#pragma once
#include <cctype>
#include <string>
#include <vector>

namespace Texto {
inline std::string normalizar(const std::string& texto) {
    std::string salida;
    salida.reserve(texto.size());
    for (size_t i = 0; i < texto.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(texto[i]);
        if (c == 0xC3 && i + 1 < texto.size()) {
            const unsigned char d = static_cast<unsigned char>(texto[i + 1]);
            char letra = 0;
            switch (d) {
                case 0x81: case 0xA1: case 0x80: case 0xA0: case 0x82: case 0xA2: letra = 'a'; break;
                case 0x89: case 0xA9: case 0x88: case 0xA8: case 0x8A: case 0xAA: letra = 'e'; break;
                case 0x8D: case 0xAD: case 0x8C: case 0xAC: letra = 'i'; break;
                case 0x93: case 0xB3: case 0x92: case 0xB2: case 0x94: case 0xB4: letra = 'o'; break;
                case 0x9A: case 0xBA: case 0x99: case 0xB9: case 0x9C: case 0xBC: letra = 'u'; break;
                case 0x91: case 0xB1: letra = 'n'; break;
                default: break;
            }
            if (letra) { salida += letra; ++i; continue; }
            // Otros caracteres UTF-8 se preservan sin alterar sus bytes.
            salida += static_cast<char>(c);
            salida += static_cast<char>(d);
            ++i;
            continue;
        }
        if (c < 128) {
            if (std::isalnum(c)) salida += static_cast<char>(std::tolower(c));
            else if (salida.empty() || salida.back() != ' ') salida += ' ';
        } else salida += static_cast<char>(c);
    }
    if (!salida.empty() && salida.back() == ' ') salida.pop_back();
    return salida;
}
inline std::vector<std::string> palabras(const std::string& normalizado) {
    std::vector<std::string> resultado;
    std::string actual;
    for (unsigned char c : normalizado) {
        if (std::isalnum(c) || c >= 128) actual += static_cast<char>(c);
        else if (!actual.empty()) { resultado.push_back(std::move(actual)); actual.clear(); }
    }
    if (!actual.empty()) resultado.push_back(std::move(actual));
    return resultado;
}
inline bool contienePalabra(const std::string& texto, const std::string& palabra) {
    if (palabra.empty()) return false;
    size_t pos = texto.find(palabra);
    while (pos != std::string::npos) {
        const bool inicio = pos == 0 || texto[pos - 1] == ' ';
        const bool fin = pos + palabra.size() == texto.size() || texto[pos + palabra.size()] == ' ';
        if (inicio && fin) return true;
        pos = texto.find(palabra, pos + 1);
    }
    return false;
}
}
