#ifndef AUTOLABA_APP_COMMON_H
#define AUTOLABA_APP_COMMON_H

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

// проверка на выбор типа
inline int ReadMenuChoice(int minValue, int maxValue) {
    int x = 0;
    if (!(std::cin >> x)) throw std::runtime_error("Bad numeric input");
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    if (x < minValue || x > maxValue) throw std::runtime_error("Choice out of range");
    return x;
}

// файл/консоль
inline int ReadInputSource() {
    std::cout << "Input source:\n"
                 "1 - Console\n"
                 "2 - File\n"
                 "> ";
    return ReadMenuChoice(1, 2);
}

// считывание элементов с консоли
inline std::vector<std::string> ReadLinesConsoleUntilEmpty() {
    std::cout << "Enter elements, one per line. Empty line finishes.\n";
    std::vector<std::string> lines;
    std::string line;
    while (true) {
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) break;
        lines.push_back(line);
    }
    return lines;
}

// считывание элементов из файла
inline std::vector<std::string> ReadLinesFromFile() {
    std::cout << "Enter file path: ";
    std::string path;
    std::getline(std::cin, path);

    std::ifstream fin(path);
    if (!fin) throw std::runtime_error("Cannot open file: " + path);

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(fin, line)) {
        if (line.empty()) break;
        lines.push_back(line);
    }
    return lines;
}

// обрезает пробельные символы в строке
inline std::string Trim(const std::string& s) {
    size_t l = 0;
    while (l < s.size() && std::isspace(static_cast<unsigned char>(s[l]))) ++l;
    size_t r = s.size();
    while (r > l && std::isspace(static_cast<unsigned char>(s[r - 1]))) --r;
    return s.substr(l, r - l);
}

// перевод из строки в T
template<typename T, typename Parser>
std::vector<T> ParseLines(const std::vector<std::string>& lines, Parser parser) {
    std::vector<T> out;
    out.reserve(lines.size());
    for (const auto& raw : lines) {
        const std::string line = Trim(raw);
        if (line.empty()) continue;
        out.push_back(parser(line));
    }
    if (out.empty()) throw std::runtime_error("No elements were provided");
    return out;
}

// удаление дубликатов
template<typename T, typename Equals>
std::vector<T> DeduplicateStable(const std::vector<T>& in, Equals eq, int& removed) {
    removed = 0;
    std::vector<T> out;
    out.reserve(in.size());

    for (const auto& e : in) {
        bool seen = false;
        for (const auto& x : out) {
            if (eq(x, e)) { seen = true; break; }
        }
        if (seen) ++removed;
        else out.push_back(e);
    }
    return out;
}

inline void SaveTextFile(const std::string& path, const std::string& text) {
    std::ofstream out(path);
    if (!out) throw std::runtime_error("Cannot write file: " + path);
    out << text;
}

#endif

