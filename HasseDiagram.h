#ifndef AUTOLABA_HASSE_DIAGRAM_H
#define AUTOLABA_HASSE_DIAGRAM_H

#include <algorithm>
#include <functional>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>


enum class Cmp { Less, Equal, Greater, Incomparable };

template<typename T>
using Comparator = std::function<Cmp(const T&, const T&)>;

template<typename T>
using ToStringFunc = std::function<std::string(const T&)>;

namespace detail {
    template<typename T, typename = void>
    struct is_ostreamable : std::false_type {};

    template<typename T>
    struct is_ostreamable<T, std::void_t<decltype(std::declval<std::ostream&>() << std::declval<const T&>())>>
        : std::true_type {};

    // красивый вывод сета
    inline std::string join_int_vector(const std::vector<int>& v) {
        std::ostringstream out;
        out << "{";
        for (size_t i = 0; i < v.size(); ++i) {
            if (i) out << ", ";
            out << v[i];
        }
        out << "}";
        return out.str();
    }

    // дефолтная функция для перевода в строчку 
    template<typename T>
    std::string defaultToString(const T& val) {
        if constexpr (std::is_same_v<T, std::vector<int>>) {
            return join_int_vector(val);
        } else if constexpr (is_ostreamable<T>::value) {
            std::ostringstream out;
            out << val;
            return out.str();
        } else {
            return "<value>";
        }
    }
}

// возвращает функцию для сравнения 
namespace HasseComparators {
    template<typename T, typename LessFunc>
    Comparator<T> FromPartialOrder(LessFunc lessThan) {
        return [lessThan](const T& a, const T& b) -> Cmp {
            const bool ab = lessThan(a, b);
            const bool ba = lessThan(b, a);

            if (ab && ba) return Cmp::Incomparable;
            if (ab) return Cmp::Less;
            if (ba) return Cmp::Greater;
            return Cmp::Equal;
        };
    }
}

template<typename T>
class HasseDiagram {
public:
    using Edge = std::pair<int, int>;

    explicit HasseDiagram(Comparator<T> comparator) : cmp_(std::move(comparator)) {}

    void AddElements(const std::vector<T>& elements) { elements_ = elements; }

    // функция для получения матрицы отношений элементов
    void Build() {
        const int n = static_cast<int>(elements_.size());
        edges_.clear();
        if (n == 0) return;

        std::vector<std::vector<char>> le(n, std::vector<char>(n, 0));
        for (int i = 0; i < n; ++i) le[i][i] = 1;

        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                if (i == j) continue;
                const Cmp r = cmp_(elements_[i], elements_[j]);
                if (r == Cmp::Less) {
                    le[i][j] = 1;
                } else if (r == Cmp::Greater) {
                    le[j][i] = 1;
                }
            }
        }

        for (int k = 0; k < n; ++k) {
            for (int i = 0; i < n; ++i) {
                if (!le[i][k]) continue;
                for (int j = 0; j < n; ++j) {
                    if (le[k][j]) le[i][j] = 1;
                }
            }
        }

        edges_.reserve(n * 2);
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                if (i == j) continue;
                if (!le[i][j] || le[j][i]) continue;

                bool has_middle = false;
                for (int k = 0; k < n; ++k) {
                    if (k == i || k == j) continue;
                    if (le[i][k] && le[k][j] && !(le[k][i] && le[i][k]) && !(le[j][k] && le[k][j])) {
                        has_middle = true;
                        break;
                    }
                }
                if (!has_middle) edges_.emplace_back(i, j);
            }
        }
    }

    const std::vector<Edge>& GetEdges() const { return edges_; }

private:
    Comparator<T> cmp_;
    std::vector<T> elements_;
    std::vector<Edge> edges_;
};

#endif
