#include <SFML/Graphics.hpp>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include "Player.hpp"
#include "Enemy.hpp"
#include "TileMap.hpp"
#include "GoalFlag.hpp"
#include "GameState.hpp"

namespace {

    // ------------------------------------------------------------------ //
    //  Tekstury kafelkow — ladowane raz przed game loopem                  //
    // ------------------------------------------------------------------ //
    struct TileTextures {
        sf::Texture dirt;       // assets/dirt.png    — ziemia z trawa (gorny rzad)
        sf::Texture dirt2;      // assets/dirt_2.png  — ziemia pod spodem
        sf::Texture brick;      // assets/brick.png   — cegly
        sf::Texture pipe;       // assets/pipe.png    — rura
        sf::Texture question;   // assets/qest.png    — blok z pytajnikiem
        sf::Texture holeLeft;   // assets/hole.png    — lewy brzeg przepasci
        sf::Texture holeRight;  // assets/hole_2.png  — prawy brzeg przepasci
        sf::Texture holeMid;    // assets/hole_3.png  — srodek przepasci

        bool load() {
            bool ok = true;

            if (!dirt.loadFromFile("assets/dirt.png"))
            {
                std::cerr << "[TileTextures] brak: assets/dirt.png\n";   ok = false;
            }
            if (!dirt2.loadFromFile("assets/dirt_2.png"))
            {
                std::cerr << "[TileTextures] brak: assets/dirt_2.png\n"; ok = false;
            }
            if (!brick.loadFromFile("assets/brick.png"))
            {
                std::cerr << "[TileTextures] brak: assets/brick.png\n";  ok = false;
            }
            if (!pipe.loadFromFile("assets/pipe.png"))
            {
                std::cerr << "[TileTextures] brak: assets/pipe.png\n";   ok = false;
            }
            if (!question.loadFromFile("assets/qest.png"))
            {
                std::cerr << "[TileTextures] brak: assets/qest.png\n";   ok = false;
            }
            if (!holeLeft.loadFromFile("assets/hole.png"))
            {
                std::cerr << "[TileTextures] brak: assets/hole.png\n";    ok = false;
            }
            if (!holeRight.loadFromFile("assets/hole_2.png"))
            {
                std::cerr << "[TileTextures] brak: assets/hole_2.png\n";  ok = false;
            }
            if (!holeMid.loadFromFile("assets/hole_3.png"))
            {
                std::cerr << "[TileTextures] brak: assets/hole_3.png\n";  ok = false;
            }

            return ok;
        }
    };

    // ------------------------------------------------------------------ //
    //  Pomocnicze: czy komorka nizej jest ziemia (do wykrycia gornego rzadu)//
    // ------------------------------------------------------------------ //
    // Zwraca true jesli ten rzad to GORNY rzad ziemi (z trawa) -
    // czyli pod nim jest kolejny rzad ziemi.
    bool isTopGroundRow(const TileMap& tileMap, int col, int row) {
        return tileMap.getTile(col, row + 1) == TileType::Ground;
    }

    bool isPipeTopTile(const TileMap& tileMap, int col, int row) {
        return tileMap.getTile(col, row - 1) != TileType::Platform;
    }

    int pipeStackHeight(const TileMap& tileMap, int col, int row) {
        int height = 1;
        while (tileMap.getTile(col, row + height) == TileType::Platform) {
            ++height;
        }
        return height;
    }

    void drawPipeStack(sf::RenderWindow& window,
        const TileTextures& tex,
        int col, int row,
        int stackHeight)
    {
        const float TS = TileMap::TILE_SIZE;
        const sf::Vector2u sz = tex.pipe.getSize();

        sf::Sprite sprite(tex.pipe);
        sprite.setPosition({ col * TS, row * TS });
        sprite.setScale({ TS / sz.x, (TS * stackHeight) / sz.y });
        window.draw(sprite);
    }

    void drawTileSprite(sf::RenderWindow& window,
        const sf::Texture& tex,
        int col, int row);

    void drawPitBottomRow(sf::RenderWindow& window,
        const TileMap& tileMap,
        const TileTextures& tex)
    {
        const sf::Vector2u sizeInTiles = tileMap.getSizeInTiles();
        if (sizeInTiles.y == 0)
            return;

        const int row = static_cast<int>(sizeInTiles.y) - 1;
        int col = 0;

        while (col < static_cast<int>(sizeInTiles.x)) {
            if (tileMap.getTile(col, row) != TileType::Empty) {
                ++col;
                continue;
            }

            const int startCol = col;
            while (col < static_cast<int>(sizeInTiles.x) &&
                tileMap.getTile(col, row) == TileType::Empty) {
                ++col;
            }

            const int endCol = col - 1;
            for (int x = startCol; x <= endCol; ++x) {
                const sf::Texture* selected = &tex.holeMid;

                if (startCol == endCol) {
                    selected = &tex.holeLeft;
                }
                else if (x == startCol) {
                    selected = &tex.holeLeft;
                }
                else if (x == endCol) {
                    selected = &tex.holeRight;
                }

                drawTileSprite(window, *selected, x, row);
            }
        }
    }

