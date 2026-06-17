#include "Leaderboard.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>

Leaderboard::Leaderboard() {}

bool Leaderboard::loadFont(const std::string& fontPath) {
    m_fontLoaded = m_font.openFromFile(fontPath);
    return m_fontLoaded;
}

// ====================================================================
//  Plik wynikow
// ====================================================================
//
//  Format pliku — jedna linia na wpis, pola rozdzielone ';':
//      nick;levelsPassed;points
//
//  Wybrany prosty, czytelny format tekstowy (latwy do debugowania
//  recznie w edytorze), a nie np. binarny — przy max. 5 wpisach
//  wydajnosc nie ma znaczenia.
//
void Leaderboard::loadFromFile(const std::string& filename) {
    m_scores.clear();

    std::ifstream file(filename);
    if (!file.is_open()) {
        // Brak pliku (np. pierwsze uruchomienie gry) — to nie jest blad,
        // po prostu zaczynamy z pusta tablica wynikow.
        std::cerr << "[Leaderboard] brak pliku wynikow: " << filename << "\n";
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string namePart, levelsPart, pointsPart;

        if (!std::getline(ss, namePart, ';'))   continue;
        if (!std::getline(ss, levelsPart, ';')) continue;
        if (!std::getline(ss, pointsPart, ';')) continue;

        ScoreEntry entry;
        entry.name = namePart;
        try {
            entry.levelsPassed = std::stoi(levelsPart);
            entry.points       = std::stoi(pointsPart);
        } catch (const std::exception&) {
            // Uszkodzona linia — ignorujemy ten wpis, nie przerywamy wczytywania.
            continue;
        }

        m_scores.push_back(entry);
    }

    sortAndTrim();
}

void Leaderboard::saveToFile(const std::string& filename) const {
    std::ofstream file(filename, std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "[Leaderboard] nie mozna zapisac pliku wynikow: " << filename << "\n";
        return;
    }

    for (const ScoreEntry& entry : m_scores) {
        file << entry.name << ';' << entry.levelsPassed << ';' << entry.points << '\n';
    }
}

// ====================================================================
//  Dodawanie wyniku
// ====================================================================
void Leaderboard::addScore(const std::string& name, int levels, int pts) {
    ScoreEntry entry;
    entry.name         = name;
    entry.levelsPassed = levels;
    entry.points       = pts;

    m_scores.push_back(entry);
    sortAndTrim();
}

void Leaderboard::sortAndTrim() {
    std::sort(m_scores.begin(), m_scores.end(),
        [](const ScoreEntry& a, const ScoreEntry& b) {
            if (a.points != b.points)
                return a.points > b.points;          // glownie liczy punktacja
            return a.levelsPassed > b.levelsPassed;   // remis: wiecej poziomow wyzej
        });

    if (m_scores.size() > MAX_ENTRIES)
        m_scores.resize(MAX_ENTRIES);
}

// ====================================================================
//  Eventy
// ====================================================================
void Leaderboard::handleEvent(const sf::Event& event, AppState& currentState) {
    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        if (key->code == sf::Keyboard::Key::Escape) {
            currentState = AppState::MainMenu;
        }
    }
}

// ====================================================================
//  Rysowanie
// ====================================================================
sf::Text Leaderboard::makeText(const std::string& str, unsigned size) const {
    sf::Text text(m_font, str, size);
    return text;
}

