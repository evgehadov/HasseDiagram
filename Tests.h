#ifndef AUTOLABA_TESTS_H
#define AUTOLABA_TESTS_H

#include <algorithm>
#include <exception>
#include <filesystem>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "HasseBuilder.h"
#include "Draw.h"
#include "AminoAcids.h"
#include "AppCommon.h"


namespace tests {

// проверка директории для скриншотов
inline std::string GraphBaseDir() {
    namespace fs = std::filesystem;

    if (fs::exists("../Graph") && fs::is_directory("../Graph")) {
        return "../Graph";
    }
    if (fs::exists("Graph") && fs::is_directory("Graph")) {
        return "Graph";
    }
    fs::create_directories("Graph");
    return "Graph";
}

// multiple definition (для обработки этой ошибки, прописывание полного пути к файлу)
inline std::string JoinPath(const std::string& dir, const std::string& name) {
    namespace fs = std::filesystem;
    return (fs::path(dir) / fs::path(name)).string();
}


// проверка cover-инвариантов для Hasse-ребер
template<typename T>
bool VerifyCovers(const std::vector<T>& elements,
                         const std::vector<HasseBuilder::Edge>& edges,
                         const std::function<bool(const T&, const T&)>& lessStrict,
                         std::string* outError = nullptr) {
    const int n = (int)elements.size();

    auto fail = [&](const std::string& msg) {
        if (outError) *outError = msg;
        return false;
    };

    for (auto [u, v] : edges) {
        if (u < 0 || v < 0 || u >= n || v >= n) {
            return fail("Edge index out of range");
        }
        if (u == v) {
            return fail("Self-loop edge detected");
        }
        if (!lessStrict(elements[u], elements[v])) {
            return fail("Edge is not 'less' by rule (u !< v)");
        }

        for (int k = 0; k < n; ++k) {
            if (k == u || k == v) continue;
            if (lessStrict(elements[u], elements[k]) && lessStrict(elements[k], elements[v])) {
                return fail("Edge is not a cover (found intermediate k)");
            }
        }
    }
    return true;
}

// правила

inline bool IntDividesStrict(int a, int b) {
    if (a == 0) return false;
    return (a != b) && (b % a == 0);
}
inline bool IntLessStrict(int a, int b) { return a < b; }

inline bool IsPrefixStrict(const std::string& a, const std::string& b) {
    if (a.size() >= b.size()) return false;
    return std::equal(a.begin(), a.end(), b.begin());
}
inline bool IsSubsequenceStrict(const std::string& a, const std::string& b) {
    if (a.size() >= b.size()) return false;
    size_t i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        if (a[i] == b[j]) ++i;
        ++j;
    }
    return i == a.size();
}
inline bool LexStrict(const std::string& a, const std::string& b) { return a < b; }

inline bool IsSubsetStrict(const std::vector<int>& A, const std::vector<int>& B) {
    if (A.size() >= B.size()) return false;
    size_t i = 0, j = 0;
    while (i < A.size() && j < B.size()) {
        if (A[i] == B[j]) { ++i; ++j; }
        else if (A[i] > B[j]) { ++j; }
        else return false;
    }
    return i == A.size();
}

struct Point2D {
    double x = 0.0;
    double y = 0.0;
};
inline bool PointLeqStrict(const Point2D& a, const Point2D& b) {
    const bool le = (a.x <= b.x) && (a.y <= b.y);
    const bool eq = (a.x == b.x) && (a.y == b.y);
    return le && !eq;
}
inline std::string PointToString(const Point2D& p) {
    std::ostringstream out;
    out << "(" << p.x << ", " << p.y << ")";
    return out.str();
}

// тесты
struct TestCase {
    std::string name;
    std::function<bool(std::string* err)> run;
};

