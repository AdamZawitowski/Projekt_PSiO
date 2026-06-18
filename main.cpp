#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>
#include <array>
#include "Player.hpp"
#include "Enemy.hpp"
#include "TileMap.hpp"
#include "GoalFlag.hpp"
#include "GameState.hpp"
#include "Checkpoint.hpp"
#include "Item.hpp"
#include "Bullet.hpp"
#include "AppState.hpp"
#include "Menu.hpp"
#include "Leaderboard.hpp"
#include "BrickDebris.hpp"

namespace {

// ================================================================== //
//  Tekstury kafelkow                                                   //
// ================================================================== //
struct TileTextures {
    sf::Texture dirt, dirt2, brick, pipe, question;
    sf::Texture holeLeft, holeRight, holeMid;

    bool load() {
        bool ok = true;
        auto ld = [&](sf::Texture& t, const char* p) {
            if (!t.loadFromFile(p))
                { std::cerr << "[TileTextures] brak: " << p << "\n"; ok = false; }
        };
        ld(dirt,      "assets/dirt.png");
        ld(dirt2,     "assets/dirt_2.png");
        ld(brick,     "assets/brick.png");
        ld(pipe,      "assets/pipe.png");
        ld(question,  "assets/qest.png");
        ld(holeLeft,  "assets/hole.png");
        ld(holeRight, "assets/hole_2.png");
        ld(holeMid,   "assets/hole_3.png");
        return ok;
    }
};

// ================================================================== //
//  Pomocnicze: rysowanie mapy                                          //
// ================================================================== //
bool isTopGroundRow(const TileMap& tm, int col, int row) {
    return tm.getTile(col, row + 1) == TileType::Ground;
}


void drawTileSprite(sf::RenderWindow& w, const sf::Texture& tex, int col, int row) {
    const float TS = TileMap::TILE_SIZE;
    sf::Vector2u sz = tex.getSize();
    sf::Sprite sp(tex);
    sp.setPosition({ col * TS, row * TS });
    sp.setScale({ TS / sz.x, TS / sz.y });
    w.draw(sp);
}

void drawPitBottomRow(sf::RenderWindow& window,
                      const TileMap& tileMap,
                      const TileTextures& tex)
{
    const sf::Vector2u sizeInTiles = tileMap.getSizeInTiles();
    if (sizeInTiles.y == 0) return;

    const int row = static_cast<int>(sizeInTiles.y) - 1;
    int col = 0;

    while (col < static_cast<int>(sizeInTiles.x)) {
        if (tileMap.getTile(col, row) != TileType::Empty) { ++col; continue; }

        const int startCol = col;
        while (col < static_cast<int>(sizeInTiles.x) &&
               tileMap.getTile(col, row) == TileType::Empty)
            ++col;

        const int endCol = col - 1;
        for (int x = startCol; x <= endCol; ++x) {
            const sf::Texture* t = &tex.holeMid;
            if (x == startCol) t = &tex.holeLeft;
            if (x == endCol)   t = &tex.holeRight;
            if (startCol == endCol) t = &tex.holeLeft;
            drawTileSprite(window, *t, x, row);
        }
    }
}

void drawTileMap(sf::RenderWindow& window,
                 const TileMap& tileMap,
                 const TileTextures& tex)
{
    const sf::Vector2u sz = tileMap.getSizeInTiles();

    for (unsigned row = 0; row < sz.y; ++row) {
        for (unsigned col = 0; col < sz.x; ++col) {
            const int c = static_cast<int>(col);
            const int r = static_cast<int>(row);
            switch (tileMap.getTile(c, r)) {
            case TileType::Ground:
                drawTileSprite(window,
                    isTopGroundRow(tileMap, c, r) ? tex.dirt : tex.dirt2, c, r);
                break;
            case TileType::Brick:
                drawTileSprite(window, tex.brick, c, r);   break;
            case TileType::QuestionBlock:
                drawTileSprite(window, tex.question, c, r); break;
            case TileType::MetalBlock:
                drawTileSprite(window, tex.dirt2, c, r);   break;
            case TileType::Platform:
                drawTileSprite(window, tex.pipe, c, r);
                break;
            default: break;
            }
        }
    }
    drawPitBottomRow(window, tileMap, tex);
}

// ================================================================== //
//  HUD                                                                 //
// ================================================================== //
std::string formatScore(int score, int digits = 6) {
    std::ostringstream oss;
    oss << std::setw(digits) << std::setfill('0') << std::max(0, score);
    return oss.str();
}

std::string formatTime(int seconds) {
    seconds = std::max(0, seconds);
    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << (seconds / 60)
        << ":" << std::setw(2) << std::setfill('0') << (seconds % 60);
    return oss.str();
}

// [LEVEL2] Dodano parametr currentLevel do wyswietlania numeru poziomu w HUD
void drawHUD(sf::RenderWindow& window, const sf::Font& font,
             int lives, int score, float timeLeft,
             const sf::Texture& texHeart, int currentLevel)
{
    const float W = static_cast<float>(window.getSize().x);

    sf::RectangleShape bar({ W, 40.f });
    bar.setFillColor(sf::Color(0, 0, 0, 200));
    window.draw(bar);

    {
        sf::Vector2u sz = texHeart.getSize();
        for (int i = 0; i < lives; ++i) {
            sf::Sprite heart(texHeart);
            heart.setScale({ 28.f / sz.x, 28.f / sz.y });
            heart.setPosition({ 20.f + i * 40.f, 6.f });
            window.draw(heart);
        }
    }

    // [LEVEL2] Etykieta aktualnego poziomu po lewej stronie pod serduszkami
    sf::Text levelLabel(font, "POZIOM " + std::to_string(currentLevel), 13);
    levelLabel.setFillColor(sf::Color(255, 220, 80));
    levelLabel.setStyle(sf::Text::Bold);
    levelLabel.setPosition({ 20.f, 26.f });
    window.draw(levelLabel);

    sf::Text scoreLabel(font, "SCORE", 13);
    scoreLabel.setFillColor(sf::Color(180, 180, 180));
    scoreLabel.setPosition({ W / 2.f - 80.f, 4.f });
    window.draw(scoreLabel);

    sf::Text scoreVal(font, formatScore(score), 18);
    scoreVal.setFillColor(sf::Color::White);
    scoreVal.setStyle(sf::Text::Bold);
    scoreVal.setPosition({ W / 2.f - 80.f, 18.f });
    window.draw(scoreVal);

    sf::Text timeLabel(font, "TIME", 13);
    timeLabel.setFillColor(sf::Color(180, 180, 180));
    timeLabel.setPosition({ W - 100.f, 4.f });
    window.draw(timeLabel);

    sf::Color timeColor = (timeLeft < 30.f) ? sf::Color(255, 80, 80) : sf::Color::White;
    sf::Text timeVal(font, formatTime(static_cast<int>(timeLeft)), 18);
    timeVal.setFillColor(timeColor);
    timeVal.setStyle(sf::Text::Bold);
    timeVal.setPosition({ W - 100.f, 18.f });
    window.draw(timeVal);
}

// ================================================================== //
//  Overlay (GameOver / Win)                                            //
//  Napisy sa automatycznie skalowane tak, by nigdy nie wychodzily      //
//  poza szerokosc okna (margines 20px z kazdej strony).               //
// ================================================================== //
void drawOverlay(sf::RenderWindow& window, const sf::Font& font,
                 const std::string& title, const std::string& subtitle,
                 sf::Color titleColor)
{
    const float W  = static_cast<float>(window.getSize().x);
    const float H  = static_cast<float>(window.getSize().y);
    const float margin = 20.f;
    const float maxW   = W - 2.f * margin;

    sf::RectangleShape bg({ W, H });
    bg.setFillColor(sf::Color(0, 0, 0, 160));
    window.draw(bg);

    // --- Tytul (GAME OVER / GRA UKONCZONA itp.) ---
    sf::Text t(font, title, 48);
    t.setFillColor(titleColor);
    t.setStyle(sf::Text::Bold);
    {
        sf::FloatRect tb = t.getLocalBounds();
        // Zmniejsz czcionke jesli napis jest za szeroki
        if (tb.size.x > maxW) {
            const unsigned int reducedSize =
                static_cast<unsigned int>(48u * (maxW / tb.size.x));
            t.setCharacterSize(std::max(12u, reducedSize));
            tb = t.getLocalBounds();
        }
        t.setOrigin({ tb.position.x + tb.size.x / 2.f,
                      tb.position.y + tb.size.y / 2.f });
    }
    t.setPosition({ W / 2.f, H / 2.f - 60.f });
    window.draw(t);

    // --- Podtytul (instrukcja R / M itp.) ---
    sf::Text s(font, subtitle, 18);
    s.setFillColor(sf::Color::White);
    {
        sf::FloatRect sb = s.getLocalBounds();
        if (sb.size.x > maxW) {
            const unsigned int reducedSize =
                static_cast<unsigned int>(18u * (maxW / sb.size.x));
            s.setCharacterSize(std::max(10u, reducedSize));
            sb = s.getLocalBounds();
        }
        s.setOrigin({ sb.position.x + sb.size.x / 2.f,
                      sb.position.y + sb.size.y / 2.f });
    }
    s.setPosition({ W / 2.f, H / 2.f + 20.f });
    window.draw(s);
}

void drawBonusCountdown(sf::RenderWindow& window, const sf::Font& font,
                        float bonusRemaining, int bonusEarned)
{
    sf::RectangleShape bg({ static_cast<float>(window.getSize().x),
                            static_cast<float>(window.getSize().y) });
    bg.setFillColor(sf::Color(0, 0, 0, 160));
    window.draw(bg);

    sf::Text t(font, "LEVEL CLEAR!", 48);
    t.setFillColor(sf::Color::Yellow);
    t.setStyle(sf::Text::Bold);
    sf::FloatRect tb = t.getLocalBounds();
    t.setOrigin({ tb.position.x + tb.size.x / 2.f,
                  tb.position.y + tb.size.y / 2.f });
    t.setPosition({ window.getSize().x / 2.f,
                    window.getSize().y / 2.f - 80.f });
    window.draw(t);

    sf::Text bonus(font,
        "TIME BONUS  " + formatTime(static_cast<int>(bonusRemaining))
        + "  x100", 26);
    bonus.setFillColor(sf::Color::White);
    sf::FloatRect bb = bonus.getLocalBounds();
    bonus.setOrigin({ bb.position.x + bb.size.x / 2.f,
                      bb.position.y + bb.size.y / 2.f });
    bonus.setPosition({ window.getSize().x / 2.f,
                        window.getSize().y / 2.f });
    window.draw(bonus);

    sf::Text earned(font, "+" + std::to_string(bonusEarned) + " pts", 22);
    earned.setFillColor(sf::Color(100, 255, 100));
    sf::FloatRect eb = earned.getLocalBounds();
    earned.setOrigin({ eb.position.x + eb.size.x / 2.f,
                       eb.position.y + eb.size.y / 2.f });
    earned.setPosition({ window.getSize().x / 2.f,
                         window.getSize().y / 2.f + 50.f });
    window.draw(earned);
}

// [LEVEL2] Ekran przejsciowy miedzy poziomami —
//          czarne tlo + duzy napis + pasek postepu odliczania.
void drawLevelTransition(sf::RenderWindow& window, const sf::Font& font,
                         int nextLevel, float timer, float duration)
{
    sf::RectangleShape bg({ static_cast<float>(window.getSize().x),
                            static_cast<float>(window.getSize().y) });
    bg.setFillColor(sf::Color::Black);
    window.draw(bg);

    sf::Text t(font, "POZIOM UKONCZONY!", 48);
    t.setFillColor(sf::Color::Yellow);
    t.setStyle(sf::Text::Bold);
    sf::FloatRect tb = t.getLocalBounds();
    t.setOrigin({ tb.position.x + tb.size.x / 2.f,
                  tb.position.y + tb.size.y / 2.f });
    t.setPosition({ window.getSize().x / 2.f,
                    window.getSize().y / 2.f - 70.f });
    window.draw(t);

    sf::Text sub(font, "LADOWANIE POZIOMU " + std::to_string(nextLevel) + "...", 28);
    sub.setFillColor(sf::Color::White);
    sf::FloatRect sb = sub.getLocalBounds();
    sub.setOrigin({ sb.position.x + sb.size.x / 2.f,
                    sb.position.y + sb.size.y / 2.f });
    sub.setPosition({ window.getSize().x / 2.f,
                      window.getSize().y / 2.f });
    window.draw(sub);

    // Pasek postepu odliczania (maleje od pelnego do pustego)
    const float barW = 400.f;
    const float barH = 10.f;
    const float barX = (window.getSize().x - barW) / 2.f;
    const float barY = window.getSize().y / 2.f + 50.f;
    const float fill = barW * (1.f - timer / duration);  // rosnie w prawo

    sf::RectangleShape barBg({ barW, barH });
    barBg.setPosition({ barX, barY });
    barBg.setFillColor(sf::Color(60, 60, 60));
    window.draw(barBg);

    sf::RectangleShape barFg({ fill, barH });
    barFg.setPosition({ barX, barY });
    barFg.setFillColor(sf::Color(80, 200, 80));
    window.draw(barFg);
}

} // namespace

