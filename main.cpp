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
#include "Player.hpp"
#include "Enemy.hpp"
#include "TileMap.hpp"
#include "GoalFlag.hpp"
#include "GameState.hpp"
#include "Checkpoint.hpp"
#include "Item.hpp"

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
    while (window.isOpen()) {

        // --- Eventy ---
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
            if (event->is<sf::Event::FocusLost>()) {
                player.clearMovementInput();
            }
            if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                if (key->code == sf::Keyboard::Key::Escape)
                    window.close();

                // Restart po Game Over lub po zakonczeniu ekranu wygranej
                if (key->code == sf::Keyboard::Key::R &&
                    gameState != GameState::Playing) {
                    player = Player();
                    player.setItemList(&items);
                    enemies.clear();
                    enemies.reserve(8);
                    enemies.emplace_back(sf::Vector2f(400.f, 380.f));
                    enemies.emplace_back(sf::Vector2f(600.f, 380.f));
                    killCount      = 0;
                    timeLeft       = LEVEL_TIME;
                    bonusRemaining = 0.f;
                    bonusEarned    = 0;
                    gameState      = GameState::Playing;

                    if (musicLoaded)
                        musicLevel1.play();

                    for (Checkpoint& cp : checkpoints)
                        cp.reset();

                }

                player.handleKeyPressed(key->code);
            }

            if (const auto* key = event->getIf<sf::Event::KeyReleased>()) {
                player.handleKeyReleased(key->code);
            }
        }

        const float dt = clock.restart().asSeconds();

        // ============================================================
        //  UPDATE
        // ============================================================
        if (gameState == GameState::Playing) {

            // --- Odliczanie czasu ---
            timeLeft -= dt;
            if (timeLeft <= 0.f) {
                timeLeft = 0.f;
                player.loseLife();   // czas minął = śmierć
            }

            // --- Gracz ---
            player.update(dt);
            player.resolveCollisions(tileMap);

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

            // --- Kolizje gracz / wrogowie ---
            for (Enemy& enemy : enemies) {
                if (!enemy.isAlive()) continue;

                for (Item& it : items) {
                    if (!it.isCollected() && it.getBounds().findIntersection(player.getBounds())) {
                        it.collect();
                        player.addScore(100);
                    }
                }

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
        if (gameState == GameState::Win && bonusRemaining > 0.f) {
            const float drain = std::min(bonusRemaining, BONUS_DRAIN_RATE * dt);
            bonusRemaining -= drain;
            const int pts    = static_cast<int>(drain * 100.f);
            bonusEarned    += pts;
            player.addScore(pts);

            if (bonusRemaining < 0.5f) bonusRemaining = 0.f;  // snap do 0
        }

        // --- Tytul okna ---
        window.setTitle("Mario Contra 2 | zycia: " + std::to_string(player.getLives())
                        + " | pkt: " + std::to_string(player.getScore()));

        // ============================================================
        //  DRAW
        // ============================================================
        window.clear(sf::Color(135, 206, 235));

        for (Item& it : items)
            it.draw(window);

        // --- Swiat gry (z kamera) ---
        window.setView(gameView);
        drawTileMap(window, tileMap, tileTextures);
        goalFlag.draw(window);
        for (Enemy& enemy : enemies) enemy.draw(window);
        player.draw(window);

        for (const Checkpoint& cp : checkpoints)
            cp.draw(window);

        // --- UI (fixed, bez scrollowania) ---
        window.setView(uiView);

        if (fontLoaded)
            drawHUD(window, font, player.getLives(), player.getScore(), timeLeft, texHeart);

        // Overlay stanow
        if (gameState == GameState::GameOver && fontLoaded)
            drawOverlay(window, font, "GAME OVER",
                "Nacisnij R aby zagrac ponownie", sf::Color::Red);

        else if (gameState == GameState::Win && bonusRemaining > 0.f && fontLoaded)
            drawBonusCountdown(window, font, bonusRemaining, bonusEarned);

        else if (gameState == GameState::Win && bonusRemaining <= 0.f && fontLoaded)
            drawOverlay(window, font, "LEVEL OVER",
                "Punkty: " + formatScore(player.getScore())
                + "   Nacisnij R", sf::Color::Yellow);

        // Przywroc widok gry
        window.setView(gameView);

        window.display();
    }

    return 0;
}