// Тест 1: INT divides
inline TestCase T01_IntDivides() {
    return {
        "T01_INT_divides",
        [](std::string* err) -> bool {
            std::vector<int> elements;
            elements.reserve(16);
            for (int a = 0; a <= 3; ++a) {
                for (int b = 0; b <= 3; ++b) {
                    int v = 1;
                    for (int i=0;i<a;++i) v *= 2;
                    for (int j=0;j<b;++j) v *= 3;
                    elements.push_back(v);
                }
            }

            auto cmp = HasseComparators::FromPartialOrder<int>(IntDividesStrict);
            auto edges = HasseBuilder::BuildHasseEdges(elements, cmp);

            std::set<int> setVals(elements.begin(), elements.end());
            std::set<std::pair<int,int>> expected;
            for (int i = 0; i < (int)elements.size(); ++i) {
                int x = elements[i];
                if (setVals.count(x*2)) {
                    int j = (int)(std::find(elements.begin(), elements.end(), x*2) - elements.begin());
                    expected.insert({i,j});
                }
                if (setVals.count(x*3)) {
                    int j = (int)(std::find(elements.begin(), elements.end(), x*3) - elements.begin());
                    expected.insert({i,j});
                }
            }

            if ((int)edges.size() != (int)expected.size()) {
                if (err) *err = "edges.size != expected.size";
            }

            if (!VerifyCovers<int>(elements, edges, IntDividesStrict, err)) return false;

            const std::string base = GraphBaseDir();
            SaveTextFile(JoinPath(base, "T01_INT_divides.dot"), HasseBuilder::ToDot<int>(elements, edges, [](int v){return std::to_string(v);} ));

            auto vertices = VerticesFromHasse<int>(elements, edges, [](int v){return std::to_string(v);} );
            SaveHassePngHidden(vertices, edges, JoinPath(base, "T01_INT_divides.png").c_str());

            std::set<std::pair<int,int>> got(edges.begin(), edges.end());
            if (got != expected) {
                if (err) *err = "edge set mismatch vs expected (divides lattice)";
                return false;
            }
            return true;
        }
    };
}

// Тест 2: INT
inline TestCase T02_IntLess() {
    return {
        "T02_INT_less",
        [](std::string* err) -> bool {
            std::vector<int> elements;
            for (int i = 1; i <= 20; ++i) elements.push_back(i); // уже отсортировано

            auto cmp = HasseComparators::FromPartialOrder<int>(IntLessStrict);
            auto edges = HasseBuilder::BuildHasseEdges(elements, cmp);

            if ((int)edges.size() != 19) {
                if (err) *err = "Expected 19 edges for total order of 20 elements";
                return false;
            }
            if (!VerifyCovers<int>(elements, edges, IntLessStrict, err)) return false;

            std::set<std::pair<int,int>> expected;
            for (int i = 0; i < 19; ++i) expected.insert({i, i+1});
            std::set<std::pair<int,int>> got(edges.begin(), edges.end());
            if (got != expected) {
                if (err) *err = "Edges are not consecutive chain for int<";
                return false;
            }

            const std::string base = GraphBaseDir();
            SaveTextFile(JoinPath(base, "T02_INT_less.dot"), HasseBuilder::ToDot<int>(elements, edges, [](int v){return std::to_string(v);} ));
            auto vertices = VerticesFromHasse<int>(elements, edges, [](int v){return std::to_string(v);} );
            SaveHassePngHidden(vertices, edges, JoinPath(base, "T02_INT_less.png").c_str());
            return true;
        }
    };
}

// Тест 3: STRING prefix
inline TestCase T03_StringPrefix() {
    return {
        "T03_STRING_prefix",
        [](std::string* err) -> bool {
            std::vector<std::string> elements = {
                "a","ab","abc","abcd","abx","abxy",
                "b","ba","bac","bb",
                "c","ca","cab",
                "d","da","dab"
            };

            auto cmp = HasseComparators::FromPartialOrder<std::string>(IsPrefixStrict);
            auto edges = HasseBuilder::BuildHasseEdges(elements, cmp);

            std::set<std::pair<int,int>> expected = {
                {0,1},{1,2},{2,3},{1,4},{4,5},
                {6,7},{7,8},{6,9},
                {10,11},{11,12},
                {13,14},{14,15}
            };

            if (!VerifyCovers<std::string>(elements, edges, IsPrefixStrict, err)) return false;

            std::set<std::pair<int,int>> got(edges.begin(), edges.end());
            if (got != expected) {
                if (err) *err = "Prefix cover edges mismatch";
                return false;
            }

            const std::string base = GraphBaseDir();
            SaveTextFile(JoinPath(base, "T03_STRING_prefix.dot"), HasseBuilder::ToDot<std::string>(elements, edges, [](const std::string& s){return s;} ));
            auto vertices = VerticesFromHasse<std::string>(elements, edges, [](const std::string& s){return s;} );
            SaveHassePngHidden(vertices, edges, JoinPath(base, "T03_STRING_prefix.png").c_str());
            return true;
        }
    };
}