// ================================================================== //
//  main                                                                //
// ================================================================== //
int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    sf::RenderWindow window(sf::VideoMode({ 800, 480 }), "Mario Contra 2");
    window.setFramerateLimit(60);

    sf::Font font;
    bool fontLoaded = false;
    if (!font.openFromFile("assets/font.ttf"))
        std::cerr << "[main] Brak czcionki: assets/font.ttf\n";
    else
        fontLoaded = true;

    AppState appState = AppState::MainMenu;

    Menu menu;
    if (!menu.loadFont("assets/font.ttf"))
        std::cerr << "[main] Menu: brak czcionki assets/font.ttf\n";

    std::string playerName;

    static const std::string LEADERBOARD_FILE = "leaderboard.txt";
    Leaderboard leaderboard;
    if (!leaderboard.loadFont("assets/font.ttf"))
        std::cerr << "[main] Leaderboard: brak czcionki assets/font.ttf\n";
    leaderboard.loadFromFile(LEADERBOARD_FILE);

    sf::Texture texHeart;
    if (!texHeart.loadFromFile("assets/heart.png"))
        std::cerr << "[main] brak: assets/heart.png\n";

    // ----------------------------------------------------------------
    // [LEVEL2] Konfiguracja obu poziomow w jednym miejscu.
    //          Dodajac trzeci poziom wystarczy dopisac tu jeden wpis.
    // ----------------------------------------------------------------
    struct LevelConfig {
        std::string mapFile;          // sciezka do pliku mapy
        std::vector<sf::Vector2f> enemySpawns;     // pozycje startowe wrogow
        std::vector<sf::Vector2f> checkpointPos;   // pozycje checkpointow
        sf::Vector2f              playerStart;     // pozycja startowa gracza
    };

    const std::array<LevelConfig, 2> levelConfigs = {{
        // --- Poziom 1 ---
        // spawnY = (row_ziemi * TILE_SIZE) - HITBOX_H = 13*32 - 100 = 316
        // Nogi gracza laduja na y=416 = gorna krawedz kafelka row=13 (pierwsza warstwa ziemi)
        {
            "level1.txt",
            {
                {600.f,        380.f},
                {38 * 32.f,   352.f},
                {62 * 32.f,   352.f},
                {80 * 32.f,   192.f},
                {105 * 32.f,  352.f},
                {130 * 32.f,  352.f},
                {158 * 32.f,  352.f},
                {185 * 32.f,  352.f},
                {215 * 32.f,  256.f},
                {235 * 32.f,  352.f},
                {255 * 32.f,  192.f},
                {54 * 32.f,   352.f},
                {71 * 32.f,   352.f},
                {170 * 32.f,  352.f},
                {95 * 32.f,   352.f},
                {200 * 32.f,  352.f},
                {80 * 32.f,   352.f}
            },
            { {2496.f, 316.f}, {5280.f, 316.f} },
            { 96.f, 316.f }
        },
        // --- Poziom 2 ---
        {
            "level2.txt",
            {
        {600.f,        316.f},   // kol. ~18, start na pierwszym odcinku ziemi
        {34 * 32.f,    316.f},   // kol. 34-39, odcinek po pierwszej dziurze
        {44 * 32.f,    316.f},   // kol. 44-47
        {60 * 32.f,    316.f},   // kol. 58-65
        {88 * 32.f,    316.f},   // kol. 87-91
        {96 * 32.f,    316.f},   // kol. 94-99 (blisko checkpointu x=3000)
        {120 * 32.f,   316.f},   // kol. 120-121
        {165 * 32.f,   316.f},   // kol. 163-167
        {184 * 32.f,   316.f},   // kol. 182-185
        {212 * 32.f,   316.f},   // kol. 210-213
        {217 * 32.f,   316.f}    // kol. 216-219
    },
            { {3000.f, 316.f} },
            { 96.f, 316.f }
        }
    }};

    // [LEVEL2] Aktualny poziom (0-indeksowany wewnetrznie, wyswietlamy jako 1-based)
    int currentLevel = 0;  // 0 = poziom 1, 1 = poziom 2

    // ----------------------------------------------------------------
    // [LEVEL2] Lamda inicjalizujaca swiat gry dla podanego poziomu.
    //          Uzywa LevelConfig — nie dubluje kodu przy restarcie.
    // ----------------------------------------------------------------
    TileMap tileMap;
    std::vector<Enemy>      enemies;
    std::vector<Checkpoint> checkpoints;
    std::unique_ptr<GoalFlag> goalFlag;   // [LEVEL2] unique_ptr bo tworzymy go po zaladowaniu mapy
    float levelWidth = 0.f;

    auto initLevel = [&](int levelIdx) {
        const LevelConfig& cfg = levelConfigs[levelIdx];

        if (!tileMap.loadFromFile(cfg.mapFile)) {
            std::cerr << "[main] Nie mozna zaladowac mapy: " << cfg.mapFile << "\n";
        }
        levelWidth = tileMap.getSizeInPixels().x;

        enemies.clear();
        enemies.reserve(static_cast<std::size_t>(cfg.enemySpawns.size()) + 4);
        for (const sf::Vector2f& pos : cfg.enemySpawns)
            enemies.emplace_back(pos);

        checkpoints.clear();
        checkpoints.reserve(static_cast<std::size_t>(cfg.checkpointPos.size()) + 4);
        for (const sf::Vector2f& pos : cfg.checkpointPos)
            checkpoints.emplace_back(pos);

        goalFlag = std::make_unique<GoalFlag>(sf::Vector2f(levelWidth - 96.f, 352.f));
    };

    // Pierwsze ladowanie — poziom 1
    initLevel(currentLevel);

    TileTextures tileTextures;
    if (!tileTextures.load())
        std::cerr << "[main] Brak niektorych tekstur kafelkow.\n";

    // --- Audio ---
    sf::SoundBuffer loseBuffer;
    sf::SoundBuffer endLevelBuffer;
    std::optional<sf::Sound> loseSound;
    std::optional<sf::Sound> endLevelSound;
    bool loseSoundLoaded = false;
    bool endLevelSoundLoaded = false;

    auto loadSoundBuffer = [&](sf::SoundBuffer& buffer, const char* path, bool& loadedFlag) {
        if (!buffer.loadFromFile(path)) {
            std::cerr << "[main] brak: " << path << "\n";
            loadedFlag = false;
        } else {
            loadedFlag = true;
        }
    };

    loadSoundBuffer(loseBuffer,     "assets/sounds/lose_sound.wav",      loseSoundLoaded);
    loadSoundBuffer(endLevelBuffer, "assets/sounds/end_level_sound.wav", endLevelSoundLoaded);

    loseSound.emplace(loseBuffer);
    endLevelSound.emplace(endLevelBuffer);
    loseSound->setVolume(90.f);
    endLevelSound->setVolume(90.f);

    // ----------------------------------------------------------------
    // MUZYKA MENU — osobny utwor odtwarzany wylacznie w menu/name input
    // ----------------------------------------------------------------
    sf::Music menuMusic;
    bool menuMusicLoaded = false;
    if (!menuMusic.openFromFile("assets/music/menu.wav")) {
        std::cerr << "[main] brak: assets/music/menu.wav (opcjonalne)\n";
    } else {
        menuMusic.setLooping(true);
        menuMusic.setVolume(60.f);
        menuMusic.play();   // startujemy od razu — aplikacja otwiera sie w menu
        menuMusicLoaded = true;
    }

    // ----------------------------------------------------------------
    // [LEVEL2] Muzyka rozgrywki — osobne utwory dla kazdego poziomu
    // ----------------------------------------------------------------
    sf::Music musicLevel1;
    sf::Music musicLevel2;
    bool musicLevel1Loaded = false;
    bool musicLevel2Loaded = false;

    if (!musicLevel1.openFromFile("assets/music/music_level_1.wav")) {
        std::cerr << "[main] brak: assets/music/music_level_1.wav\n";
    } else {
        musicLevel1.setLooping(true);
        musicLevel1.setVolume(55.f);
        // NIE startujemy — zacznie grac dopiero po wejsciu do rozgrywki
        musicLevel1Loaded = true;
    }

    if (!musicLevel2.openFromFile("assets/music/music_level_2.wav")) {
        std::cerr << "[main] brak: assets/music/music_level_2.wav (opcjonalne)\n";
    } else {
        musicLevel2.setLooping(true);
        musicLevel2.setVolume(55.f);
        musicLevel2Loaded = true;
        // NIE startujemy — zacznie grac dopiero po przejsciu do poziomu 2
    }

    // Pomocnicza lamda: zatrzymuje CALA muzyke rozgrywki (obie sciezki)
    auto stopGameplayMusic = [&]() {
        musicLevel1.stop();
        musicLevel2.stop();
    };

    // Pomocnicza lamda: pauza/wznawianie muzyki rozgrywki (Escape)
    auto pauseGameplayMusic = [&]() {
        if (musicLevel1.getStatus() == sf::Music::Status::Playing) musicLevel1.pause();
        if (musicLevel2.getStatus() == sf::Music::Status::Playing) musicLevel2.pause();
    };
    auto resumeGameplayMusic = [&]() {
        // Wznawiamy tylko te, ktora byla grana (nie startujemy obu naraz)
        if (musicLevel1.getStatus() == sf::Music::Status::Paused) musicLevel1.play();
        if (musicLevel2.getStatus() == sf::Music::Status::Paused) musicLevel2.play();
    };

    // [LEVEL2] Pomocnicza lamda przelaczajaca muzyk miedzy poziomami rozgrywki
    auto switchMusic = [&](int levelIdx) {
        musicLevel1.stop();
        musicLevel2.stop();
        if (levelIdx == 0 && musicLevel1Loaded) musicLevel1.play();
        if (levelIdx == 1 && musicLevel2Loaded) musicLevel2.play();
        // Dla ewentualnych dalszych poziomow muzyka poziomu 2 gra dalej
        if (levelIdx > 1  && musicLevel2Loaded) musicLevel2.play();
    };

    // --- Obiekty gry ---
    Player player;

    std::vector<Item> items;
    player.setItemList(&items);
    items.reserve(32);

    // [LEVEL2] Ustaw pozycje startowa gracza wg konfiguracji poziomu
    player.setSpawnPoint(levelConfigs[currentLevel].playerStart);

    std::vector<Bullet> bullets;
    bullets.reserve(32);
    float shootCooldown = 0.f;
    static constexpr float SHOOT_DELAY = 0.25f;
    bool windowHasFocus = true;

    std::vector<BrickDebris> brickDebris;
    brickDebris.reserve(32);

    int killCount = 0;

    GameState gameState = GameState::Playing;

    static constexpr float LEVEL_TIME = 300.f;
    float timeLeft = LEVEL_TIME;

    float bonusRemaining = 0.f;
    int   bonusEarned    = 0;
    static constexpr float BONUS_DRAIN_RATE = 60.f;

    bool scoreSaved = false;

    // [LEVEL2] Stan i timer ekranu przejsciowego miedzy poziomami
    bool  inLevelTransition      = false;
    float levelTransitionTimer   = 0.f;
    static constexpr float LEVEL_TRANSITION_DURATION = 2.5f;  // sekundy

    float shakeTimer = 0.f;
    static constexpr float SHAKE_DURATION  = 0.4f;
    static constexpr float SHAKE_MAGNITUDE = 6.f;

    // [FIX3] Timer auto-powrotu do menu po wyswietleniu ekranu koncowego
    float winScreenTimer = 0.f;
    static constexpr float WIN_SCREEN_DURATION = 6.f;

    // --- Kamera ---
    const sf::Vector2f viewSize(static_cast<float>(window.getSize().x),
                                static_cast<float>(window.getSize().y));

    // [LEVEL2] maxCameraCenterX musi byc przeliczane po zmianie mapy —
    //          uzywamy lambdy, zeby nie duplikowac kodu
    auto computeMaxCamX = [&]() {
        return std::max(viewSize.x / 2.f, levelWidth - viewSize.x / 2.f);
    };
    float maxCameraCenterX = computeMaxCamX();

    // [FIX3] Pelny reset gry — wywolywany przy R i przy powrocie do menu po wygranej.
    // Centralizuje logike resetu zamiast duplikowac ja w kilku miejscach.
    // UWAGA: winScreenTimer musi byc zdefiniowany wczesniej (jest powyzej).
    auto fullGameReset = [&]() {
        currentLevel = 0;

        player = Player();
        player.setName(playerName);
        player.setItemList(&items);
        player.setSpawnPoint(levelConfigs[0].playerStart);
        items.clear();
        bullets.clear();
        shootCooldown = 0.f;

        initLevel(0);
        maxCameraCenterX = computeMaxCamX();

        killCount         = 0;
        timeLeft          = LEVEL_TIME;
        bonusRemaining    = 0.f;
        bonusEarned       = 0;
        gameState         = GameState::Playing;
        scoreSaved        = false;
        inLevelTransition = false;
        winScreenTimer    = 0.f;

        stopGameplayMusic();
        if (menuMusicLoaded) menuMusic.play();   // wracamy do menu — wlacz muzyke menu
    };

    sf::View gameView(sf::FloatRect({ 0.f, 0.f }, viewSize));
    gameView.setCenter(viewSize / 2.f);
    const sf::View uiView = window.getDefaultView();

    constexpr float CAMERA_LERP = 5.f;

    sf::Clock clock;

    // ================================================================
    //  GAME LOOP
    // ================================================================
    while (window.isOpen() && !menu.wantsToExit()) {

        // --- Eventy ---
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
            if (event->is<sf::Event::FocusLost>()) {
                player.clearMovementInput();
                windowHasFocus = false;
            }
            if (event->is<sf::Event::FocusGained>()) {
                windowHasFocus = true;
            }

            // [LEVEL2] Podczas ekranu przejsciowego blokujemy wszystkie inputy gry
            if (inLevelTransition) continue;

            switch (appState) {

            case AppState::MainMenu:
            case AppState::NameInput:
                menu.handleEvent(*event, appState, playerName);
                if (appState == AppState::Gameplay) {
                    player.setName(playerName);
                    // Wejscie do rozgrywki — zatrzymaj muzyke menu i wystartuj gameplay
                    menuMusic.stop();
                    switchMusic(currentLevel);
                }
                break;

            case AppState::PauseMenu:
                if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                    if (key->code == sf::Keyboard::Key::Escape) {
                        appState = AppState::Gameplay;
                        resumeGameplayMusic();   // wznow muzyke od miejsca pauzy
                    }
                    else if (key->code == sf::Keyboard::Key::Q) {
                        stopGameplayMusic();
                        if (menuMusicLoaded) menuMusic.play();   // powrot do menu
                        menu.reset();
                        appState = AppState::MainMenu;
                    }
                }
                break;

            case AppState::Leaderboard:
                leaderboard.handleEvent(*event, appState);
                break;

            case AppState::Gameplay:
                if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                    if (key->code == sf::Keyboard::Key::Escape) {
                        appState = AppState::PauseMenu;
                        pauseGameplayMusic();    // wycisz muzyke podczas pauzy
                        break;
                    }

                    // R — restart od Level 1 (dziala w GameOver i Win)
                    if (key->code == sf::Keyboard::Key::R &&
                        gameState != GameState::Playing) {
                        fullGameReset();           // [FIX3] centralizowany reset
                        switchMusic(0);
                    }

                    // [FIX3] M — powrot do menu glownego po wygranej lub po game over
                    // Resetuje gre i przelacza appState na MainMenu.
                    if (key->code == sf::Keyboard::Key::M &&
                        gameState != GameState::Playing) {
                        fullGameReset();
                        menu.reset();
                        appState = AppState::MainMenu;
                    }

                    player.handleKeyPressed(key->code);
                }

                if (const auto* key = event->getIf<sf::Event::KeyReleased>()) {
                    player.handleKeyReleased(key->code);
                }
                break;
            }
        }

        const float dt = clock.restart().asSeconds();

        if (appState == AppState::MainMenu || appState == AppState::NameInput) {
            menu.update(dt);
        }

        // ============================================================
        //  [LEVEL2] UPDATE — ekran przejsciowy miedzy poziomami
        // ============================================================
        if (appState == AppState::Gameplay && inLevelTransition) {
            levelTransitionTimer -= dt;

            if (levelTransitionTimer <= 0.f) {
                // Czas minal — ladujemy poziom 2 i wznawiamy gre
                inLevelTransition = false;

                initLevel(currentLevel);           // laduje mape, wrogow, checkpointy
                maxCameraCenterX = computeMaxCamX();

                // Pozycja startowa gracza na nowym poziomie
                const sf::Vector2f startPos = levelConfigs[currentLevel].playerStart;
                player.setSpawnPoint(startPos);
                player.respawn();                  // teleportuje do spawn pointa
                player.clearMovementInput();

                // Reset kamery natychmiastowy (bez lerp) — zeby nie "przeskoczyc" przez mape
                gameView.setCenter({ viewSize.x / 2.f, viewSize.y / 2.f });

                items.clear();
                bullets.clear();
                shootCooldown = 0.f;

                timeLeft       = LEVEL_TIME;       // nowy czas dla poziomu 2
                bonusRemaining = 0.f;
                bonusEarned    = 0;
                gameState      = GameState::Playing;

                switchMusic(currentLevel);         // [LEVEL2] muzyka poziomu 2
            }
        }

        // ============================================================
        //  UPDATE — glowna logika gry (blokowana podczas przejscia)
        // ============================================================
        if (appState == AppState::Gameplay && gameState == GameState::Playing
            && !inLevelTransition)
        {
            // --- Odliczanie czasu ---
            timeLeft -= dt;
            if (timeLeft <= 0.f) {
                timeLeft = 0.f;
                player.loseLife();
            }

            // --- Gracz ---
            player.update(dt);
            player.resolveCollisions(tileMap);


            if (auto brickPos = player.getLastBrickHit()) {
                brickDebris.emplace_back(*brickPos);
                player.clearLastBrickHit();
            }

            shootCooldown -= dt;
            if (windowHasFocus && shootCooldown <= 0.f &&
                (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Z) ||
                    sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)))
            {
                sf::FloatRect pb = player.getBounds();
                float dir = player.getFacingDirection();
                if (player.isOnGround() && !player.isCrouching() &&
                    (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) ||
                        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)))
                {
                    dir = -1.f;
                }
                else {
                    dir = 1.f;
                }
                float bulletY;
                if (player.isCrouching())
                    bulletY = pb.position.y + pb.size.y - 13.f;
                else
                    bulletY = pb.position.y + pb.size.y - 55.f;

                sf::Vector2f spawnPos = {
                    dir > 0.f ? pb.position.x + pb.size.x : pb.position.x,
                    bulletY
                };

                if (player.isTripleShotActive()) {
                    bullets.emplace_back(spawnPos, dir,  0.f);
                    bullets.emplace_back(spawnPos, dir, -15.f);
                    bullets.emplace_back(spawnPos, dir,  15.f);
                }
                else {
                    bullets.emplace_back(spawnPos, dir);
                }
                shootCooldown = SHOOT_DELAY;
            }

            // --- Update pociskow + kolizje z kafelkami ---
            for (Bullet& bullet : bullets) {
                if (!bullet.isActive()) continue;

                bullet.update(dt, tileMap);

                sf::FloatRect bb = bullet.getBounds();
                sf::Vector2f center = {
                    bb.position.x + bb.size.x / 2.f,
                    bb.position.y + bb.size.y / 2.f
                };

                const float TS = TileMap::TILE_SIZE;

                std::array<sf::Vector2f, 3> checkPoints = { {
                    { bb.position.x,                    center.y },
                    { bb.position.x + bb.size.x - 1.f, center.y },
                    { center.x,                         center.y }
                } };

                for (sf::Vector2f point : checkPoints) {
                    int col = static_cast<int>(point.x / TS);
                    int row = static_cast<int>(point.y / TS);
                    TileType t = tileMap.getTile(col, row);

                    if (t == TileType::Brick) {
                        sf::Vector2f brickPos = { col * TileMap::TILE_SIZE,
                                                  row * TileMap::TILE_SIZE };
                        tileMap.hitBrick(col, row, items);
                        brickDebris.emplace_back(brickPos);   // ← nowa linia
                        bullet.deactivate();
                        break;
                    }
                    else if (t == TileType::QuestionBlock) {
                        tileMap.hitQuestionBlock(col, row, items);
                        bullet.deactivate();
                        break;
                    }
                }
            }

            for (Bullet& bullet : bullets) {
                if (!bullet.isActive()) continue;
                for (Enemy& enemy : enemies) {
                    if (!enemy.isAlive()) continue;
                    if (bullet.getBounds().findIntersection(enemy.getBounds())) {
                        bullet.deactivate();
                        if (enemy.hit()) {
                            player.addScore(100);
                            ++killCount;
                            break;
                        }
                    }
                }
            }

            bullets.erase(
                std::remove_if(bullets.begin(), bullets.end(),
                    [](const Bullet& b) { return !b.isActive(); }),
                bullets.end());

            for (BrickDebris& d : brickDebris)
                d.update(dt);
            brickDebris.erase(
                std::remove_if(brickDebris.begin(), brickDebris.end(),
                    [](const BrickDebris& d) { return !d.isActive(); }),
                brickDebris.end());

            for (Checkpoint& cp : checkpoints) {
                if (cp.checkCollision(player))
                    player.setSpawnPoint(cp.getPosition());
            }

            for (Item& it : items)
                it.update(dt);

            for (Item& it : items) {
                if (it.isCollected()) continue;

                sf::FloatRect ib = it.getBounds();
                float footX = ib.position.x + ib.size.x / 2.f;
                float footY = ib.position.y + ib.size.y + 1.f;

                int col = static_cast<int>(footX / TileMap::TILE_SIZE);
                int row = static_cast<int>(footY / TileMap::TILE_SIZE);

                if (col == it.m_sourceCol && row == it.m_sourceRow)
                    continue;

                if (tileMap.isSolid(col, row)) {
                    float groundY = row * TileMap::TILE_SIZE;
                    it.snapToGround(groundY);
                }
            }

            if (player.justDied()) {
                shakeTimer = SHAKE_DURATION;
                player.clearJustDied();
            }

            for (Enemy& enemy : enemies)
                enemy.update(dt, tileMap);

            for (Item& it : items) {
                if (!it.isCollected() && it.getBounds().findIntersection(player.getBounds())) {
                    it.collect();

                    if (it.getType() == ItemType::Coin)
                        player.addScore(100);
                    else if (it.getType() == ItemType::Heart) {
                        player.addLife();
                        player.addScore(300);
                    }
                    else if (it.getType() == ItemType::Mushroom) {
                        player.activateTripleShot();
                        player.addScore(200);
                    }
                }
            }

            for (Enemy& enemy : enemies) {
                if (!enemy.isAlive()) continue;

                if (enemy.checkCollisionWithPlayer(player) && player.getVelocity().y > 0.f) {
                    enemy.kill();
                    player.addScore(100);
                    ++killCount;
                    player.bounce();
                }
                else if (enemy.getBounds().findIntersection(player.getBounds())) {
                    if (player.isInvincible()) continue;
                    player.loseLife();
                    if (player.justDied()) {
                        shakeTimer = SHAKE_DURATION;
                        player.clearJustDied();
                    }
                }
            }

            // --- Kamera (lerp) ---
            const sf::FloatRect pb = player.getBounds();
            float targetX = pb.position.x + pb.size.x / 2.f;
            targetX = std::clamp(targetX, viewSize.x / 2.f, maxCameraCenterX);

            sf::Vector2f center = gameView.getCenter();
            const float lerp = std::min(1.f, CAMERA_LERP * dt);
            center.x += (targetX - center.x) * lerp;
            center.y  = viewSize.y / 2.f;

            shakeTimer = std::max(0.f, shakeTimer - dt);
            if (shakeTimer > 0.f) {
                const float t = shakeTimer / SHAKE_DURATION;
                const float mag = SHAKE_MAGNITUDE * t;
                center.x += (std::rand() % 2 == 0 ? 1 : -1) * mag;
                center.y += (std::rand() % 2 == 0 ? 1 : -1) * mag;
            }

            gameView.setCenter(center);

            // --- Warunki konca gry ---
            if (player.getLives() <= 0 && !player.isDying()) {
                gameState = GameState::GameOver;
                stopGameplayMusic();
                if (loseSoundLoaded && loseSound) loseSound->play();

                if (!scoreSaved) {
                    // [LEVEL2] Przekazujemy aktualny poziom jako liczbe ukoncz. poziomow
                    leaderboard.addScore(playerName, currentLevel, player.getScore());
                    leaderboard.saveToFile(LEADERBOARD_FILE);
                    scoreSaved = true;
                }
            }

            // --------------------------------------------------------
            // [LEVEL2] Kolizja z flaga — zachowanie zalezne od poziomu
            // --------------------------------------------------------
            if (goalFlag && goalFlag->checkCollisionWithPlayer(player)) {
                if (currentLevel < static_cast<int>(levelConfigs.size()) - 1) {
                    // --- Jest jeszcze nastepny poziom — uruchom przejscie ---
                    gameState = GameState::Playing;  // nie Win, bo punkty nie sa jeszcze zapisane
                    stopGameplayMusic();
                    if (endLevelSoundLoaded && endLevelSound) endLevelSound->play();

                    // Bonus za czas dla biezacego poziomu — przyznaj natychmiast
                    // (bez animacji countdown, bo ekran przejsciowy i tak zajmie czas)
                    const int timeBonus = static_cast<int>(timeLeft) * 100;
                    player.addScore(timeBonus);

                    currentLevel++;                            // [LEVEL2] nastepny poziom
                    player.clearMovementInput();
                    inLevelTransition   = true;
                    levelTransitionTimer = LEVEL_TRANSITION_DURATION;
                }
                else {
                    // --- To byl ostatni poziom — normalne zakonczenie gry ---
                    gameState      = GameState::Win;
                    bonusRemaining = timeLeft;
                    bonusEarned    = 0;
                    timeLeft       = 0.f;
                    stopGameplayMusic();
                    if (endLevelSoundLoaded && endLevelSound) endLevelSound->play();
                }
            }

        } // end Playing

        // [FIX3] Odliczanie bonusu po wygranej (ostatni poziom)
        if (appState == AppState::Gameplay && gameState == GameState::Win && bonusRemaining > 0.f) {
            const float drain = std::min(bonusRemaining, BONUS_DRAIN_RATE * dt);
            bonusRemaining -= drain;
            const int pts    = static_cast<int>(drain * 100.f);
            bonusEarned    += pts;
            player.addScore(pts);

            if (bonusRemaining < 0.5f) bonusRemaining = 0.f;
        }

        // [LEVEL2] Zapis do leaderboard — dopiero po ostatnim poziomie i wyzerowaniu bonusu
        if (appState == AppState::Gameplay && gameState == GameState::Win &&
            bonusRemaining <= 0.f && !scoreSaved)
        {
            leaderboard.addScore(playerName, currentLevel + 1, player.getScore());
            leaderboard.saveToFile(LEADERBOARD_FILE);
            scoreSaved = true;
        }

        // [FIX3] Odliczanie timera ekranu wygranej — po jego uplywie auto-powrot do menu.
        // Timer startuje dopiero gdy bonus jest wyzerowany (scoreSaved == true),
        // zeby gracz zdazyl zobaczyc ostateczny wynik przed znikaniem ekranu.
        if (appState == AppState::Gameplay && gameState == GameState::Win &&
            bonusRemaining <= 0.f && scoreSaved)
        {
            winScreenTimer += dt;
            if (winScreenTimer >= WIN_SCREEN_DURATION) {
                // Czas minal — resetuj gre i wróc do menu glownego
                fullGameReset();
                menu.reset();
                appState = AppState::MainMenu;
            }
        }

        // --- Tytul okna ---
        if (appState == AppState::Gameplay || appState == AppState::PauseMenu) {
            window.setTitle("Mario Contra 2 | poziom: " + std::to_string(currentLevel + 1)
                            + " | zycia: " + std::to_string(player.getLives())
                            + " | pkt: " + std::to_string(player.getScore()));
        } else {
            window.setTitle("Mario Contra 2");
        }

        // ============================================================
        //  DRAW
        // ============================================================
        window.clear(sf::Color(135, 206, 235));

        if (appState == AppState::Gameplay || appState == AppState::PauseMenu) {

            // [LEVEL2] Ekran przejsciowy rysujemy zamiast swiata gry
            if (inLevelTransition) {
                window.setView(uiView);
                if (fontLoaded)
                    drawLevelTransition(window, font, currentLevel + 1,
                                        levelTransitionTimer, LEVEL_TRANSITION_DURATION);
                window.display();
                continue;  // pomijamy reszte rysowania w tej klatce
            }

            window.setView(gameView);

            drawTileMap(window, tileMap, tileTextures);

            for (Item& it : items)
                it.draw(window);

            for (Bullet& b : bullets)
                b.draw(window);

            for (BrickDebris& d : brickDebris)
                d.draw(window);

            if (goalFlag) goalFlag->draw(window);
            for (Enemy& enemy : enemies) enemy.draw(window);
            player.draw(window);

            for (const Checkpoint& cp : checkpoints)
                cp.draw(window);

            window.setView(uiView);

            if (fontLoaded)
                drawHUD(window, font, player.getLives(), player.getScore(), timeLeft,
                        texHeart, currentLevel + 1);  // [LEVEL2] przekazujemy numer poziomu

            if (appState == AppState::Gameplay) {
                if (gameState == GameState::GameOver && fontLoaded)
                    drawOverlay(window, font, "GAME OVER",
                        "R - zagraj ponownie   M - menu glowne", sf::Color::Red);

                else if (gameState == GameState::Win && bonusRemaining > 0.f && fontLoaded)
                    drawBonusCountdown(window, font, bonusRemaining, bonusEarned);

                else if (gameState == GameState::Win && bonusRemaining <= 0.f && fontLoaded)
                    drawOverlay(window, font, "GRA UKONCZONA!",
                        "Punkty: " + formatScore(player.getScore())
                        + "   R-restart  M-menu  (auto-powrot za "
                        + std::to_string(static_cast<int>(WIN_SCREEN_DURATION - winScreenTimer) + 1)
                        + "s)", sf::Color::Yellow);
            }

            if (appState == AppState::PauseMenu && fontLoaded) {
                drawOverlay(window, font, "PAUZA",
                    "Esc - wznow   /   Q - menu glowne", sf::Color::White);
            }

            window.setView(gameView);
        }
        else if (appState == AppState::MainMenu || appState == AppState::NameInput) {
            window.setView(uiView);
            menu.draw(window);
        }
        else if (appState == AppState::Leaderboard) {
            window.setView(uiView);
            leaderboard.draw(window);
        }

        window.display();
    }

    return 0;
}