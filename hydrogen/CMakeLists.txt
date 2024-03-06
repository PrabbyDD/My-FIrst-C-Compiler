cmake_minimum_required(VERSION 3.25)
project(hydrogen)

set(CMAKE_CXX_STANDARD 17)

add_executable(hydro src/main.cpp
        src/tokenization.hpp
        src/parser.hpp
        src/generation.hpp
        src/arena.hpp)