// Тест 4: STRING subsequence
inline TestCase T04_StringSubsequence() {
    return {
        "T04_STRING_subsequence",
        [](std::string* err) -> bool {
            std::vector<std::string> elements = {
                "A",
                "AB",
                "ACB",
                "AXCB",
                "ZAXCB",
                "ZAXRCB",
                "ZQAXRCB",
                "ZQAXRCPB",
                "ZQAXRCPBD",
                "MZQAXRCPBD",
                "MZQAXRCPBDE",
                "MZQAXRCPBDEF",
                "MZQAXRCPBDEFG",
                "MZQAXRCPBDEFGH",
                "MZQAXRCPBDEFGHI"
            };

            auto cmp = HasseComparators::FromPartialOrder<std::string>(IsSubsequenceStrict);
            auto edges = HasseBuilder::BuildHasseEdges(elements, cmp);

            if ((int)edges.size() != 14) {
                if (err) *err = "Expected 14 edges for subsequence chain of 15";
                return false;
            }
            if (!VerifyCovers<std::string>(elements, edges, IsSubsequenceStrict, err)) return false;

            std::set<std::pair<int,int>> expected;
            for (int i = 0; i < 14; ++i) expected.insert({i,i+1});
            std::set<std::pair<int,int>> got(edges.begin(), edges.end());
            if (got != expected) {
                if (err) *err = "Subsequence chain edges mismatch";
                return false;
            }

            const std::string base = GraphBaseDir();
            SaveTextFile(JoinPath(base, "T04_STRING_subsequence.dot"), HasseBuilder::ToDot<std::string>(elements, edges, [](const std::string& s){return s;} ));
            auto vertices = VerticesFromHasse<std::string>(elements, edges, [](const std::string& s){return s;} );
            SaveHassePngHidden(vertices, edges, JoinPath(base, "T04_STRING_subsequence.png").c_str());
            return true;
        }
    };
}

// Тест 5: STRING lexicographic
inline TestCase T05_StringLex() {
    return {
        "T05_STRING_lex",
        [](std::string* err) -> bool {
            std::vector<std::string> elements = {
                "aa","ab","ac","ad","ae",
                "ba","bb","bc","bd","be",
                "ca","cb","cc","cd","ce",
                "da","db","dc","dd","de"
            };

            auto cmp = HasseComparators::FromPartialOrder<std::string>(LexStrict);
            auto edges = HasseBuilder::BuildHasseEdges(elements, cmp);

            if ((int)edges.size() != 19) {
                if (err) *err = "Expected 19 edges for total lex order of 20";
                return false;
            }
            if (!VerifyCovers<std::string>(elements, edges, LexStrict, err)) return false;

            std::set<std::pair<int,int>> expected;
            for (int i = 0; i < 19; ++i) expected.insert({i,i+1});
            std::set<std::pair<int,int>> got(edges.begin(), edges.end());
            if (got != expected) {
                if (err) *err = "Lex chain edges mismatch";
                return false;
            }

            const std::string base = GraphBaseDir();
            SaveTextFile(JoinPath(base, "T05_STRING_lex.dot"), HasseBuilder::ToDot<std::string>(elements, edges, [](const std::string& s){return s;} ));
            auto vertices = VerticesFromHasse<std::string>(elements, edges, [](const std::string& s){return s;} );
            SaveHassePngHidden(vertices, edges, JoinPath(base, "T05_STRING_lex.png").c_str());
            return true;
        }
    };
}

