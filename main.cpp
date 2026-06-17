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

bool isPipeTopTile(const TileMap& tm, int col, int row) {
    return tm.getTile(col, row - 1) != TileType::Platform;
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
                if (isPipeTopTile(tileMap, c, r))
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
// Formatuje liczbe do N cyfr z wiodacymi zerami: 150 -> "000150"
std::string formatScore(int score, int digits = 6) {
    std::ostringstream oss;
    oss << std::setw(digits) << std::setfill('0') << std::max(0, score);
    return oss.str();
}

// Formatuje czas MM:SS
std::string formatTime(int seconds) {
    seconds = std::max(0, seconds);
    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << (seconds / 60)
        << ":" << std::setw(2) << std::setfill('0') << (seconds % 60);
    return oss.str();
}

void drawHUD(sf::RenderWindow& window, const sf::Font& font,
             int lives, int score, float timeLeft,
             const sf::Texture& texHeart)
{
    const float W = static_cast<float>(window.getSize().x);

    // Pasek tla
    sf::RectangleShape bar({ W, 40.f });
    bar.setFillColor(sf::Color(0, 0, 0, 200));
    window.draw(bar);

    // Ikony zyc — sprajt heart.png skalowany do 28x28px, odstep 40px
    {
        sf::Vector2u sz = texHeart.getSize();
        for (int i = 0; i < lives; ++i) {
            sf::Sprite heart(texHeart);
            heart.setScale({ 28.f / sz.x, 28.f / sz.y });
            heart.setPosition({ 20.f + i * 40.f, 6.f });
            window.draw(heart);
        }
    }

    // SCORE
    sf::Text scoreLabel(font, "SCORE", 13);
    scoreLabel.setFillColor(sf::Color(180, 180, 180));
    scoreLabel.setPosition({ W / 2.f - 80.f, 4.f });
    window.draw(scoreLabel);

    sf::Text scoreVal(font, formatScore(score), 18);
    scoreVal.setFillColor(sf::Color::White);
    scoreVal.setStyle(sf::Text::Bold);
    scoreVal.setPosition({ W / 2.f - 80.f, 18.f });
    window.draw(scoreVal);

    // TIME
    sf::Text timeLabel(font, "TIME", 13);
    timeLabel.setFillColor(sf::Color(180, 180, 180));
    timeLabel.setPosition({ W - 100.f, 4.f });
    window.draw(timeLabel);

    // Czerwony czas gdy < 30s
    sf::Color timeColor = (timeLeft < 30.f) ? sf::Color(255, 80, 80) : sf::Color::White;
    sf::Text timeVal(font, formatTime(static_cast<int>(timeLeft)), 18);
    timeVal.setFillColor(timeColor);
    timeVal.setStyle(sf::Text::Bold);
    timeVal.setPosition({ W - 100.f, 18.f });
    window.draw(timeVal);
}

// ================================================================== //
//  Overlay (GameOver / Win)                                            //
// ================================================================== //
void drawOverlay(sf::RenderWindow& window, const sf::Font& font,
                 const std::string& title, const std::string& subtitle,
                 sf::Color titleColor)
{
    sf::RectangleShape bg({ static_cast<float>(window.getSize().x),
                            static_cast<float>(window.getSize().y) });
    bg.setFillColor(sf::Color(0, 0, 0, 160));
    window.draw(bg);

    sf::Text t(font, title, 48);
    t.setFillColor(titleColor);
    t.setStyle(sf::Text::Bold);
    sf::FloatRect tb = t.getLocalBounds();
    t.setOrigin({ tb.position.x + tb.size.x / 2.f,
                  tb.position.y + tb.size.y / 2.f });
    t.setPosition({ window.getSize().x / 2.f,
                    window.getSize().y / 2.f - 60.f });
    window.draw(t);

    sf::Text s(font, subtitle, 18);
    s.setFillColor(sf::Color::White);
    sf::FloatRect sb = s.getLocalBounds();
    s.setOrigin({ sb.position.x + sb.size.x / 2.f,
                  sb.position.y + sb.size.y / 2.f });
    s.setPosition({ window.getSize().x / 2.f,
                    window.getSize().y / 2.f + 20.f });
    window.draw(s);
}

// Odliczanie bonusu czasowego po wygranej
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

} // namespace

