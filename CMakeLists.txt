cmake_minimum_required(VERSION 3.20)

project(LabaAvtomat)

set(CMAKE_CXX_STANDARD 20)

find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(GLUT REQUIRED)

if (NOT TARGET glfw3::glfw)
    add_library(glfw3::glfw ALIAS glfw)
endif()

add_executable(LabaAvtomat main.cpp)

target_link_libraries(LabaAvtomat
        OpenGL::GL
        glfw3::glfw
        GLUT::GLUT
)
