#ifndef AUTOLABA_SIMPLE_TYPE_HANDLER_H
#define AUTOLABA_SIMPLE_TYPE_HANDLER_H

#include <functional>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "AppCommon.h"
#include "HasseBuilder.h"
#include "ITypeHandler.h"
#include "Draw.h"

template<typename T>
class SimpleTypeHandler final : public ITypeHandler {
public:
    using Parser = std::function<T(const std::string&)>;
    using Equals = std::function<bool(const T&, const T&)>;
    using ToStr = ToStringFunc<T>;
    using RuleFactory = std::function<Comparator<T>()>;

    struct Rule {
        std::string name;
        RuleFactory makeComparator;
    };

    SimpleTypeHandler(
        std::string typeName,
        Parser parser,
        Equals equals,
        ToStr toString,
        std::vector<Rule> rules
    ) : typeName_(std::move(typeName)),
        parser_(std::move(parser)),
        equals_(std::move(equals)),
        toString_(std::move(toString)),
        rules_(std::move(rules)) {}

    std::string Name() const override { return typeName_; }

    void Run() override {
        const int src = ReadInputSource();
        RunWithSource(src);
    }

    // полный запуск системы с учетом вида считывания данных
    void RunWithSource(int src) override {
        std::vector<std::string> lines = (src == 1) ? ReadLinesConsoleUntilEmpty() : ReadLinesFromFile();
        std::vector<T> elements = ParseLines<T>(lines, parser_);

        int removed = 0;
        elements = DeduplicateStable<T>(elements, equals_, removed);
        if (removed > 0) {
            std::cout << "Removed duplicates: " << removed << "\n";
        }

        std::cout << "Elements (" << elements.size() << "):\n";
        for (size_t i = 0; i < elements.size(); ++i) {
            std::cout << "  [" << i << "] " << toString_(elements[i]) << "\n";
        }

        std::cout << "Choose the way of making relation:\n1 - By rule\n2 - Making pairs\n> ";
        int scr;
        std::cin >> scr;
        std::vector<HasseBuilder::Edge> edges;
        if (scr == 1) {
            if (rules_.empty()) throw std::runtime_error("No rules registered for type: " + typeName_);

            std::cout << "Choose rule for " << typeName_ << ":\n";
            for (size_t i = 0; i < rules_.size(); ++i) {
                std::cout << "  " << (i + 1) << " - " << rules_[i].name << "\n";
            }
            std::cout << "> ";
            const int r = ReadMenuChoice(1, static_cast<int>(rules_.size()));

            Comparator<T> cmp = rules_[r - 1].makeComparator();
            edges = HasseBuilder::BuildHasseEdges(elements, std::move(cmp));
        } else {
            std::cout << "Write every pair you need, ending with space:\n";
            std::string line;
            while (true) {
                if (!std::getline(std::cin, line)) break;
                if (line == " ") break;

                std::vector<T> words;
                std::istringstream lineStream(line);
                std::string word;
                while (lineStream >> word) {
                    if (word.empty()) continue;
                    words.push_back(parser_(word));
                }
                if (words.size() < 2) {
                    std::cout << "";
                    continue;
                }
                int index1 = -1;
                int index2 = -1;
                for (int i = 0; i < elements.size(); i++) {
                    if (elements[i] == words[0]) {
                        index1 = i;
                    } else if (elements[i] == words[1]) {
                        index2 = i;
                    }
                }
                if (index1 != -1 && index2 != -1) {
                    edges.emplace_back(index1, index2);
                }
            }
        }
        std::cout << "\nHasse edges (" << edges.size() << "):\n";
        for (const auto& [u, v] : edges) {
            std::cout << toString_(elements[u]) << " -> " << toString_(elements[v]) << "\n";
        }

        const std::string dot = HasseBuilder::ToDot<T>(elements, edges, toString_);
        SaveTextFile("hasse.dot", dot);
        std::cout << "Saved hasse.dot\n";
        std::vector<DrawVertex> vertices = VerticesFromHasse(elements, edges, toString_);
        std::cout << "HasseDiagram was saved in screenshot.png\n";
        int argc = 0;
        char* argv[] = {nullptr};
        glutInit(&argc, argv);
        DrawHasse(vertices, edges, "../Graph/screenshot.png");
    }


private:
    std::string typeName_;
    Parser parser_;
    Equals equals_;
    ToStr toString_;
    std::vector<Rule> rules_;
};

#endif
