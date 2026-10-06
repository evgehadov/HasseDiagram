#include <algorithm>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "AppCommon.h"
#include "HasseBuilder.h"
#include "ITypeHandler.h"
#include "SimpleTypeHandler.h"
#include "Tests.h"

// максимальные элементы неправильны - есть
// прикрутить рисовашку к этому всему - есть
// прикрутить БиоИнформатику - есть
// вернуть систему пар - сделать
// прописать тесты большие (20+ элементов) - сделать (желательно картинки скринов сохранить в папку - есть
// добавить в изначальном выборе шаг введения своей структуры (как сейчас, введена биоинфа)


struct Point2D {
    double x = 0.0;
    double y = 0.0;

    bool operator==(const Point2D& other) const {
        return x == other.x && y == other.y;
    }
};

// пользовательский тип: MyType (заготовка)
struct MyType {
    // TODO: впиши сюда поля своего типа.
    // Примеры:
    // int a = 0;
    // std::string name;

    // Минимум, который нужен проекту:
    // 1) эквивалентность (==) для удаления дублей
    // 2) парсер из строки
    // 3) ToString для печати/вершин
};

// эквивалентность
static bool operator==(const MyType& lhs, const MyType& rhs) { // MyTypeEquals
    // TODO: напиши сравнение
    // return lhs.a == rhs.a && lhs.name == rhs.name;
    return false;
}

// перевод в строку
static std::string MyTypeToString(const MyType& v) {
    // TODO: сделай красивый вывод
    // return std::format("({}, {})", v.a, v.name);
    return "<MyType>";
}

// парсер из строки
static MyType ParseMyType(const std::string& line) {
    // TODO: распарсить line -> MyType
    // Пример:
    // std::istringstream iss(line);
    // MyType t;
    // if (!(iss >> t.a >> t.name)) throw std::runtime_error("Bad MyType line: '" + line + "'");
    // return t;

    throw std::runtime_error("ParseMyType is not implemented yet. Fill it in main.cpp");
}

// частичный порядок (строгий): a < b ?
static bool MyTypeLessStrict(const MyType& a, const MyType& b) {
    // TODO: впиши правило "строго меньше"
    // Важно: строго меньше = НЕ равны.
    // return (a.a < b.a) && ...;

    throw std::runtime_error("MyTypeLessStrict is not implemented yet. Fill it in main.cpp");
}

// парсеры для различных типов данных
static int ParseInt(const std::string& line) {
    size_t pos = 0;
    int v = 0;
    try {
        v = std::stoi(line, &pos);
    } catch (...) {
        throw std::runtime_error("Bad INT line: '" + line + "'");
    }
    if (pos != line.size()) throw std::runtime_error("Bad INT line: '" + line + "'");
    return v;
}

static std::string ParseString(const std::string& line) {
    if (line.empty()) throw std::runtime_error("Empty STRING line");
    return line;
}

static std::vector<int> ParseSetInt(const std::string& line) {
    std::istringstream iss(line);
    std::vector<int> v;
    int x;
    while (iss >> x) v.push_back(x);
    if (v.empty()) throw std::runtime_error("Bad SET_INT line (no numbers): '" + line + "'");
    std::sort(v.begin(), v.end());
    v.erase(std::unique(v.begin(), v.end()), v.end());
    return v;
}

static Point2D ParsePoint2D(const std::string& line) {
    std::istringstream iss(line);
    Point2D p;
    if (!(iss >> p.x >> p.y)) {
        throw std::runtime_error("Bad Point2D line (expected: x y): '" + line + "'");
    }
    return p;
}

// правила
static bool IsPrefixStrict(const std::string& a, const std::string& b) {
    if (a.size() >= b.size()) return false;
    return std::equal(a.begin(), a.end(), b.begin());
}

static bool IsSubsequenceStrict(const std::string& a, const std::string& b) {
    if (a.size() >= b.size()) return false;
    size_t i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        if (a[i] == b[j]) ++i;
        ++j;
    }
    return i == a.size();
}

static bool IsSubsetStrict(const std::vector<int>& A, const std::vector<int>& B) {
    if (A.size() >= B.size()) return false;
    size_t i = 0, j = 0;
    while (i < A.size() && j < B.size()) {
        if (A[i] == B[j]) { ++i; ++j; }
        else if (A[i] > B[j]) { ++j; }
        else return false;
    }
    return i == A.size();
}

static bool PointLeqStrict(const Point2D& a, const Point2D& b) {
    const bool le = (a.x <= b.x) && (a.y <= b.y);
    const bool eq = (a.x == b.x) && (a.y == b.y);
    return le && !eq;
}

static std::string PointToString(const Point2D& p) {
    std::ostringstream out;
    out << "(" << p.x << ", " << p.y << ")";
    return out.str();
}

// мэйкеры 
static std::unique_ptr<ITypeHandler> MakeIntHandler() {
    using H = SimpleTypeHandler<int>;
    std::vector<H::Rule> rules;

    rules.push_back({
        "divides order (a divides b)",
        [] {
            return HasseComparators::FromPartialOrder<int>([](int a, int b) {
                if (a == 0) return false;
                return (a != b) && (b % a == 0);
            });
        }
    });

    rules.push_back({
        "less or equal (a <= b)",
        [] {
            return HasseComparators::FromPartialOrder<int>([](int a, int b) { return a < b; });
        }
    });

    return std::make_unique<H>(
        "INT",
        ParseInt,
        [](int a, int b) { return a == b; },
        [](int v) { return std::to_string(v); },
        std::move(rules)
    );
}

