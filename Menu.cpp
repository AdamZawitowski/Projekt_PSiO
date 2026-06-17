#include "Menu.hpp"

Menu::Menu() {
    m_optionLabels = {
        "1. Nowa Gra",
        "2. Tablica Wynikow",
        "3. Wyjdz"
    };
}

bool Menu::loadFont(const std::string& fontPath) {
    m_fontLoaded = m_font.openFromFile(fontPath);
    return m_fontLoaded;
}

void Menu::reset() {
    m_screen = MenuScreen::Options;
    m_selectedIndex = 0;
    m_nameBuffer.clear();
    m_cursorBlinkTimer = 0.f;
    m_cursorVisible = true;
    m_exitRequested = false;
}

// ====================================================================
//  Eventy
// ====================================================================
void Menu::handleEvent(const sf::Event& event, AppState& currentState, std::string& playerName) {
    switch (m_screen) {
        case MenuScreen::Options:
            handleOptionsEvent(event, currentState);
            break;
        case MenuScreen::NameEntry:
            handleNameInputEvent(event, currentState, playerName);
            break;
    }
}

void Menu::handleOptionsEvent(const sf::Event& event, AppState& currentState) {
    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        const int count = static_cast<int>(MenuOption::Count);

        if (key->code == sf::Keyboard::Key::Up || key->code == sf::Keyboard::Key::W) {
            m_selectedIndex = (m_selectedIndex - 1 + count) % count;
        }
        else if (key->code == sf::Keyboard::Key::Down || key->code == sf::Keyboard::Key::S) {
            m_selectedIndex = (m_selectedIndex + 1) % count;
        }
        else if (key->code == sf::Keyboard::Key::Enter) {
            switch (static_cast<MenuOption>(m_selectedIndex)) {
                case MenuOption::NewGame:
                    // Przejscie do ekranu wpisywania nicku
                    m_screen = MenuScreen::NameEntry;
                    m_nameBuffer.clear();
                    currentState = AppState::NameInput;
                    break;

                case MenuOption::Leaderboard:
                    currentState = AppState::Leaderboard;
                    break;

                case MenuOption::Quit:
                    m_exitRequested = true;
                    break;

                default:
                    break;
            }
        }

        // Obsluga szybkiego wyboru cyframi 1/2/3 (opcjonalnie, zgodnie z etykietami)
        else if (key->code == sf::Keyboard::Key::Num1) {
            m_selectedIndex = static_cast<int>(MenuOption::NewGame);
        }
        else if (key->code == sf::Keyboard::Key::Num2) {
            m_selectedIndex = static_cast<int>(MenuOption::Leaderboard);
        }
        else if (key->code == sf::Keyboard::Key::Num3) {
            m_selectedIndex = static_cast<int>(MenuOption::Quit);
        }
    }
}

void Menu::handleNameInputEvent(const sf::Event& event, AppState& currentState, std::string& playerName) {
    // Obsluga wpisywania znakow
    if (const auto* text = event.getIf<sf::Event::TextEntered>()) {
        // 8 = Backspace, 13 = Enter (CR) — filtrujemy znaki kontrolne,
        // Backspace i Enter obslugujemy osobno przez KeyPressed/unicode.
        const char32_t unicode = text->unicode;

        if (unicode == 8) { // Backspace
            if (!m_nameBuffer.empty())
                m_nameBuffer.pop_back();
        }
        else if (unicode >= 32 && unicode < 127) { // drukowalne ASCII
            if (m_nameBuffer.size() < MAX_NAME_LENGTH)
                m_nameBuffer.push_back(static_cast<char>(unicode));
        }
        // Znaki >= 128 (np. polskie diakrytyki w UTF-8/inne kodowania)
        // ignorujemy tutaj dla prostoty renderowania z domyslna czcionka.
    }

    // Obsluga klawiszy specjalnych: Enter (zatwierdzenie), Escape (powrot)
    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        if (key->code == sf::Keyboard::Key::Enter) {
            if (!m_nameBuffer.empty()) {
                playerName = m_nameBuffer;
                m_screen = MenuScreen::Options; // reset ekranu na przyszlosc
                currentState = AppState::Gameplay;
            }
        }
        else if (key->code == sf::Keyboard::Key::Escape) {
            // Powrot do listy opcji bez zatwierdzania nicku
            m_screen = MenuScreen::Options;
            currentState = AppState::MainMenu;
        }
    }
}

// ====================================================================
//  Update
// ====================================================================
void Menu::update(float dt) {
    if (m_screen == MenuScreen::NameEntry) {
        m_cursorBlinkTimer += dt;
        if (m_cursorBlinkTimer >= CURSOR_BLINK_INTERVAL) {
            m_cursorBlinkTimer = 0.f;
            m_cursorVisible = !m_cursorVisible;
        }
    }
}