// Тест 6: SET_INT subset
inline TestCase T06_SetIntSubset() {
    return {
        "T06_SET_INT_subset",
        [](std::string* err) -> bool {
            std::vector<std::vector<int>> elements;
            elements.reserve(15);

            for (int mask = 1; mask < (1<<4); ++mask) {
                std::vector<int> s;
                for (int i = 0; i < 4; ++i) {
                    if (mask & (1<<i)) s.push_back(i+1);
                }
                elements.push_back(s);
            }

            auto cmp = HasseComparators::FromPartialOrder<std::vector<int>>(IsSubsetStrict);
            auto edges = HasseBuilder::BuildHasseEdges(elements, cmp);

            if ((int)edges.size() != 28) {
                if (err) *err = "Expected 28 edges for subset lattice n=4 without empty set";
                return false;
            }
            if (!VerifyCovers<std::vector<int>>(elements, edges, IsSubsetStrict, err)) return false;

            auto vecToKey = [](const std::vector<int>& v){
                std::ostringstream out;
                for (int x : v) out << x << ",";
                return out.str();
            };
            std::map<std::string,int> idx;
            for (int i = 0; i < (int)elements.size(); ++i) idx[vecToKey(elements[i])] = i;

            std::set<std::pair<int,int>> expected;
            for (int i = 0; i < (int)elements.size(); ++i) {
                const auto& A = elements[i];
                std::set<int> setA(A.begin(), A.end());
                for (int x = 1; x <= 4; ++x) {
                    if (setA.count(x)) continue;
                    std::vector<int> B = A;
                    B.push_back(x);
                    std::sort(B.begin(), B.end());
                    auto it = idx.find(vecToKey(B));
                    if (it != idx.end()) expected.insert({i, it->second});
                }
            }

            std::set<std::pair<int,int>> got(edges.begin(), edges.end());
            if (got != expected) {
                if (err) *err = "Subset cover edges mismatch";
                return false;
            }

            const std::string base = GraphBaseDir();
            SaveTextFile(JoinPath(base, "T06_SET_INT_subset.dot"), HasseBuilder::ToDot<std::vector<int>>(elements, edges, nullptr));
            auto vertices = VerticesFromHasse<std::vector<int>>(elements, edges, nullptr);
            SaveHassePngHidden(vertices, edges, JoinPath(base, "T06_SET_INT_subset.png").c_str());
            return true;
        }
    };
}

// Тест 7: POINT2D coordinate-wise
inline TestCase T07_Point2D() {
    return {
        "T07_POINT2D_coordwise",
        [](std::string* err) -> bool {
            std::vector<Point2D> elements;
            elements.reserve(16);
            for (int x=0;x<4;++x) {
                for (int y=0;y<4;++y) {
                    elements.push_back(Point2D{(double)x, (double)y});
                }
            }

            auto cmp = HasseComparators::FromPartialOrder<Point2D>(PointLeqStrict);
            auto edges = HasseBuilder::BuildHasseEdges(elements, cmp);

            if ((int)edges.size() != 24) {
                if (err) *err = "Expected 24 edges for 4x4 grid covers";
                return false;
            }
            if (!VerifyCovers<Point2D>(elements, edges, PointLeqStrict, err)) return false;

            auto idx = [&](int x,int y){ return x*4 + y; };
            std::set<std::pair<int,int>> expected;
            for (int x=0;x<4;++x) {
                for (int y=0;y<4;++y) {
                    if (x+1<4) expected.insert({idx(x,y), idx(x+1,y)});
                    if (y+1<4) expected.insert({idx(x,y), idx(x,y+1)});
                }
            }

            std::set<std::pair<int,int>> got(edges.begin(), edges.end());
            if (got != expected) {
                if (err) *err = "Point2D cover edges mismatch";
                return false;
            }

            const std::string base = GraphBaseDir();
            SaveTextFile(JoinPath(base, "T07_POINT2D_coordwise.dot"), HasseBuilder::ToDot<Point2D>(elements, edges, PointToString));
            auto vertices = VerticesFromHasse<Point2D>(elements, edges, PointToString);
            SaveHassePngHidden(vertices, edges, JoinPath(base, "T07_POINT2D_coordwise.png").c_str());
            return true;
        }
    };
}