static std::unique_ptr<ITypeHandler> MakeStringHandler() {
    using H = SimpleTypeHandler<std::string>;
    std::vector<H::Rule> rules;

    rules.push_back({ "prefix order (x is prefix of y)", [] { return HasseComparators::FromPartialOrder<std::string>(IsPrefixStrict); } });
    rules.push_back({ "subsequence order", [] { return HasseComparators::FromPartialOrder<std::string>(IsSubsequenceStrict); } });
    rules.push_back({
        "lexicographic order (total)",
        [] {
            return HasseComparators::FromPartialOrder<std::string>([](const std::string& a, const std::string& b) { return a < b; });
        }
    });

    return std::make_unique<H>(
        "STRING",
        ParseString,
        [](const std::string& a, const std::string& b) { return a == b; },
        [](const std::string& s) { return s; },
        std::move(rules)
    );
}

static std::unique_ptr<ITypeHandler> MakeSetIntHandler() {
    using H = SimpleTypeHandler<std::vector<int>>;
    std::vector<H::Rule> rules;

    rules.push_back({
        "subset order (A ⊂ B)",
        [] { return HasseComparators::FromPartialOrder<std::vector<int>>(IsSubsetStrict); }
    });

    return std::make_unique<H>(
        "SET_INT",
        ParseSetInt,
        [](const std::vector<int>& a, const std::vector<int>& b) { return a == b; },
        [](const std::vector<int>& v) { return detail::defaultToString(v); },
        std::move(rules)
    );
}

static std::unique_ptr<ITypeHandler> MakePoint2DHandler() {
    using H = SimpleTypeHandler<Point2D>;
    std::vector<H::Rule> rules;

    rules.push_back({
        "coordinate-wise order (x1<=x2 and y1<=y2)",
        [] { return HasseComparators::FromPartialOrder<Point2D>(PointLeqStrict); }
    });

    return std::make_unique<H>(
        "POINT2D",
        ParsePoint2D,
        [](const Point2D& a, const Point2D& b) { return a.x == b.x && a.y == b.y; },
        PointToString,
        std::move(rules)
    );
}

static std::unique_ptr<ITypeHandler> MakeMyTypeHandler() {
    using H = SimpleTypeHandler<MyType>;
    std::vector<H::Rule> rules;

    rules.push_back({
        "TODO: define partial order for MyType",
        [] {
            return HasseComparators::FromPartialOrder<MyType>(MyTypeLessStrict);
        }
    });

    return std::make_unique<H>(
        "MYTYPE",
        ParseMyType,
        [](const MyType& l, const MyType& r) { return l == r; },
        MyTypeToString,
        std::move(rules)
    );
}

int main() {
    try {
        std::vector<std::unique_ptr<ITypeHandler>> demos;
        demos.push_back(MakeIntHandler());
        demos.push_back(MakeStringHandler());
        demos.push_back(MakeSetIntHandler());
        demos.push_back(MakePoint2DHandler());
        demos.push_back(MakeMyTypeHandler());

        const int src = ReadInputSource();

        std::cout << "Choose type:\n";
        for (size_t i = 0; i < demos.size(); ++i) {
            std::cout << "  " << (i + 1) << " - " << demos[i]->Name() << "\n";
        }
        const int bioIndex  = (int)demos.size() + 1;
        const int testIndex = (int)demos.size() + 2;

        std::cout << "  " << bioIndex  << " - BIOINFO\n";
        std::cout << "  " << testIndex << " - TESTS\n";
        std::cout << "> ";

        const int choice = ReadMenuChoice(1, testIndex);

        if (choice == 5) {
            std::cout << "Before this write some functions for us to build a HasseDiagram!!!\n";
        }

        if (choice == testIndex) {
            return tests::RunAllCompact(std::cout);
        }

        if (choice == bioIndex) {
            readCSV();

            std::vector<std::string> lines = (src == 1) ? ReadLinesConsoleUntilEmpty() : ReadLinesFromFile();
            std::vector<std::string> elements = ParseLines<std::string>(lines, ParseString);

            int removed = 0;
            elements = DeduplicateStable<std::string>(
                elements,
                [](const std::string& a, const std::string& b){ return a == b; },
                removed
            );
            if (removed > 0) std::cout << "Removed duplicates: " << removed << "\n";

            std::vector<std::string> NewElements;
            for (const auto& element : elements) {
                if (CheckSeq(element)) NewElements.push_back(element);
            }

            std::cout << "Elements (" << NewElements.size() << "):\n";
            for (size_t i = 0; i < NewElements.size(); ++i) {
                std::cout << "  [" << i << "] " << NewElements[i] << "\n";
            }

            auto cmp = HasseComparators::FromPartialOrder<std::string>(IsSubsequenceStrict);
            auto edges = HasseBuilder::BuildHasseEdges(elements, std::move(cmp));

            std::cout << "\nHasse edges (" << edges.size() << "):\n";
            for (const auto& [u, v] : edges) {
                std::cout << elements[u] << " -> " << elements[v] << "\n";
            }

            const std::string dot = HasseBuilder::ToDot<std::string>(elements, edges);
            SaveTextFile("hasse.dot", dot);
            std::cout << "Saved hasse.dot\n";

#ifndef NO_DRAW
            static bool glutInited = false;
            if (!glutInited) {
                int argc = 0;
                char* argv[] = { nullptr };
                glutInit(&argc, argv);
                glutInited = true;
            }
            std::vector<DrawVertex> vertices = VerticesFromHasse(elements, edges);
            DrawHasseBio(vertices, edges);
#endif

            return 0;
        }

        demos[choice - 1]->RunWithSource(src);
        return 0;

        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
            return 1;
        }
    }
