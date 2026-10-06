#ifndef AUTOLABA_AMINOACIDS_H
#define AUTOLABA_AMINOACIDS_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <iomanip>

inline std::map<char, int> AminoToIndex;
inline std::vector<std::vector<int>> scores;
inline std::string AllAminoAcids = "CSTAGPDEQNHRKMILVWYF";
inline int gap = -4;

// разделение хедера таблицы BLOSUM62
inline std::vector<char> splitToAminoNo1(const std::string& header) {
    std::vector<char> aminos;
    std::stringstream ss(header);
    std::string amino;
    bool flag = false;

    while (std::getline(ss, amino, ';')) {
        if (flag) {
            aminos.push_back(amino[0]);
        } else {
            flag = true;
        }
    }
    return aminos;
}
// разделение строк со значениями сравнения аминокислот
inline std::vector<int> splitToScoreNo1(const std::string& line) {
    std::vector<int> scores;
    std::stringstream ss(line);
    std::string score;
    bool flag = false;

    while (std::getline(ss, score, ';')) {
        if (flag) {
            scores.push_back(std::stoi(score));
        } else {
            flag = true;
        }
    }
    return scores;
}

// считывание таблицы сравнения аминокислот (таблицы BLOSUM62)
inline void readCSV() {
    std::ifstream file("BLOSUM62.csv");
    if (!file.is_open()) {
        file.open("../BLOSUM62.csv");
    }
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open BLOSUM62.csv - file not found");
    }

    std::string header;
    std::getline(file, header);
    std::vector<char> AminoAcids = splitToAminoNo1(header);

    for (int i = 0; i < AminoAcids.size(); i++) {
        AminoToIndex[AminoAcids[i]] = i;
    }

    std::string row;
    while (std::getline(file, row, '\n')) {
        scores.push_back(splitToScoreNo1(row));
    }
    file.close();
}

// нахождение значения для пары аминокислот по таблице BLOSUM62
inline int score(const char A, const char B) {
    int IndexA = AminoToIndex[A];
    int IndexB = AminoToIndex[B];
    return scores[IndexA][IndexB];
}

// проверка на принадлежность последовательности к аминокислотной
inline bool CheckSeq(const std::string& seq) {
    bool flag = true;
    for (char c : seq) {
        if (AllAminoAcids.find(c) == std::string::npos) {
            flag = false;
            break;
        }
    }
    return flag;
}

// написание динамического расчета для определения лучшего выравнивания для двух последовательностей
inline std::vector<std::vector<int>> DP(const std::string& seq1, const std::string& seq2) {
    std::vector<std::vector<int>> dp(seq1.size() + 1, std::vector<int>(seq2.size() + 1, 0));

    dp[0][0] = 0;
    for (int i = 1; i <= seq1.size(); i++) {
        dp[i][0] = gap;
    }
    for (int j = 1; j <= seq2.size(); j++) {
        dp[0][j] = gap;
    }

    for (int i = 1; i <= seq1.size(); i++) {
        for (int j = 1; j <= seq2.size(); j++) {
            dp[i][j] = std::max(dp[i - 1][j - 1] + score(seq1[i - 1], seq2[j - 1]), std::max(dp[i - 1][j] + gap, dp[i][j - 1] + gap));
        }
    }
    return dp;
}
inline int Score(const std::string& seq1, const std::string& seq2) {
    std::vector<std::vector<int>> dp = DP(seq1, seq2);
    return dp[seq1.size()][seq2.size()];
}

// вывод таблицы выравнивания последовательностей
inline void printDP(const std::vector<std::vector<int>>& dp, const std::string& seq1, const std::string& seq2) {
    const int W = 4;
    std::cout << std::setw(W) << ' ';
    std::cout << std::setw(W) << '-';
    for (char c : seq2) {
        std::cout << std::setw(W) << c;
    }
    std::cout << '\n';

    for (size_t i = 0; i < dp.size(); ++i) {
        if (i == 0)
            std::cout << std::setw(W) << '-';
        else
            std::cout << std::setw(W) << seq1[i - 1];

        for (int j : dp[i]) {
            std::cout << std::setw(W) << j;
        }
        std::cout << '\n';
    }
}

// возвращение выравненных последовательностей аминокислот
inline std::pair<std::string, std::string> traceBack(const std::vector<std::vector<int>>& dp, const std::string& seq1, const std::string& seq2) {
    std::pair<std::string, std::string> equal;
    size_t i = seq1.size();
    size_t j = seq2.size();
    while (i > 0 && j > 0) {
        if (dp[i][j] == dp[i - 1][j - 1] + score(seq1[i - 1], seq2[j - 1])) {
            equal.first.insert(0, 1, seq1[i - 1]);
            equal.second.insert(0, 1, seq2[j - 1]);
            i--;
            j--;
        } else if (dp[i][j] == dp[i - 1][j] + gap) {
            equal.first.insert(0, 1, seq1[i - 1]);
            equal.second.insert(0, 1, '-');
            i--;
        } else {
            equal.first.insert(0, 1, '-');
            equal.second.insert(0, 1, seq2[j - 1]);
            j--;
        }
    }
    while (i > 0) {
        equal.first.insert(0, 1, seq1[i - 1]);
        equal.second.insert(0, 1, '-');
        i--;
    }
    while (j > 0) {
        equal.first.insert(0, 1, '-');
        equal.second.insert(0, 1, seq2[j - 1]);
        j--;
    }
    return equal;
}

#endif
