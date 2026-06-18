cmake_minimum_required(VERSION 3.20)
project(MarioContra2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SFML_DIR "C:/SFML/SFML-3.0.0/lib/cmake/SFML")

# UWAGA: ustaw SFML_DIR jako zmienną środowiskową lub zmień tę ścieżkę
# Przykład: C:/SFML/SFML-3.0.0/lib/cmake/SFML
if(DEFINED ENV{SFML_DIR})
    set(SFML_DIR $ENV{SFML_DIR})
else()
    set(SFML_DIR "C:/SFML/SFML-3.0.0/lib/cmake/SFML")
endif()

find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System Audio)

# Pliki zrodlowe
file(GLOB SOURCES "*.cpp")

add_executable(MarioContra2 ${SOURCES})


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
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_RUNTIME_DLLS:MarioContra2>
        $<TARGET_FILE_DIR:MarioContra2>
    COMMAND_EXPAND_LISTS
)

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

add_custom_command(TARGET MarioContra2 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/level2.txt"
        "$<TARGET_FILE_DIR:MarioContra2>/level2.txt"
    COMMENT "Kopiowanie level1.txt -> katalog exe"
)