    // ------------------------------------------------------------------ //
    //  Rysowanie pojedynczego kafelka jako sprajt                          //
    //  Sprajt pozycjonowany lewym-gornym rogiem na (col*TS, row*TS)        //
    //  i skalowany do dokladnie TILE_SIZE x TILE_SIZE                      //
    // ------------------------------------------------------------------ //
    void drawTileSprite(sf::RenderWindow& window,
        const sf::Texture& tex,
        int col, int row)
    {
        const float TS = TileMap::TILE_SIZE;
        sf::Vector2u sz = tex.getSize();

        sf::Sprite sprite(tex);
        sprite.setPosition({ col * TS, row * TS });
        // Skaluj teksture do rozmiaru kafelka (na wypadek roznych rozmiarow PNG)
        sprite.setScale({ TS / sz.x, TS / sz.y });
        window.draw(sprite);
    }

    // ------------------------------------------------------------------ //
    //  Glowna funkcja rysowania mapy                                        //
    // ------------------------------------------------------------------ //
    void drawTileMap(sf::RenderWindow& window,
        const TileMap& tileMap,
        const TileTextures& tex)
    {
        const sf::Vector2u sizeInTiles = tileMap.getSizeInTiles();

        for (unsigned row = 0; row < sizeInTiles.y; ++row) {
            for (unsigned col = 0; col < sizeInTiles.x; ++col) {

                const TileType type = tileMap.getTile(
                    static_cast<int>(col), static_cast<int>(row));

                switch (type) {

                case TileType::Ground: {
                    bool isTop = isTopGroundRow(tileMap,
                        static_cast<int>(col), static_cast<int>(row));
                    drawTileSprite(window,
                        isTop ? tex.dirt : tex.dirt2,
                        static_cast<int>(col), static_cast<int>(row));
                    break;
                }

                case TileType::Brick:
                    drawTileSprite(window, tex.brick,
                        static_cast<int>(col), static_cast<int>(row));
                    break;

                case TileType::QuestionBlock:
                    drawTileSprite(window, tex.question,
                        static_cast<int>(col), static_cast<int>(row));
                    break;

                case TileType::MetalBlock:
                    drawTileSprite(window, tex.dirt2,
                        static_cast<int>(col), static_cast<int>(row));
                    break;

                case TileType::Platform:
                    if (!isPipeTopTile(tileMap, static_cast<int>(col), static_cast<int>(row)))
                        break;
                    drawPipeStack(window, tex,
                        static_cast<int>(col), static_cast<int>(row),
                        pipeStackHeight(tileMap, static_cast<int>(col), static_cast<int>(row)));
                    break;

                case TileType::Empty:
                    // Abyss floor is rendered in a separate pass on the bottom row.
                    break;

                default:
                    break;
                }
            }
        }

        drawPitBottomRow(window, tileMap, tex);
    }

    // ------------------------------------------------------------------ //
    //  countTiles — bez zmian, potrzebne do statystyk                      //
    // ------------------------------------------------------------------ //
    struct TileCounts {
        int ground = 0, question = 0, pipes = 0, brick = 0, metal = 0;
    };

    TileCounts countTiles(const TileMap& tileMap) {
        TileCounts counts;
        const sf::Vector2u sizeInTiles = tileMap.getSizeInTiles();
        for (unsigned row = 0; row < sizeInTiles.y; ++row) {
            for (unsigned col = 0; col < sizeInTiles.x; ++col) {
                switch (tileMap.getTile(static_cast<int>(col), static_cast<int>(row))) {
                case TileType::Ground:        ++counts.ground;   break;
                case TileType::QuestionBlock: ++counts.question; break;
                case TileType::Platform:      ++counts.pipes;    break;
                case TileType::Brick:         ++counts.brick;    break;
                case TileType::MetalBlock:    ++counts.metal;    break;
                default: break;
                }
            }
        }
        return counts;
    }

    void drawOverlay(sf::RenderWindow& window, const sf::Font& font,
                 const std::string& title, const std::string& subtitle,
                 sf::Color titleColor) {
    sf::RectangleShape bg({static_cast<float>(window.getSize().x),
                           static_cast<float>(window.getSize().y)});
    bg.setFillColor(sf::Color(0, 0, 0, 160));
    window.draw(bg);

    sf::Text titleText(font, title, 36);
    titleText.setFillColor(titleColor);
    titleText.setStyle(sf::Text::Bold);
    sf::FloatRect tb = titleText.getLocalBounds();
    titleText.setOrigin({tb.position.x + tb.size.x / 2.f,
                         tb.position.y + tb.size.y / 2.f});
    titleText.setPosition({window.getSize().x / 2.f,
                           window.getSize().y / 2.f - 50.f});
    window.draw(titleText);

    sf::Text subText(font, subtitle, 18);
    subText.setFillColor(sf::Color::White);
    sf::FloatRect sb = subText.getLocalBounds();
    subText.setOrigin({sb.position.x + sb.size.x / 2.f,
                       sb.position.y + sb.size.y / 2.f});
    subText.setPosition({window.getSize().x / 2.f,
                         window.getSize().y / 2.f + 30.f});
    window.draw(subText);
}

} // namespace