// ====================================================================
//  Draw
// ====================================================================
sf::Text Menu::makeText(const std::string& str, unsigned size) const {
    sf::Text text(m_font, str, size);
    return text;
}

void Menu::draw(sf::RenderWindow& window) {
    if (!m_fontLoaded) return;

    switch (m_screen) {
        case MenuScreen::Options:
            drawOptions(window);
            break;
        case MenuScreen::NameEntry:
            drawNameInput(window);
            break;
    }
}

void Menu::drawOptions(sf::RenderWindow& window) {
    const sf::Vector2u winSize = window.getSize();

    // Tlo
    sf::RectangleShape bg(sf::Vector2f(static_cast<float>(winSize.x), static_cast<float>(winSize.y)));
    bg.setFillColor(sf::Color(20, 20, 40));
    window.draw(bg);

    // Tytul
    sf::Text title = makeText("MARIO CONTRA 2", 42);
    title.setFillColor(sf::Color::White);
    title.setStyle(sf::Text::Bold);
    {
        sf::FloatRect tb = title.getLocalBounds();
        title.setOrigin({ tb.position.x + tb.size.x / 2.f, tb.position.y + tb.size.y / 2.f });
        title.setPosition({ winSize.x / 2.f, winSize.y / 2.f - 120.f });
    }
    window.draw(title);

    // Opcje
    const float startY = winSize.y / 2.f - 20.f;
    const float spacing = 44.f;

    for (std::size_t i = 0; i < m_optionLabels.size(); ++i) {
        const bool selected = (static_cast<int>(i) == m_selectedIndex);

        sf::Text optionText = makeText(m_optionLabels[i], 26);
        optionText.setFillColor(selected ? sf::Color::Yellow : sf::Color::White);
        optionText.setStyle(selected ? sf::Text::Bold : sf::Text::Regular);

        sf::FloatRect ob = optionText.getLocalBounds();
        optionText.setOrigin({ ob.position.x + ob.size.x / 2.f, ob.position.y + ob.size.y / 2.f });
        optionText.setPosition({ winSize.x / 2.f, startY + static_cast<float>(i) * spacing });

        window.draw(optionText);

        if (selected) {
            // Prosty wskaznik wyboru ">" po lewej stronie opcji
            sf::Text arrow = makeText(">", 26);
            arrow.setFillColor(sf::Color::Yellow);
            arrow.setStyle(sf::Text::Bold);
            sf::FloatRect ab = arrow.getLocalBounds();
            arrow.setOrigin({ ab.position.x + ab.size.x / 2.f, ab.position.y + ab.size.y / 2.f });
            arrow.setPosition({ winSize.x / 2.f - ob.size.x / 2.f - 24.f,
                                startY + static_cast<float>(i) * spacing });
            window.draw(arrow);
        }
    }
}

void Menu::drawNameInput(sf::RenderWindow& window) {
    const sf::Vector2u winSize = window.getSize();

    sf::RectangleShape bg(sf::Vector2f(static_cast<float>(winSize.x), static_cast<float>(winSize.y)));
    bg.setFillColor(sf::Color(20, 20, 40));
    window.draw(bg);

    sf::Text prompt = makeText("Wpisz swoje imie:", 28);
    prompt.setFillColor(sf::Color::White);
    {
        sf::FloatRect pb = prompt.getLocalBounds();
        prompt.setOrigin({ pb.position.x + pb.size.x / 2.f, pb.position.y + pb.size.y / 2.f });
        prompt.setPosition({ winSize.x / 2.f, winSize.y / 2.f - 60.f });
    }
    window.draw(prompt);

    // Pole tekstowe z migajacym kursorem "_"
    std::string displayed = m_nameBuffer;
    if (m_cursorVisible) displayed += "_";

    sf::Text nameText = makeText(displayed, 32);
    nameText.setFillColor(sf::Color::Yellow);
    nameText.setStyle(sf::Text::Bold);
    {
        sf::FloatRect nb = nameText.getLocalBounds();
        nameText.setOrigin({ nb.position.x + nb.size.x / 2.f, nb.position.y + nb.size.y / 2.f });
        nameText.setPosition({ winSize.x / 2.f, winSize.y / 2.f });
    }
    window.draw(nameText);

    sf::Text hint = makeText("Enter - zatwierdz   /   Esc - wstecz", 16);
    hint.setFillColor(sf::Color(180, 180, 180));
    {
        sf::FloatRect hb = hint.getLocalBounds();
        hint.setOrigin({ hb.position.x + hb.size.x / 2.f, hb.position.y + hb.size.y / 2.f });
        hint.setPosition({ winSize.x / 2.f, winSize.y / 2.f + 70.f });
    }
    window.draw(hint);
}