// Тест 8: BIOINFO
inline TestCase T08_Bioinfo() {
    return {
        "T08_BIOINFO_chain",
        [](std::string* err) -> bool {
            try { readCSV(); }
            catch (const std::exception& e) {
                if (err) *err = std::string("readCSV failed: ") + e.what();
                return false;
            }

            std::vector<std::string> elements = {
                "C",
                "CS",
                "CST",
                "CSTA",
                "CSTAG",
                "CSTAGP",
                "CSTAGPD",
                "CSTAGPDE",
                "CSTAGPDEQ",
                "CSTAGPDEQN",
                "CSTAGPDEQNH",
                "CSTAGPDEQNHR",
                "CSTAGPDEQNHRK",
                "CSTAGPDEQNHRKM",
                "CSTAGPDEQNHRKMI"
            };

            for (const auto& s : elements) {
                if (!CheckSeq(s)) {
                    if (err) *err = "Invalid amino sequence in test data: " + s;
                    return false;
                }
            }

            auto cmp = HasseComparators::FromPartialOrder<std::string>(IsSubsequenceStrict);
            auto edges = HasseBuilder::BuildHasseEdges(elements, cmp);

            if ((int)edges.size() != 14) {
                if (err) *err = "Expected 14 edges for BIO chain of 15";
                return false;
            }
            if (!VerifyCovers<std::string>(elements, edges, IsSubsequenceStrict, err)) return false;

            const std::string base = GraphBaseDir();
            SaveTextFile(JoinPath(base, "T08_BIOINFO_chain.dot"), HasseBuilder::ToDot<std::string>(elements, edges, [](const std::string& s){return s;} ));
            auto vertices = VerticesFromHasse<std::string>(elements, edges, [](const std::string& s){return s;} );
            SaveHassePngHidden(vertices, edges, JoinPath(base, "T08_BIOINFO_chain.png").c_str());

            if (Score("CST", "CST") <= 0) {
                if (err) *err = "Unexpected Score(seq,seq) <= 0";
                return false;
            }

            return true;
        }
    };
}

    // Тест 9: BIOINFO
inline TestCase T09_BioinfoLongChainWithFilter() {
    return {
        "T09_BIOINFO_long_chain_filter",
        [](std::string* err) -> bool {
            try { readCSV(); }
            catch (const std::exception& e) {
                if (err) *err = std::string("readCSV failed: ") + e.what();
                return false;
            }

            std::vector<std::string> raw = {
                "C", "CS", "CST", "CXT", "CSTAG", "CSTAGP", "CSTAGPDEQNH", "HELLO", "CSTAGPDEQNHRKMILVWYF"
            };

            int bad = 0;
            std::vector<std::string> filtered;
            for (const auto& s : raw) {
                if (CheckSeq(s)) filtered.push_back(s);
                else bad++;
            }
            if (bad < 2) {
                if (err) *err = "Expected >=2 invalid sequences filtered out";
                return false;
            }
            if (filtered.empty()) {
                if (err) *err = "All sequences became invalid after filter (unexpected)";
                return false;
            }

            std::vector<std::string> chain = {
                "C",
                "CS",
                "CST",
                "CSTA",
                "CSTAG",
                "CSTAGP",
                "CSTAGPD",
                "CSTAGPDE",
                "CSTAGPDEQ",
                "CSTAGPDEQN",
                "CSTAGPDEQNH",
                "CSTAGPDEQNHR",
                "CSTAGPDEQNHRK",
                "CSTAGPDEQNHRKM",
                "CSTAGPDEQNHRKMI",
                "CSTAGPDEQNHRKMIL",
                "CSTAGPDEQNHRKMILV",
                "CSTAGPDEQNHRKMILVW",
                "CSTAGPDEQNHRKMILVWY",
                "CSTAGPDEQNHRKMILVWYF"
            };

            for (const auto& s : chain) {
                if (!CheckSeq(s)) {
                    if (err) *err = "Invalid amino sequence in chain: " + s;
                    return false;
                }
            }

            auto cmp = HasseComparators::FromPartialOrder<std::string>(IsSubsequenceStrict);
            auto edges = HasseBuilder::BuildHasseEdges(chain, cmp);

            if ((int)edges.size() != (int)chain.size() - 1) {
                if (err) *err = "Expected 19 edges for BIO chain of 20";
                return false;
            }
            if (!VerifyCovers<std::string>(chain, edges, IsSubsequenceStrict, err)) return false;

            const std::string base = GraphBaseDir();
            SaveTextFile(JoinPath(base, "T09_BIOINFO_long_chain_filter.dot"),
                         HasseBuilder::ToDot<std::string>(chain, edges, [](const std::string& s){return s;}));

            auto vertices = VerticesFromHasse<std::string>(chain, edges, [](const std::string& s){return s;});
            SaveHassePngHidden(vertices, edges, JoinPath(base, "T09_BIOINFO_long_chain_filter.png").c_str());

            return true;
        }
    };
}

