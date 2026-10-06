#ifndef AUTOLABA_DRAW_H
#define AUTOLABA_DRAW_H

#include <algorithm>
#include <map>
#include <vector>
#include <filesystem>
#include <iostream>

#define GLUT_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>
#include <GLUT/glut.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image_write.h"
#include "HasseBuilder.h"
#include "AminoAcids.h"



inline float Radius = 0.0f;

struct DrawVertex {
    int index = 0;
    std::string string;
    float x = 0.0f;
    float y = 0.0f;
};

// распределение элементов по уровням 
inline std::map<int, std::vector<int>> levelIndex(const std::vector<HasseBuilder::Edge> &edges, int n) {

    std::vector<std::vector<int>> adj(n);
    std::vector<int> indeg(n, 0);
    for (auto& e : edges) {
        adj[e.first].push_back(e.second);
        indeg[e.second]++;
    }
    std::queue<int> q;
    std::vector<int> level(n, 0);

    for (int i = 0; i < n; ++i)
        if (indeg[i] == 0)
            q.push(i);

    while (!q.empty()) {
        int u = q.front(); q.pop();
        for (int v : adj[u]) {
            level[v] = std::max(level[v], level[u] + 1);
            if (--indeg[v] == 0)
                q.push(v);
        }
    }
    std::map<int, std::vector<int>> result;
    for (int i = 0; i < n; ++i)
        result[level[i]].push_back(i);
    return result;
}

// расчет значений для смещения по осям и определение радиуса
inline std::pair<float, float> CountSteps(const std::map<int, std::vector<int>>& levels) {
    const int levelCount = static_cast<int>(levels.size());
    const float yStep = 2.0f / static_cast<float>(levelCount + 1);
    int maxInLevel = 1;
    for (auto& p : levels)
        maxInLevel = std::max(maxInLevel, static_cast<int>(p.second.size()));

    const float xStepMin = 2.0f / static_cast<float>(maxInLevel + 1);
    const float outRadius = 0.35f * std::min(xStepMin, yStep);
    std::pair<float, float> result;
    result.first = yStep;
    result.second = outRadius;
    return result;
}

// распределение всех вершин по уровням через структуру DrawVertex
template <typename T>
std::vector<DrawVertex> VerticesFromHasse(const std::vector<T>& elements, const std::vector<HasseBuilder::Edge> &edges, ToStringFunc<T> toString = nullptr) {
    std::vector<DrawVertex> vertices;
    std::map<int, std::vector<int>> levels = levelIndex(edges, static_cast<int>(elements.size()));
    std::pair<float, float> counts = CountSteps(levels);
    Radius = counts.second;
    if (!toString) {
        toString = [](const T& v) { return detail::defaultToString(v); };
    }

    for (const auto& pair : levels) {
        for (int i = 0; i < pair.second.size(); i++) {
            DrawVertex vertex;
            vertex.index = pair.second[i];
            vertex.string = toString(elements[pair.second[i]]);
            vertex.y = -1.0f + static_cast<float>(pair.first + 1) * counts.first;
            vertex.x = -1.0f + static_cast<float>(i + 1) * (2.0f / static_cast<float>(pair.second.size() + 1));
            vertices.push_back(vertex);
        }
    }
    return vertices;
}

// отрисовка кругов для вершин диаграммы
inline void drawCircle(const float cx, const float cy, const float r, const int segments = 24) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);

    for (int i = 0; i <= segments; ++i) {
        float a = 2.0f * 3.1415926f * static_cast<float>(i) / static_cast<float>(segments);
        float x = cx + r * std::cos(a);
        float y = cy + r * std::sin(a);
        glVertex2f(x, y);
    }
    glEnd();
}
// отрисовка текста для подписи вершин
inline void drawText(float x, float y, const char* text) {
    glRasterPos2f(x, y);
    for (const char* c = text; *c != '\0'; ++c) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
}

// получение ширины текста для отцентровки текста
inline int getTextWidth(const char* text, void* font) {
    int width = 0;
    for (const char* c = text; *c != '\0'; ++c)
        width += glutBitmapWidth(font, *c);
    return width;
}
// визуализация текста с условием центровки
inline void drawTextCentered(float x, float y, const char* text, void* font = GLUT_BITMAP_TIMES_ROMAN_24) {
    int w = getTextWidth(text, font);
    float scaleX = 1.0f / 600.0f;
    float cx = x - (static_cast<float>(w) * scaleX) * 0.5f;
    glRasterPos2f(cx, y);
    for (const char* c = text; *c != '\0'; ++c)
        glutBitmapCharacter(font, *c);
}

// структура для хранения информации о выравнивании
struct AlignmentInfo {
    std::string from;
    std::string to;
    int score = 0;
    std::string align1;
    std::string align2;
};

