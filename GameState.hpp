#pragma once

// Stan gry — uzywany w main.cpp do przelaczania ekranow
enum class GameState {
    Playing,   // normalna rozgrywka
    GameOver,  // gracz stracil wszystkie zycia
    Win        // gracz dotknał flagi
};
