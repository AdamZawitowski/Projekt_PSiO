cmake_minimum_required(VERSION 3.20)
project(MarioContra2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Znajdz SFML 3
find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System Audio)

# Pliki zrodlowe
add_executable(MarioContra2
    main.cpp
    Player.cpp
    Enemy.cpp
    TileMap.cpp
)

# Linkuj SFML
target_link_libraries(MarioContra2 PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    SFML::Audio
)

# ---------------------------------------------------------------
# Kopiuj assets/ i level1.txt do folderu z exe po kazdym buildzie
# ---------------------------------------------------------------
add_custom_command(TARGET MarioContra2 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:MarioContra2>/assets"
    COMMENT "Kopiowanie assets/ -> katalog exe"
)

add_custom_command(TARGET MarioContra2 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/level1.txt"
        "$<TARGET_FILE_DIR:MarioContra2>/level1.txt"
    COMMENT "Kopiowanie level1.txt -> katalog exe"
)
