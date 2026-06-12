#include <SFML/Graphics.hpp>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include "Player.hpp"
#include "Enemy.hpp"
#include "TileMap.hpp"

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
    sf::Texture hole;       // assets/hole.png    — przepasc

    bool load() {
        struct { sf::Texture& tex; const char* path; } assets[] = {
            { dirt,     "assets/dirt.png"   },
            { dirt2,    "assets/dirt_2.png" },
            { brick,    "assets/brick.png"  },
            { pipe,     "assets/pipe.png"   },
            { question, "assets/qest.png"   },
            { hole,     "assets/hole.png"   },
        };

        bool ok = true;
        for (auto& a : assets) {
            if (!a.tex.loadFromFile(a.path)) {
                std::cerr << "[TileTextures] Nie mozna wczytac: " << a.path << "\n";
                ok = false;
            }
        }
        return ok;
    }
};

// ------------------------------------------------------------------ //
//  Pomocnicze: czy komorka nizej jest ziemia (do wykrycia gornego rzadu)//
// ------------------------------------------------------------------ //
bool isGroundBelow(const TileMap& tileMap, int col, int row) {
    return tileMap.getTile(col, row + 1) == TileType::Ground;
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
    const float TS  = TileMap::TILE_SIZE;
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
                // Gorny rzad ziemi (pod nim tez ziemia lub skraj mapy) = trawa
                // Dolny rzad ziemi = czysty dirt
                bool isTop = isGroundBelow(tileMap,
                    static_cast<int>(col), static_cast<int>(row));
                drawTileSprite(window,
                    isTop ? tex.dirt
                          : tex.dirt2,
                    static_cast<int>(col), static_cast<int>(row));
                break;
            }

            case TileType::Brick:
                drawTileSprite(window,
                    tex.brick,
                    static_cast<int>(col), static_cast<int>(row));
                break;

            case TileType::QuestionBlock:
                drawTileSprite(window,
                    tex.question,
                    static_cast<int>(col), static_cast<int>(row));
                break;

            case TileType::MetalBlock:
                // Brak dedykowanego assetu — uzywamy dirt2 jako fallback
                drawTileSprite(window,
                    tex.dirt2,
                    static_cast<int>(col), static_cast<int>(row));
                break;

            case TileType::Platform:
                drawTileSprite(window,
                    tex.pipe,
                    static_cast<int>(col), static_cast<int>(row));
                break;

            case TileType::Empty: {
                // Rysuj hole.png tylko w miejscu przepasci w rzedach ziemi
                // (zeby przepasc byla widoczna pod poziomem gruntu)
                bool inGroundZone = false;
                for (int checkRow = static_cast<int>(row);
                     checkRow < static_cast<int>(sizeInTiles.y); ++checkRow)
                {
                    if (tileMap.getTile(static_cast<int>(col), checkRow)
                            == TileType::Ground) {
                        inGroundZone = true;
                        break;
                    }
                }
                if (inGroundZone)
                    drawTileSprite(window,
                        tex.hole,
                        static_cast<int>(col), static_cast<int>(row));
                break;
            }

            default:
                break;
            }
        }
    }
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

} // namespace

// ================================================================== //
//  main                                                                //
// ================================================================== //
int main() {
    sf::RenderWindow window(sf::VideoMode({800, 480}), "Mario Contra 2");
    window.setFramerateLimit(60);

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

    sf::Clock clock;

    const sf::Vector2f viewSize(
        static_cast<float>(window.getSize().x),
        static_cast<float>(window.getSize().y));
    const float levelWidth       = tileMap.getSizeInPixels().x;
    const float maxCameraCenterX = std::max(viewSize.x / 2.f,
                                            levelWidth - viewSize.x / 2.f);

    sf::View view(sf::FloatRect({0.f, 0.f}, viewSize));
    view.setCenter(viewSize / 2.f);

    constexpr float cameraLerpSpeed = 5.f;

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();
            if (const auto* key = event->getIf<sf::Event::KeyPressed>())
                if (key->code == sf::Keyboard::Key::Escape)
                    window.close();
        }

        const float dt = clock.restart().asSeconds();

        // --- UPDATE ---
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
            } else if (enemy.getBounds().findIntersection(player.getBounds())) {
                player.loseLife();
            }
        }

        // --- Kamera ---
        const sf::FloatRect playerBounds = player.getBounds();
        float targetCenterX = playerBounds.position.x + playerBounds.size.x / 2.f;
        if (targetCenterX < viewSize.x / 2.f)  targetCenterX = viewSize.x / 2.f;
        if (targetCenterX > maxCameraCenterX)   targetCenterX = maxCameraCenterX;

        sf::Vector2f center = view.getCenter();
        const float lerp    = std::min(1.f, cameraLerpSpeed * dt);
        center.x += (targetCenterX - center.x) * lerp;
        center.y  = viewSize.y / 2.f;
        view.setCenter(center);

        window.setTitle(
            "Mario Contra 2 | zycia: " + std::to_string(player.getLives())
            + " | pkt: "               + std::to_string(player.getScore()));

        // --- DRAW ---
        window.clear(sf::Color(135, 206, 235));
        window.setView(view);
        drawTileMap(window, tileMap, tileTextures);
        for (Enemy& enemy : enemies)
            enemy.draw(window);
        player.draw(window);
        window.display();
    }

    return 0;
}