// построение таблицы выравниваний
inline std::vector<AlignmentInfo> GetAlignments(const std::vector<HasseBuilder::Edge> &edges, const std::vector<DrawVertex>& vertices) {
    std::vector<AlignmentInfo> result;
    for (const auto& e : edges) {
        auto it1 = std::find_if(vertices.begin(), vertices.end(),
                [e](const DrawVertex& dv) { return dv.index == e.first; });
        auto it2 = std::find_if(vertices.begin(), vertices.end(),
            [e](const DrawVertex& dv) { return dv.index == e.second; });
        if (it1 != vertices.end() && it2 != vertices.end()) {
            AlignmentInfo info;
            info.from = (*it1).string;
            info.to = (*it2).string;
            info.score = Score(info.from, info.to);
            auto aligned = traceBack(DP(info.from, info.to), info.from, info.to);
            info.align1 = aligned.first;
            info.align2 = aligned.second;
            result.push_back(info);
        }
    }
    return result;
}

// отрисовка текста с заданным шрифтом
inline void drawTextWithFont(float x, float y, const char* text, void* font) {
    glRasterPos2f(x, y);
    for (const char* c = text; *c != '\0'; ++c) {
        glutBitmapCharacter(font, *c);
    }
}

// отрисовка панели с выравниваниями
inline void drawAlignmentPanel(const std::vector<AlignmentInfo>& alignments, float panelX) {
    void* fontTitle = GLUT_BITMAP_TIMES_ROMAN_24;
    void* fontText = GLUT_BITMAP_HELVETICA_18;

    float y = 0.92f;
    float lineHeight = 0.055f;
    float blockSpacing = 0.03f;

    glColor3f(0.2f, 0.2f, 0.6f);
    drawTextWithFont(panelX, y, "ALIGNMENTS", fontTitle);
    y -= lineHeight * 1.2f;

    glColor3f(0.7f, 0.7f, 0.7f);
    glBegin(GL_LINES);
    glVertex2f(panelX, y + 0.04f);
    glVertex2f(0.98f, y + 0.04f);
    glEnd();
    y -= 0.02f;

    glColor3f(0.0f, 0.0f, 0.0f);

    for (const auto& a : alignments) {
        if (y < -0.95f) break;

        glColor3f(0.1f, 0.4f, 0.1f);
        std::string header = a.from + " -> " + a.to;
        drawTextWithFont(panelX, y, header.c_str(), fontTitle);
        y -= lineHeight * 0.8f;

        glColor3f(0.5f, 0.2f, 0.0f);
        std::string scoreStr = "Score: " + std::to_string(a.score);
        drawTextWithFont(panelX, y, scoreStr.c_str(), fontText);
        y -= lineHeight * 0.9f;

        glColor3f(0.0f, 0.0f, 0.0f);
        std::string line1 = "  " + a.align1;
        std::string line2 = "  " + a.align2;
        drawTextWithFont(panelX, y, line1.c_str(), fontText);
        y -= lineHeight * 0.7f;
        drawTextWithFont(panelX, y, line2.c_str(), fontText);
        y -= lineHeight;

        y -= blockSpacing;
    }
}