void Leaderboard::draw(sf::RenderWindow& window) const {
    if (!m_fontLoaded) return;

    const sf::Vector2u winSize = window.getSize();
    const float centerX = winSize.x / 2.f;

    // --- Tlo ---
    sf::RectangleShape bg(sf::Vector2f(static_cast<float>(winSize.x), static_cast<float>(winSize.y)));
    bg.setFillColor(sf::Color(20, 20, 40));
    window.draw(bg);

    // --- Tytul ---
    sf::Text title = makeText("TABLICA WYNIKOW", 38);
    title.setFillColor(sf::Color::Yellow);
    title.setStyle(sf::Text::Bold);
    {
        sf::FloatRect tb = title.getLocalBounds();
        title.setOrigin({ tb.position.x + tb.size.x / 2.f, tb.position.y + tb.size.y / 2.f });
        title.setPosition({ centerX, 70.f });
    }
    window.draw(title);

    // --- Ramka tabeli ---
    const float tableTop    = 120.f;
    const float tableWidth  = 560.f;
    const float rowHeight   = 42.f;
    const float headerHeight = 36.f;
    const float tableLeft   = centerX - tableWidth / 2.f;

    sf::RectangleShape tableBg(sf::Vector2f(tableWidth, headerHeight + rowHeight * MAX_ENTRIES));
    tableBg.setPosition({ tableLeft, tableTop });
    tableBg.setFillColor(sf::Color(0, 0, 0, 120));
    tableBg.setOutlineColor(sf::Color(255, 215, 0));
    tableBg.setOutlineThickness(2.f);
    window.draw(tableBg);

    // Kolumny: # | NICK | POZIOMY | PUNKTY
    const float colRank   = tableLeft + 20.f;
    const float colName   = tableLeft + 70.f;
    const float colLevels = tableLeft + 320.f;
    const float colPoints = tableLeft + 430.f;

    auto drawCell = [&](const std::string& str, float x, float y, sf::Color color, bool bold) {
        sf::Text cell = makeText(str, 20);
        cell.setFillColor(color);
        if (bold) cell.setStyle(sf::Text::Bold);
        cell.setPosition({ x, y });
        window.draw(cell);
    };

    // --- Naglowek tabeli ---
    const float headerY = tableTop + 6.f;
    drawCell("#",      colRank,   headerY, sf::Color(180, 180, 180), true);
    drawCell("NICK",   colName,   headerY, sf::Color(180, 180, 180), true);
    drawCell("POZIOMY", colLevels, headerY, sf::Color(180, 180, 180), true);
    drawCell("PUNKTY", colPoints, headerY, sf::Color(180, 180, 180), true);

    // Linia oddzielajaca naglowek od wierszy
    sf::RectangleShape headerLine(sf::Vector2f(tableWidth - 4.f, 2.f));
    headerLine.setPosition({ tableLeft + 2.f, tableTop + headerHeight });
    headerLine.setFillColor(sf::Color(255, 215, 0, 150));
    window.draw(headerLine);

    // --- Wiersze wynikow (zawsze 5 miejsc, puste jesli brak wyniku) ---
    for (std::size_t i = 0; i < MAX_ENTRIES; ++i) {
        const float rowY = tableTop + headerHeight + static_cast<float>(i) * rowHeight + 8.f;

        // Subtelne podswietlenie pierwszego miejsca
        sf::Color rowColor = sf::Color::White;
        if (i == 0) rowColor = sf::Color(255, 215, 0);       // zloto — 1. miejsce
        else if (i == 1) rowColor = sf::Color(210, 210, 230); // srebro — 2. miejsce
        else if (i == 2) rowColor = sf::Color(205, 150, 100); // brąz — 3. miejsce

        drawCell(std::to_string(i + 1) + ".", colRank, rowY, rowColor, i < 3);

        if (i < m_scores.size()) {
            const ScoreEntry& entry = m_scores[i];
            drawCell(entry.name,                         colName,   rowY, rowColor, i < 3);
            drawCell(std::to_string(entry.levelsPassed),  colLevels, rowY, rowColor, i < 3);
            drawCell(std::to_string(entry.points),        colPoints, rowY, rowColor, i < 3);
        } else {
            drawCell("---", colName,   rowY, sf::Color(100, 100, 100), false);
            drawCell("-",   colLevels, rowY, sf::Color(100, 100, 100), false);
            drawCell("-",   colPoints, rowY, sf::Color(100, 100, 100), false);
        }
    }

    // --- Podpowiedz wyjscia ---
    sf::Text hint = makeText("Esc - powrot do menu", 16);
    hint.setFillColor(sf::Color(180, 180, 180));
    {
        sf::FloatRect hb = hint.getLocalBounds();
        hint.setOrigin({ hb.position.x + hb.size.x / 2.f, hb.position.y + hb.size.y / 2.f });
        hint.setPosition({ centerX, tableTop + headerHeight + rowHeight * MAX_ENTRIES + 40.f });
    }
    window.draw(hint);
}