// ================================================================== //
//  main                                                                //
// ================================================================== //
int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    sf::RenderWindow window(sf::VideoMode({ 800, 480 }), "Mario Contra 2");
    window.setFramerateLimit(60);

    // --- Czcionka ---
    sf::Font font;
    bool fontLoaded = false;
    if (!font.openFromFile("assets/font.ttf"))
        std::cerr << "[main] Brak czcionki: assets/font.ttf\n";
    else
        fontLoaded = true;

    // --- Stan calej aplikacji (menu / nick / gra / pauza / leaderboard) ---
    AppState appState = AppState::MainMenu;

    // --- Menu glowne + ekran wpisywania nicku ---
    Menu menu;
    if (!menu.loadFont("assets/font.ttf"))
        std::cerr << "[main] Menu: brak czcionki assets/font.ttf\n";

    std::string playerName; // wypelniane przez Menu po NameInput

    // --- Tablica wynikow ---
    static const std::string LEADERBOARD_FILE = "leaderboard.txt";
    Leaderboard leaderboard;
    if (!leaderboard.loadFont("assets/font.ttf"))
        std::cerr << "[main] Leaderboard: brak czcionki assets/font.ttf\n";
    leaderboard.loadFromFile(LEADERBOARD_FILE);

    // --- Tekstura serduszka do HUD ---
    sf::Texture texHeart;
    if (!texHeart.loadFromFile("assets/heart.png"))
        std::cerr << "[main] brak: assets/heart.png\n";

    // --- Mapa ---
    TileMap tileMap;
    if (!tileMap.loadFromFile("level1.txt")) return 1;

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

    loadSoundBuffer(loseBuffer, "assets/sounds/lose_sound.wav", loseSoundLoaded);
    loadSoundBuffer(endLevelBuffer, "assets/sounds/end_level_sound.wav", endLevelSoundLoaded);

    loseSound.emplace(loseBuffer);
    endLevelSound.emplace(endLevelBuffer);
    loseSound->setVolume(90.f);
    endLevelSound->setVolume(90.f);

    sf::Music musicLevel1;
    bool musicLoaded = false;
    if (!musicLevel1.openFromFile("assets/music/music_level_1.wav")) {
        std::cerr << "[main] brak: assets/music/music_level_1.wav\n";
    } else {
        musicLevel1.setLooping(true);
        musicLevel1.setVolume(55.f);
        musicLevel1.play();
        musicLoaded = true;
    }

    const float levelWidth = tileMap.getSizeInPixels().x;

    // --- Obiekty gry ---
    Player player;

    std::vector<Enemy> enemies;
    // reserve zapobiega realokacji przy emplace_back — bez tego vector
    // moze przeniesc obiekty w pamieci i unieważnic wskazniki sprite->texture
    enemies.reserve(8);
    enemies.emplace_back(sf::Vector2f(400.f, 380.f));
    enemies.emplace_back(sf::Vector2f(600.f, 380.f));

    std::vector<Item> items;
    player.setItemList(&items);
    items.reserve(32);

    std::vector<Bullet> bullets;
    bullets.reserve(32);
    float shootCooldown = 0.f;              
    static constexpr float SHOOT_DELAY = 0.25f;
    bool windowHasFocus = true;

    std::vector<Checkpoint> checkpoints;
    checkpoints.reserve(8);  // zapobiega realokacji i uniewazneniu wskaznikow sprite->texture
    checkpoints.emplace_back(sf::Vector2f(5280.f, 360.f));
    checkpoints.emplace_back(sf::Vector2f(2496.f, 360.f));


    GoalFlag goalFlag(sf::Vector2f(levelWidth - 96.f, 352.f));

    int killCount = 0;

    // --- Stan gry ---
    GameState gameState = GameState::Playing;

    // --- Licznik czasu poziomu ---
    static constexpr float LEVEL_TIME = 300.f;  // 5 minut
    float timeLeft = LEVEL_TIME;

    // --- Bonus za czas po wygranej ---
    float bonusRemaining = 0.f;   // sekund do odliczenia
    int   bonusEarned    = 0;     // punkty juz przyznane
    static constexpr float BONUS_DRAIN_RATE = 60.f; // sekund/sekunde (szybkie odliczanie)

    // Zapobiega wielokrotnemu dopisaniu tego samego wyniku do tablicy
    // wynikow w kolejnych klatkach po GameOver/Win. Resetowane przy R.
    bool scoreSaved = false;

    // --- Screen shake ---
    float shakeTimer     = 0.f;
    static constexpr float SHAKE_DURATION  = 0.4f;
    static constexpr float SHAKE_MAGNITUDE = 6.f;

    // --- Kamera ---
    const sf::Vector2f viewSize(static_cast<float>(window.getSize().x),
                                static_cast<float>(window.getSize().y));
    const float maxCameraCenterX = std::max(viewSize.x / 2.f,
                                            levelWidth - viewSize.x / 2.f);

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

            // Rozdzielenie obslugi zdarzen wedlug stanu calej aplikacji.
            switch (appState) {

            case AppState::MainMenu:
            case AppState::NameInput:
                // Menu samo wewnetrznie rozpoznaje, ktory z dwoch
                // ekranow (opcje / wpisywanie nicku) obecnie obslugiwac.
                menu.handleEvent(*event, appState, playerName);

                // Gdy NameInput zostalo wlasnie zatwierdzone, Menu
                // ustawilo appState = AppState::Gameplay i playerName.
                // Przekazujemy nick do gracza i (re)inicjalizujemy swiat gry.
                if (appState == AppState::Gameplay) {
                    player.setName(playerName);
                }
                break;

            case AppState::PauseMenu:
                if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                    if (key->code == sf::Keyboard::Key::Escape) {
                        appState = AppState::Gameplay; // wznowienie
                    }
                    else if (key->code == sf::Keyboard::Key::Q) {
                        // Powrot do glownego menu z pauzy
                        menu.reset();
                        appState = AppState::MainMenu;
                    }
                }
                break;

            case AppState::Leaderboard:
                // Klasa Leaderboard sama obsluguje wyjscie (Escape -> MainMenu).
                leaderboard.handleEvent(*event, appState);
                break;

            case AppState::Gameplay:
                if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                    if (key->code == sf::Keyboard::Key::Escape) {
                        appState = AppState::PauseMenu; // pauza, nie zamykanie okna
                        break; // nie przekazujemy Escape do gracza
                    }

                    // Restart po Game Over lub po zakonczeniu ekranu wygranej
                    if (key->code == sf::Keyboard::Key::R &&
                        gameState != GameState::Playing) {
                        player = Player();
                        player.setName(playerName);
                        player.setItemList(&items);
                        items.clear();

                        tileMap.loadFromFile("level1.txt");

                        bullets.clear();
                        shootCooldown = 0.f;

                        enemies.clear();
                        enemies.reserve(8);
                        enemies.emplace_back(sf::Vector2f(400.f, 380.f));
                        enemies.emplace_back(sf::Vector2f(600.f, 380.f));
                        killCount      = 0;
                        timeLeft       = LEVEL_TIME;
                        bonusRemaining = 0.f;
                        bonusEarned    = 0;
                        gameState      = GameState::Playing;
                        scoreSaved     = false;

                        if (musicLoaded) {
                            musicLevel1.stop();
                            musicLevel1.play();
                        }

                        for (Checkpoint& cp : checkpoints)
                            cp.reset();
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

        // Menu ma wlasna, lekka logike (np. miganie kursora nicku)
        // niezalezna od stanu rozgrywki.
        if (appState == AppState::MainMenu || appState == AppState::NameInput) {
            menu.update(dt);
        }

        // ============================================================
        //  UPDATE
        // ============================================================
        if (appState == AppState::Gameplay && gameState == GameState::Playing) {

            // --- Odliczanie czasu ---
            timeLeft -= dt;
            if (timeLeft <= 0.f) {
                timeLeft = 0.f;
                player.loseLife();   // czas minął = śmierć
            }

            // --- Gracz ---
            player.update(dt);
            player.resolveCollisions(tileMap);

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

                if (player.isCrouching()) {
                    // strzał przy kucaniu
                    bulletY = pb.position.y + pb.size.y - 13.f;
                }
                else {
                    // strzał przy staniu
                    bulletY = pb.position.y + pb.size.y - 55.f;
                }

                sf::Vector2f spawnPos = {
                    dir > 0.f ? pb.position.x + pb.size.x
                              : pb.position.x,
                    bulletY
                };

                if (player.isTripleShotActive()) {
                    bullets.emplace_back(spawnPos, dir, 0.f);
                    bullets.emplace_back(spawnPos, dir, -15.f); 
                    bullets.emplace_back(spawnPos, dir, 15.f); 
                }
                else {
                    bullets.emplace_back(spawnPos, dir);
                }
                shootCooldown = SHOOT_DELAY;
            }

            // --- Update pocisków + kolizje z wrogami ---
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
                        tileMap.hitBrick(col, row, items);
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

            for (Checkpoint& cp : checkpoints) {
                if (cp.checkCollision(player)) {
                    player.setSpawnPoint(cp.getPosition());
                }
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

            // Screen shake — odpalany przez justDied()
            if (player.justDied()) {
                shakeTimer = SHAKE_DURATION;
                player.clearJustDied();
            }

            // --- Wrogowie ---
            for (Enemy& enemy : enemies)
                enemy.update(dt, tileMap);

            // --- Kolizja gracz / itemy ---
            for (Item& it : items) {
                if (!it.isCollected() && it.getBounds().findIntersection(player.getBounds())) {
                    it.collect();

                    if (it.getType() == ItemType::Coin) {
                        player.addScore(100);
                    }
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

            // --- Kolizje gracz / wrogowie ---
            for (Enemy& enemy : enemies) {
                if (!enemy.isAlive()) continue;               

                if (enemy.checkCollisionWithPlayer(player) && player.getVelocity().y > 0.f) {
                    enemy.kill();
                    player.addScore(100);
                    ++killCount;
                    player.bounce();
                }

                else if (enemy.getBounds().findIntersection(player.getBounds())) {
                    if (player.isInvincible())
                        continue;

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

            // Screen shake — losowe przesuniecie kamery
            shakeTimer = std::max(0.f, shakeTimer - dt);
            if (shakeTimer > 0.f) {
                const float t = shakeTimer / SHAKE_DURATION;  // 1->0
                const float mag = SHAKE_MAGNITUDE * t;
                center.x += (std::rand() % 2 == 0 ? 1 : -1) * mag;
                center.y += (std::rand() % 2 == 0 ? 1 : -1) * mag;
            }

            gameView.setCenter(center);

            // --- Warunki konca gry ---
            if (player.getLives() <= 0 && !player.isDying()) {
                gameState = GameState::GameOver;
                if (musicLoaded)
                    musicLevel1.stop();
                if (loseSoundLoaded && loseSound)
                    loseSound->play();

                if (!scoreSaved) {
                    // Gracz przegral bez ukonczenia poziomu — 0 poziomow przejscia.
                    leaderboard.addScore(playerName, 0, player.getScore());
                    leaderboard.saveToFile(LEADERBOARD_FILE);
                    scoreSaved = true;
                }
            }

            if (goalFlag.checkCollisionWithPlayer(player)) {
                gameState      = GameState::Win;
                bonusRemaining = timeLeft;   // zamroz czas jako bonus
                bonusEarned    = 0;
                timeLeft       = 0.f;
                if (musicLoaded)
                    musicLevel1.stop();
                if (endLevelSoundLoaded && endLevelSound)
                    endLevelSound->play();
            }

        } // end Playing

        // --- Odliczanie bonusu po wygranej ---
        if (appState == AppState::Gameplay && gameState == GameState::Win && bonusRemaining > 0.f) {
            const float drain = std::min(bonusRemaining, BONUS_DRAIN_RATE * dt);
            bonusRemaining -= drain;
            const int pts    = static_cast<int>(drain * 100.f);
            bonusEarned    += pts;
            player.addScore(pts);

            if (bonusRemaining < 0.5f) bonusRemaining = 0.f;  // snap do 0
        }

        // Zapis wyniku do tablicy wynikow — dopiero gdy bonus czasowy
        // sie wyczerpal, aby zapisac ostateczna (pelna) punktacje.
        if (appState == AppState::Gameplay && gameState == GameState::Win &&
            bonusRemaining <= 0.f && !scoreSaved) {
            // Gracz dotarl do flagi — 1 przebyty poziom (gra ma na razie 1 poziom).
            leaderboard.addScore(playerName, 1, player.getScore());
            leaderboard.saveToFile(LEADERBOARD_FILE);
            scoreSaved = true;
        }

        // --- Tytul okna ---
        if (appState == AppState::Gameplay || appState == AppState::PauseMenu) {
            window.setTitle("Mario Contra 2 | zycia: " + std::to_string(player.getLives())
                            + " | pkt: " + std::to_string(player.getScore()));
        } else {
            window.setTitle("Mario Contra 2");
        }

        // ============================================================
        //  DRAW
        // ============================================================
        window.clear(sf::Color(135, 206, 235));

        if (appState == AppState::Gameplay || appState == AppState::PauseMenu) {
            // --- Swiat gry (z kamera) ---
            window.setView(gameView);

            drawTileMap(window, tileMap, tileTextures);

            for (Item& it : items)
                it.draw(window);

            for (Bullet& b : bullets)
                b.draw(window);

            goalFlag.draw(window);
            for (Enemy& enemy : enemies) enemy.draw(window);
            player.draw(window);

            for (const Checkpoint& cp : checkpoints)
                cp.draw(window);

            // --- UI (fixed, bez scrollowania) ---
            window.setView(uiView);

            if (fontLoaded)
                drawHUD(window, font, player.getLives(), player.getScore(), timeLeft, texHeart);

            // Overlay stanow rozgrywki (Game Over / Win) — tylko gdy faktycznie gramy
            if (appState == AppState::Gameplay) {
                if (gameState == GameState::GameOver && fontLoaded)
                    drawOverlay(window, font, "GAME OVER",
                        "Nacisnij R aby zagrac ponownie", sf::Color::Red);

                else if (gameState == GameState::Win && bonusRemaining > 0.f && fontLoaded)
                    drawBonusCountdown(window, font, bonusRemaining, bonusEarned);

                else if (gameState == GameState::Win && bonusRemaining <= 0.f && fontLoaded)
                    drawOverlay(window, font, "LEVEL OVER",
                        "Punkty: " + formatScore(player.getScore())
                        + "   Nacisnij R", sf::Color::Yellow);
            }

            // --- Nakladka menu pauzy (gra zamrozona pod nia) ---
            if (appState == AppState::PauseMenu && fontLoaded) {
                drawOverlay(window, font, "PAUZA",
                    "Esc - wznow   /   Q - menu glowne", sf::Color::White);
            }

            // Przywroc widok gry
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