// сохранение графа
inline void saveScreenshot(GLFWwindow* window, const char* filename, GLenum readBuf = GL_BACK) {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    glViewport(0, 0, width, height);

    std::vector<unsigned char> pixels(width * height * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    glFinish();
    glReadBuffer(readBuf);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    std::vector<unsigned char> flipped(width * height * 3);
    for (int y = 0; y < height; ++y) {
        std::memcpy(&flipped[y * width * 3],
                    &pixels[(height - 1 - y) * width * 3],
                    width * 3);
    }

    namespace fs = std::filesystem;
    try {
        fs::path p(filename);
        if (p.has_parent_path()) fs::create_directories(p.parent_path());
    } catch (...) {}

    int ok = stbi_write_png(filename, width, height, 3, flipped.data(), width * 3);
    if (!ok) std::cerr << "ERROR: cannot write png to: " << filename << "\n";
}



// вырисовка диаграммы Хассе
inline int DrawHasse(const std::vector<DrawVertex>& vertices,
                     const std::vector<HasseBuilder::Edge>& edges,
                     const char* outPngPath) {

    if (!glfwInit())
        return -1;

    GLFWwindow* window = glfwCreateWindow(600, 600, "HasseDiagram", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        bool savedOnce = false;
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        glColor3f(0.f, 0.f, 0.f);
        glBegin(GL_LINES);
        for (const auto& e : edges) {
            auto it1 = std::find_if(vertices.begin(), vertices.end(),
                [e](const DrawVertex& dv) { return dv.index == e.first; });
            auto it2 = std::find_if(vertices.begin(), vertices.end(),
                [e](const DrawVertex& dv) { return dv.index == e.second; });
            if (it1 != vertices.end() && it2 != vertices.end()) {
                const DrawVertex& parent = *it1;
                const DrawVertex& child  = *it2;
                glVertex2f(parent.x, parent.y);
                glVertex2f(child.x,  child.y);
            }
        }
        glEnd();

        for (const auto & vertice : vertices) {
            glColor3f(1.0f, 0.753f, 0.796f);
            drawCircle(vertice.x, vertice.y, Radius);
            glColor3f(0.f, 0.f, 0.f);
            drawTextCentered(vertice.x, vertice.y - (Radius + 0.05f), vertice.string.c_str());
        }
        glfwSwapBuffers(window);
        glfwPollEvents();
        saveScreenshot(window, "../Graph/screenshot.png");
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}


inline int SaveHassePngHidden(const std::vector<DrawVertex>& vertices,
                              const std::vector<HasseBuilder::Edge>& edges,
                              const char* outPngPath) {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(600, 600, "hidden", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);

    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    glViewport(0, 0, w, h);

    glClearColor(1.f, 1.f, 1.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    glColor3f(0.f, 0.f, 0.f);
    glBegin(GL_LINES);
    for (const auto& e : edges) {
        auto it1 = std::find_if(vertices.begin(), vertices.end(),
            [&](const DrawVertex& dv){ return dv.index == e.first; });
        auto it2 = std::find_if(vertices.begin(), vertices.end(),
            [&](const DrawVertex& dv){ return dv.index == e.second; });
        if (it1 != vertices.end() && it2 != vertices.end()) {
            glVertex2f(it1->x, it1->y);
            glVertex2f(it2->x, it2->y);
        }
    }
    glEnd();

    for (const auto& v : vertices) {
        glColor3f(1.0f, 0.753f, 0.796f);
        drawCircle(v.x, v.y, Radius);
        glColor3f(0.f, 0.f, 0.f);
        drawTextCentered(v.x, v.y - (Radius + 0.05f), v.string.c_str());
    }


    glfwSwapBuffers(window);
    saveScreenshot(window, outPngPath, GL_FRONT);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}



// создание смещённых вершин для Bio
inline std::vector<DrawVertex> ShiftVerticesForBio(const std::vector<DrawVertex>& vertices) {
    std::vector<DrawVertex> shifted;
    for (const auto& v : vertices) {
        DrawVertex sv = v;
        sv.x = -0.95f + (v.x + 1.0f) * (1.30f / 2.0f);
        shifted.push_back(sv);
    }
    return shifted;
}

// вырисовка хассе для биоинформатики
inline int DrawHasseBio(const std::vector<DrawVertex>& vertices, const std::vector<HasseBuilder::Edge> &edges) {
    if (!glfwInit())
        return -1;

    GLFWwindow* window = glfwCreateWindow(600, 600, "HasseDiagram - Bioinformatics", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    std::vector<DrawVertex> shiftedVertices = ShiftVerticesForBio(vertices);

    std::vector<AlignmentInfo> alignments = GetAlignments(edges, shiftedVertices);

    const float dividerX = 0.12f;
    const float panelX = 0.15f;

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        glColor3f(0.3f, 0.3f, 0.3f);
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        for (const auto& e : edges) {
            auto it1 = std::find_if(shiftedVertices.begin(), shiftedVertices.end(),
                [e](const DrawVertex& dv) { return dv.index == e.first; });
            auto it2 = std::find_if(shiftedVertices.begin(), shiftedVertices.end(),
                [e](const DrawVertex& dv) { return dv.index == e.second; });
            if (it1 != shiftedVertices.end() && it2 != shiftedVertices.end()) {
                glVertex2f(it1->x, it1->y);
                glVertex2f(it2->x, it2->y);
            }
        }
        glEnd();

        glColor3f(0.6f, 0.0f, 0.0f);
        for (const auto& e : edges) {
            auto it1 = std::find_if(shiftedVertices.begin(), shiftedVertices.end(),
                [e](const DrawVertex& dv) { return dv.index == e.first; });
            auto it2 = std::find_if(shiftedVertices.begin(), shiftedVertices.end(),
                [e](const DrawVertex& dv) { return dv.index == e.second; });

            if (it1 != shiftedVertices.end() && it2 != shiftedVertices.end()) {
                float dx = it2->x - it1->x;
                float dy = it2->y - it1->y;
                float len = std::sqrt(dx*dx + dy*dy);
                if (len > 0.001f) {
                    float nx = -dy / len;
                    float ny =  dx / len;
                    float mx = (it1->x + it2->x) * 0.5f;
                    float my = (it1->y + it2->y) * 0.5f;
                    float offset = 0.04f;
                    std::string label = std::to_string(Score(it1->string, it2->string));
                    drawTextCentered(mx + nx * offset, my + ny * offset, label.c_str(), GLUT_BITMAP_HELVETICA_18);
                }
            }
        }

        for (const auto& v : shiftedVertices) {
            glColor3f(1.0f, 0.753f, 0.796f);
            drawCircle(v.x, v.y, Radius * 0.7f);

            glColor3f(0.0f, 0.0f, 0.0f);
            drawTextCentered(v.x, v.y - (Radius + 0.015f), v.string.c_str(), GLUT_BITMAP_HELVETICA_18);
        }

        glColor3f(0.7f, 0.7f, 0.7f);
        glLineWidth(1.0f);
        glBegin(GL_LINES);
        glVertex2f(0.375f, -0.98f);
        glVertex2f(0.375f,  0.98f);
        glEnd();

        drawAlignmentPanel(alignments, 0.4f);

        glfwSwapBuffers(window);
        glfwPollEvents();
        saveScreenshot(window, "../Graph/screenshot.png");
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

#endif
