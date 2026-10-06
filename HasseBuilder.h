#ifndef AUTOLABA_HASSE_BUILDER_H
#define AUTOLABA_HASSE_BUILDER_H

#include <algorithm>
#include <map>
#include <queue>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "HasseDiagram.h"

class HasseBuilder {
private:
    // форматирование строки для файлов типа .dot 
    static std::string EscapeDot(const std::string& s) {
        std::string r;
        r.reserve(s.size());
        for (char c : s) {
            if (c == '\\' || c == '"') r.push_back('\\');
            r.push_back(c);
        }
        return r;
    }

public:
    using Edge = std::pair<int, int>;

    // создание диаграммы и получение ребер на основе компаратора
    template<typename T>
    static std::vector<Edge> BuildHasseEdges(const std::vector<T>& elements, Comparator<T> comparator) {
        HasseDiagram<T> diagram(std::move(comparator));
        diagram.AddElements(elements);
        diagram.Build();
        return diagram.GetEdges();
    }
    // создание диаграммы и получение ребер на основе функции сравнения
    template<typename T, typename LessFunc>
    static std::vector<Edge> BuildHasseEdgesFromLess(const std::vector<T>& elements, LessFunc lessThan) {
        auto comparator = HasseComparators::FromPartialOrder<T>(lessThan);
        return BuildHasseEdges(elements, std::move(comparator));
    }

    // распределение вершин по уровням
    static std::map<int, std::vector<int>> LevelIndex(const std::vector<Edge>& edges, int n) {
        std::vector<std::vector<int>> adj(n);
        std::vector<int> indeg(n, 0);
        for (const auto& e : edges) {
            adj[e.first].push_back(e.second);
            indeg[e.second]++;
        }

        std::queue<int> q;
        std::vector<int> level(n, 0);
        for (int i = 0; i < n; ++i) if (indeg[i] == 0) q.push(i);

        while (!q.empty()) {
            int u = q.front(); q.pop();
            for (int v : adj[u]) {
                level[v] = std::max(level[v], level[u] + 1);
                if (--indeg[v] == 0) q.push(v);
            }
        }

        std::map<int, std::vector<int>> result;
        for (int i = 0; i < n; ++i) result[level[i]].push_back(i);
        return result;
    }

    // вывод информации о диаграмме в файл формата .dot
    template<typename T>
    static std::string ToDot(
        const std::vector<T>& elements,
        const std::vector<Edge>& edges,
        ToStringFunc<T> toString = nullptr,
        bool includeExtremesAsComments = true
    ) {
        if (!toString) {
            toString = [](const T& v) { return detail::defaultToString(v); };
        }

        std::ostringstream out;
        out << "digraph Hasse {\n";
        out << "  rankdir=BT;\n";
        out << "  node [shape=circle];\n";

        for (size_t i = 0; i < elements.size(); ++i) {
            out << "  n" << i << " [label=\"" << EscapeDot(toString(elements[i])) << "\"];\n";
        }

        for (const auto& [u, v] : edges) {
            out << "  n" << u << " -> n" << v << ";\n";
        }

        if (includeExtremesAsComments) {
            auto levels = LevelIndex(edges, static_cast<int>(elements.size()));
            const int minLvl = levels.empty() ? 0 : levels.begin()->first;
            const int maxLvl = levels.empty() ? 0 : levels.rbegin()->first;
            int maxWidth = 0;
            for (const auto& [lvl, vec] : levels) {
                maxWidth = std::max(maxWidth, static_cast<int>(vec.size()));
            }

            out << "  // Extreme characteristics:\n";
            out << "  // Minimal elements: [";
            if (!levels.empty()) {
                const auto& mins = levels[minLvl];
                for (size_t i = 0; i < mins.size(); ++i) {
                    if (i) out << ", ";
                    out << toString(elements[mins[i]]);
                }
            }
            out << "]\n";

            out << "  // Maximal elements: [";
            if (!levels.empty()) {
                const auto& maxs = levels[maxLvl];
                for (size_t i = 0; i < maxs.size(); ++i) {
                    if (i) out << ", ";
                    out << toString(elements[maxs[i]]);
                }
            }
            out << "]\n";

            out << "  // Height: " << static_cast<int>(levels.size()) << "\n";
            out << "  // Width: " << maxWidth << "\n";
        }

        out << "}\n";
        return out.str();
    }
};

#endif