// Тест 10: BIOINFO
inline TestCase T10_BioinfoScoreSymmetry() {
    return {
        "T10_BIOINFO_score_symmetry",
        [](std::string* err) -> bool {
            try { readCSV(); }
            catch (const std::exception& e) {
                if (err) *err = std::string("readCSV failed: ") + e.what();
                return false;
            }

            const std::string a = "CSTAGPDE";
            const std::string b = "CSTAGPE";

            if (!CheckSeq(a) || !CheckSeq(b)) {
                if (err) *err = "Test sequences not valid by CheckSeq";
                return false;
            }

            const int sab = Score(a, b);
            const int sba = Score(b, a);
            if (sab != sba) {
                if (err) *err = "Score is not symmetric: Score(a,b)!=Score(b,a)";
                return false;
            }

            std::vector<std::string> elems = { "C", "CS", "CST", "CSTA", "CSTAG", "CSTAGP" };
            auto cmp = HasseComparators::FromPartialOrder<std::string>(IsSubsequenceStrict);
            auto edges = HasseBuilder::BuildHasseEdges(elems, cmp);

            const std::string base = GraphBaseDir();
            SaveTextFile(JoinPath(base, "T10_BIOINFO_score_symmetry.dot"),
                         HasseBuilder::ToDot<std::string>(elems, edges, [](const std::string& s){return s;}));

            auto vertices = VerticesFromHasse<std::string>(elems, edges, [](const std::string& s){return s;});
            SaveHassePngHidden(vertices, edges, JoinPath(base, "T10_BIOINFO_score_symmetry.png").c_str());

            return true;
        }
    };
}