// ================================================================== //
//  main                                                                //
// ================================================================== //
int main() {
    sf::RenderWindow window(sf::VideoMode({ 800, 480 }), "Mario Contra 2");
    window.setFramerateLimit(60);

    // --- Czcionka ---
    sf::Font font;
    bool fontLoaded = false;
    if (!font.openFromFile("assets/font.ttf"))
        std::cerr << "[main] Brak czcionki: assets/font.ttf\n";
    else
        fontLoaded = true;

    TileMap tileMap;
    if (!tileMap.loadFromFile("level1.txt"))
        return 1;

    // Laduj tekstury kafelkow — raz, przed petla
    TileTextures tileTextures;
    if (!tileTextures.load()) {
        std::cerr << "[main] Nie wszystkie tekstury kafelkow zostaly zaladowane.\n";
        // Kontynuujemy — brakujace tekstury beda niewidoczne (bialy kwadrat SFML)
    }

    const TileCounts tileCounts = countTiles(tileMap);

    Player player;

    // Wrogowie — dodaj tyle ile chcesz, podaj pozycje startowe
    std::vector<Enemy> enemies;
    enemies.emplace_back(sf::Vector2f(400.f, 380.f));
    enemies.emplace_back(sf::Vector2f(600.f, 380.f));

    const float levelWidth = tileMap.getSizeInPixels().x;

    // flaga na koncu poziomu
    GoalFlag goalFlag(sf::Vector2f(levelWidth - 96.f, 352.f));


    // stan gry
    GameState gameState = GameState::Playing;

    sf::Clock clock;

    const sf::Vector2f viewSize(
        static_cast<float>(window.getSize().x),
        static_cast<float>(window.getSize().y));
    const float maxCameraCenterX = std::max(viewSize.x / 2.f,
        levelWidth - viewSize.x / 2.f);

    sf::View view(sf::FloatRect({ 0.f, 0.f }, viewSize));
    view.setCenter(viewSize / 2.f);
    const sf::View uiView = window.getDefaultView();

    constexpr float cameraLerpSpeed = 5.f;

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();
            if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                if (key->code == sf::Keyboard::Key::Escape)
                    window.close();
                if (key->code == sf::Keyboard::Key::R &&
                    gameState != GameState::Playing) {
                    player = Player();
                    enemies.clear();
                    enemies.emplace_back(sf::Vector2f(400.f, 380.f));
                    enemies.emplace_back(sf::Vector2f(600.f, 380.f));
                    gameState = GameState::Playing;
                }
            }
        }

        const float dt = clock.restart().asSeconds();

        // --- UPDATE ---
        if (gameState == GameState::Playing) {
        player.update(dt);
        player.resolveCollisions(tileMap);

        for (Enemy& enemy : enemies)
            enemy.update(dt, tileMap);

        // --- Kolizje gracza z wrogami ---
        for (Enemy& enemy : enemies) {
            if (!enemy.isAlive()) continue;

            bool stompedFromAbove = enemy.checkCollisionWithPlayer(player);
            if (stompedFromAbove) {
                enemy.kill();
                player.addScore(100);
            }
            else if (enemy.getBounds().findIntersection(player.getBounds())) {
                player.loseLife();
            }
        }

        // --- Kamera ---
        const sf::FloatRect playerBounds = player.getBounds();
        float targetCenterX = playerBounds.position.x + playerBounds.size.x / 2.f;
        if (targetCenterX < viewSize.x / 2.f)  targetCenterX = viewSize.x / 2.f;
        if (targetCenterX > maxCameraCenterX)   targetCenterX = maxCameraCenterX;

        sf::Vector2f center = view.getCenter();
        const float lerp = std::min(1.f, cameraLerpSpeed * dt);
        center.x += (targetCenterX - center.x) * lerp;
        center.y = viewSize.y / 2.f;
        view.setCenter(center);

        // sprawdz game over
        if (player.getLives() <= 0 && !player.isDying())
            gameState = GameState::GameOver;

        // sprawdz wygranie
        if (goalFlag.checkCollisionWithPlayer(player))
            gameState = GameState::Win;

        } // koniec if (gameState == Playing)

        window.setTitle(
            "Mario Contra 2 | zycia: " + std::to_string(player.getLives())
            + " | pkt: " + std::to_string(player.getScore()));

        // --- DRAW ---
        window.clear(sf::Color(135, 206, 235));
        window.setView(view);
        drawTileMap(window, tileMap, tileTextures);
        goalFlag.draw(window);
        for (Enemy& enemy : enemies)
            enemy.draw(window);
        player.draw(window);

        // ekran overlay
        window.setView(uiView);
        if (gameState == GameState::GameOver && fontLoaded)
            drawOverlay(window, font, "GAME OVER",
                "Nacisnij R aby zagrac ponownie", sf::Color::Red);
        else if (gameState == GameState::Win && fontLoaded)
            drawOverlay(window, font, "WYGRALES!",
                "Punkty: " + std::to_string(player.getScore())
                + "   Nacisnij R aby zagrac ponownie", sf::Color::Yellow);
        window.setView(view);

        window.display();
    }

    return 0;
}