#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include "AppState.hpp"

// ================================================================== //
//  Leaderboard — odpowiada za stan AppState::Leaderboard               //
// ================================================================== //
//
//  UWAGA NAZEWNICTWA: w architekturze ustalonej wczesniej, ogolny stan
//  aplikacji (menu / gra / pauza / leaderboard) nazywa sie AppState
//  (plik AppState.h), aby nie kolidowac z juz istniejacym w projekcie
//  GameState.hpp (Playing/GameOver/Win, uzywany wewnatrz rozgrywki).
//  Ta klasa zarzadza wiec stanem AppState::Leaderboard.
//
//  Zarzadza:
//   - lista top 5 wynikow (ScoreEntry: nick, liczba przejscia poziomow,
//     punkty), zawsze sortowana od najlepszego,
//   - zapisem/wczytywaniem wynikow z pliku tekstowego,
//   - rysowaniem tabeli wynikow na ekranie,
//   - obsluga wyjscia: Escape wraca do AppState::MainMenu.
//
class Leaderboard {
public:
    struct ScoreEntry {
        std::string name;
        int levelsPassed;
        int points;
    };

    Leaderboard();

    // Wczytuje czcionke z podanej sciezki. Zwraca true w razie sukcesu.
    bool loadFont(const std::string& fontPath);

    // Wczytuje liste wynikow z pliku. Jesli plik nie istnieje lub jest
    // uszkodzony, lista zostaje pusta (bez przerywania dzialania gry).
    void loadFromFile(const std::string& filename);

    // Zapisuje aktualna liste wynikow (maks. top 5) do pliku.
    void saveToFile(const std::string& filename) const;

    // Dodaje nowy wynik, sortuje liste od najlepszego (wg punktow,
    // a przy remisie wg liczby przebytych poziomow) i obcina do top 5.
    void addScore(const std::string& name, int levels, int pts);

    // Obsluga zdarzen — wywolywana z main.cpp, gdy currentState ==
    // AppState::Leaderboard. Escape przelacza currentState na MainMenu.
    void handleEvent(const sf::Event& event, AppState& currentState);

    // Rysowanie tabeli wynikow.
    void draw(sf::RenderWindow& window) const;

    const std::vector<ScoreEntry>& getScores() const { return m_scores; }

private:
    static constexpr std::size_t MAX_ENTRIES = 5;

    void sortAndTrim();
    sf::Text makeText(const std::string& str, unsigned size) const;

    std::vector<ScoreEntry> m_scores;

    sf::Font m_font;
    bool     m_fontLoaded = false;
};