// Тест 11: STRING prefix
inline TestCase T11_StringPrefixAutoExpected20() {
    return {
        "T11_STRING_prefix_auto_expected_20",
        [](std::string* err) -> bool {
            std::vector<std::string> elements = {
                "a","b",
                "aa","ab","ba","bb",
                "aaa","aab","aba","abb","baa","bab","bba","bbb",
                "aaaa","aaab","aaba","aabb","abaa","abab"
            };

            auto cmp = HasseComparators::FromPartialOrder<std::string>(IsPrefixStrict);
            auto edges = HasseBuilder::BuildHasseEdges(elements, cmp);

            std::set<std::pair<int,int>> expected;
            for (int u = 0; u < (int)elements.size(); ++u) {
                for (int v = 0; v < (int)elements.size(); ++v) {
                    if (u == v) continue;
                    if (!IsPrefixStrict(elements[u], elements[v])) continue;

                    bool hasIntermediate = false;
                    for (int k = 0; k < (int)elements.size(); ++k) {
                        if (k == u || k == v) continue;
                        if (IsPrefixStrict(elements[u], elements[k]) && IsPrefixStrict(elements[k], elements[v])) {
                            hasIntermediate = true;
                            break;
                        }
                    }
                    if (!hasIntermediate) expected.insert({u,v});
                }
            }

            if (!VerifyCovers<std::string>(elements, edges, IsPrefixStrict, err)) return false;

            std::set<std::pair<int,int>> got(edges.begin(), edges.end());
            if (got != expected) {
                if (err) *err = "Prefix edges mismatch vs auto-expected covers";
                return false;
            }

            const std::string base = GraphBaseDir();
            SaveTextFile(JoinPath(base, "T11_STRING_prefix_auto_expected_20.dot"),
                         HasseBuilder::ToDot<std::string>(elements, edges, [](const std::string& s){return s;}));

            auto vertices = VerticesFromHasse<std::string>(elements, edges, [](const std::string& s){return s;});
            SaveHassePngHidden(vertices, edges, JoinPath(base, "T11_STRING_prefix_auto_expected_20.png").c_str());

            return true;
        }
    };
}
// Тест 9: INT divides
inline TestCase T09_IntDivides_235() {
    return {
        "T09_INT_divides_235",
        [](std::string* err) -> bool {
            std::vector<int> elements;
            elements.reserve(18);
            for (int a = 0; a <= 2; ++a) {
                for (int b = 0; b <= 2; ++b) {
                    for (int c = 0; c <= 1; ++c) {
                        int v = 1;
                        for (int i = 0; i < a; ++i) v *= 2;
                        for (int j = 0; j < b; ++j) v *= 3;
                        for (int k = 0; k < c; ++k) v *= 5;
                        elements.push_back(v);
                    }
                }
            }

            auto cmp = HasseComparators::FromPartialOrder<int>(IntDividesStrict);
            auto edges = HasseBuilder::BuildHasseEdges(elements, cmp);

            std::set<int> setVals(elements.begin(), elements.end());
            std::set<std::pair<int,int>> expected;
            for (int i = 0; i < (int)elements.size(); ++i) {
                int x = elements[i];
                for (int mul : {2, 3, 5}) {
                    int y = x * mul;
                    if (setVals.count(y)) {
                        int j = (int)(std::find(elements.begin(), elements.end(), y) - elements.begin());
                        expected.insert({i, j});
                    }
                }
            }

            if (!VerifyCovers<int>(elements, edges, IntDividesStrict, err)) return false;

            const std::string base = GraphBaseDir();
            SaveTextFile(JoinPath(base, "T09_INT_divides_235.dot"),
                         HasseBuilder::ToDot<int>(elements, edges, [](int v){ return std::to_string(v); }));

            auto vertices = VerticesFromHasse<int>(elements, edges, [](int v){ return std::to_string(v); });
            SaveHassePngHidden(vertices, edges, JoinPath(base, "T09_INT_divides_235.png").c_str());

            std::set<std::pair<int,int>> got(edges.begin(), edges.end());
            if (got != expected) {
                if (err) *err = "edge set mismatch vs expected (divides 2/3/5 lattice)";
                return false;
            }
            return true;
        }
    };
}

// Тест 10: BIOINFO
inline TestCase T10_Bio_SingleLetters_NoEdges() {
    return {
        "T10_BIO_single_letters",
        [](std::string* err) -> bool {
            try { readCSV(); } catch (...) {}

            std::vector<std::string> elements;
            elements.reserve(AllAminoAcids.size());
            for (char c : AllAminoAcids) elements.push_back(std::string(1, c));

            for (const auto& s : elements) {
                if (!CheckSeq(s)) {
                    if (err) *err = "CheckSeq failed on amino acid: " + s;
                    return false;
                }
            }

            auto cmp = HasseComparators::FromPartialOrder<std::string>(IsSubsequenceStrict);
            auto edges = HasseBuilder::BuildHasseEdges(elements, cmp);

            if (!edges.empty()) {
                if (err) *err = "Expected 0 edges for single-letter set under strict subsequence";
            }
            if (!VerifyCovers<std::string>(elements, edges, IsSubsequenceStrict, err)) return false;

            const std::string base = GraphBaseDir();
            SaveTextFile(JoinPath(base, "T10_BIO_single_letters.dot"),
                         HasseBuilder::ToDot<std::string>(elements, edges, [](const std::string& s){ return s; }));

            auto vertices = VerticesFromHasse<std::string>(elements, edges, [](const std::string& s){ return s; });
            SaveHassePngHidden(vertices, edges, JoinPath(base, "T10_BIO_single_letters.png").c_str());

            return edges.empty();
        }
    };
}

