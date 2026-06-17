// Plik nazwany AppState.h (a nie GameState.h), poniewaz projekt juz posiada
// wlasny GameState.h z enumem { Playing, GameOver, Win } uzywanym w petli gry.
// Aby uniknac kolizji nazw plikow i enumow, stan calej aplikacji nazywa sie AppState.
#pragma once

// ================================================================== //
//  AppState — globalny stan calej aplikacji (menu / gra / pauza)      //
// ================================================================== //
//
//  UWAGA: Projekt mial juz wczesniej plik "GameState.h" z enumem
//  `enum class GameState { Playing, GameOver, Win }` opisujacym stan
//  WEWNATRZ rozgrywki (uzywany w main.cpp w petli gry). Aby uniknac
//  konfliktu nazw, ten nowy, "wyzszy poziom" stanu aplikacji nazywa
//  sie AppState. AppState::Gameplay odpowiada za caly dotychczasowy
//  stan Playing/GameOver/Win.
//
enum class AppState {
    MainMenu,    // ekran glownego menu
    NameInput,   // wpisywanie nicku gracza przed rozgrywka
    Gameplay,    // rozgrywka (caly dotychczasowy main loop gry)
    PauseMenu,   // menu pauzy w trakcie rozgrywki
    Leaderboard  // tablica wynikow
};