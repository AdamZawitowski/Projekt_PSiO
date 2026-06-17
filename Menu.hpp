#pragma once
#include <SFML/Graphics.hpp>
#include <optional>
#include <string>
#include <vector>
#include "AppState.hpp"

// ================================================================== //
//  Menu — odpowiada za stan AppState::MainMenu oraz AppState::NameInput //
// ================================================================== //
//
//  Zarzadza:
//   - renderowaniem opcji glownego menu ("Nowa Gra", "Tablica Wynikow",
//     "Wyjdz") wraz z podswietleniem aktualnie wybranej opcji,
//   - nawigacja klawiatura (strzalki / W,S) i zatwierdzaniem (Enter),
//   - ekranem wpisywania nicku gracza (TextEntered) po wybraniu
//     "Nowa Gra" — po zatwierdzeniu Enterem nick trafia do referencji
//     playerName, a stan aplikacji zmienia sie na AppState::Gameplay.
//
class Menu {
public:
    Menu();

    // Wczytuje czcionke z podanej sciezki. Zwraca true w razie sukcesu.
    bool loadFont(const std::string& fontPath);

    // Obsluga zdarzen — wywolywana z main.cpp w petli eventow,
    // gdy currentState == AppState::MainMenu lub AppState::NameInput.
    // Moze zmienic currentState (np. na NameInput, Gameplay, lub
    // zasygnalizowac wyjscie poprzez zwrocenie informacji do main.cpp).
    void handleEvent(const sf::Event& event, AppState& currentState, std::string& playerName);

    // Logika niezalezna od zdarzen (np. animacja migajacego kursora
    // przy wpisywaniu nicku). dt w sekundach.
    void update(float dt);

    // Rysowanie aktualnego ekranu menu (MainMenu lub NameInput).
    // Funkcja sama rozpoznaje, ktory ekran rysowac, na podstawie
    // wewnetrznego stanu ustawionego przez handleEvent.
    void draw(sf::RenderWindow& window);

    // Czy uzytkownik zazadal zamkniecia aplikacji (wybral "Wyjdz").
    bool wantsToExit() const { return m_exitRequested; }

    // Resetuje menu do stanu poczatkowego (np. po powrocie z gry).
    void reset();

private:
    enum class MenuScreen {
        Options,    // lista opcji: Nowa Gra / Tablica Wynikow / Wyjdz
        NameEntry   // ekran wpisywania nicku
    };

    enum class MenuOption : int {
        NewGame = 0,
        Leaderboard = 1,
        Quit = 2,
        Count = 3
    };

    void handleOptionsEvent(const sf::Event& event, AppState& currentState);
    void handleNameInputEvent(const sf::Event& event, AppState& currentState, std::string& playerName);

    void drawOptions(sf::RenderWindow& window);
    void drawNameInput(sf::RenderWindow& window);

    sf::Text makeText(const std::string& str, unsigned size) const;

    sf::Font m_font;
    bool     m_fontLoaded = false;

    MenuScreen m_screen = MenuScreen::Options;

    // --- Stan listy opcji ---
    std::vector<std::string> m_optionLabels;
    int m_selectedIndex = 0;

    // --- Stan wpisywania nicku ---
    std::string m_nameBuffer;
    static constexpr std::size_t MAX_NAME_LENGTH = 12;

    // Migajacy kursor przy wpisywaniu nicku
    float m_cursorBlinkTimer = 0.f;
    bool  m_cursorVisible = true;
    static constexpr float CURSOR_BLINK_INTERVAL = 0.5f;

    bool m_exitRequested = false;
};