// Тест 11: BIOINFO
inline TestCase T11_Bio_Chain20_ScoreSanity() {
    return {
        "T11_BIO_chain20_score",
        [](std::string* err) -> bool {
            try { readCSV(); }
            catch (const std::exception& e) {
                if (err) *err = std::string("readCSV failed: ") + e.what();
                return false;
            }

            std::string cur;
            std::vector<std::string> elements;
            elements.reserve(AllAminoAcids.size());

            for (char c : AllAminoAcids) {
                cur.push_back(c);
                elements.push_back(cur);
            }

            for (const auto& s : elements) {
                if (!CheckSeq(s)) {
                    if (err) *err = "Invalid amino sequence in chain: " + s;
                    return false;
                }
            }

            auto cmp = HasseComparators::FromPartialOrder<std::string>(IsSubsequenceStrict);
            auto edges = HasseBuilder::BuildHasseEdges(elements, cmp);

            if ((int)edges.size() != ((int)elements.size() - 1)) {
                if (err) *err = "Expected chain edges = N-1 for BIO chain";
                return false;
            }
            if (!VerifyCovers<std::string>(elements, edges, IsSubsequenceStrict, err)) return false;

            std::set<std::pair<int,int>> expected;
            for (int i = 0; i + 1 < (int)elements.size(); ++i) expected.insert({i, i+1});
            std::set<std::pair<int,int>> got(edges.begin(), edges.end());
            if (got != expected) {
                if (err) *err = "BIO chain edges mismatch";
                return false;
            }

            const std::string s = elements[4];
            int diag = 0;
            for (char ch : s) diag += score(ch, ch);
            int gotScore = Score(s, s);
            if (gotScore != diag) {
                if (err) *err = "Score(seq,seq) != sum(score(c,c)) for '" + s + "'";
                return false;
            }

            const std::string base = GraphBaseDir();
            SaveTextFile(JoinPath(base, "T11_BIO_chain20_score.dot"),
                         HasseBuilder::ToDot<std::string>(elements, edges, [](const std::string& x){ return x; }));
            auto vertices = VerticesFromHasse<std::string>(elements, edges, [](const std::string& x){ return x; });
            SaveHassePngHidden(vertices, edges, JoinPath(base, "T11_BIO_chain20_score.png").c_str());

            return true;
        }
    };
}


// 4) все тесты

    inline int RunAllCompact(std::ostream& out = std::cout) {
    std::vector<TestCase> cases = {
        T01_IntDivides(),
        T02_IntLess(),
        T03_StringPrefix(),
        T04_StringSubsequence(),
        T05_StringLex(),
        T06_SetIntSubset(),
        T07_Point2D(),
        T08_Bioinfo(),
        T09_IntDivides_235(),
        T10_Bio_SingleLetters_NoEdges(),
        T11_Bio_Chain20_ScoreSanity()
    };


    int passed = 0;
    for (auto& tc : cases) {
        std::string err;
        bool ok = false;
        try {
            ok = tc.run(&err);
        } catch (...) {
            ok = false;
        }
        if (ok) ++passed;
    }

    out << "Passed " << passed << "/" << (int)cases.size() << "\n";
    out << "Graphs saved to: " << GraphBaseDir() << "\n";
    return passed == (int)cases.size() ? 0 : 1;
}

inline int RunAll(std::ostream& out = std::cout) {
    std::vector<TestCase> cases = {
        T01_IntDivides(),
        T02_IntLess(),
        T03_StringPrefix(),
        T04_StringSubsequence(),
        T05_StringLex(),
        T06_SetIntSubset(),
        T07_Point2D(),
        T08_Bioinfo()
    };

    int passed = 0;
    out << "Running " << cases.size() << " tests...\n";

    for (auto& tc : cases) {
        std::string err;
        bool ok = false;
        try {
            ok = tc.run(&err);
        } catch (const std::exception& e) {
            err = e.what();
            ok = false;
        } catch (...) {
            err = "unknown exception";
            ok = false;
        }

        if (ok) {
            ++passed;
            out << "[PASS] " << tc.name << "\n";
        } else {
            out << "[FAIL] " << tc.name;
            if (!err.empty()) out << " :: " << err;
            out << "\n";
        }
    }

    out << "Passed " << passed << " / " << (int)cases.size() << "\n";
    out << "Graphs saved to: " << GraphBaseDir() << "\n";
    return passed == (int)cases.size() ? 0 : 1;
}
}

#endif
