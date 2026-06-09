#include <SFML/Graphics.hpp>
#include <algorithm>
#include <string>
#include "Player.hpp"
#include "TileMap.hpp"

namespace {

sf::Color colorForTile(const Tile& tile) {
    switch (tile.type) {
    case TileType::Ground:        return sf::Color(139, 90, 43);
    case TileType::Brick:         return sf::Color(200, 80, 60);
    case TileType::QuestionBlock: return sf::Color(255, 200, 64);
    case TileType::MetalBlock:    return sf::Color(160, 160, 170);
    default:                      return sf::Color::Transparent;
    }
}

sf::Color outlineForTile(const Tile& tile) {
    switch (tile.type) {
    case TileType::QuestionBlock:
        return tile.hasBonus && !tile.activated ? sf::Color(200, 120, 0) : sf::Color(80, 80, 80);
    case TileType::Brick:
        return tile.destructible ? sf::Color(120, 40, 30) : sf::Color(40, 40, 40);
    default:
        return sf::Color(40, 40, 40);
    }
}

float outlineThicknessForTile(const Tile& tile) {
    if (tile.type == TileType::QuestionBlock && tile.hasBonus && !tile.activated)
        return 2.f;
    return 1.f;
}

bool isPipeTop(const TileMap& tileMap, int col, int row) {
    if (row <= 0)
        return true;
    return tileMap.getTile(col, row - 1) != TileType::Platform;
}

void drawPipeTile(sf::RenderWindow& window, const TileMap& tileMap, int col, int row) {
    const float x = col * TileMap::TILE_SIZE;
    const float y = row * TileMap::TILE_SIZE;
    const float size = TileMap::TILE_SIZE;

    sf::RectangleShape body({size, size});
    body.setPosition({x, y});
    body.setFillColor(sf::Color(0, 128, 0));
    window.draw(body);

    sf::RectangleShape highlight({7.f, size});
    highlight.setPosition({x + 5.f, y});
    highlight.setFillColor(sf::Color(96, 220, 96));
    window.draw(highlight);

    sf::RectangleShape shadow({5.f, size});
    shadow.setPosition({x + size - 7.f, y});
    shadow.setFillColor(sf::Color(0, 76, 0));
    window.draw(shadow);

    sf::RectangleShape innerShade({size - 14.f, size});
    innerShade.setPosition({x + 12.f, y});
    innerShade.setFillColor(sf::Color(0, 104, 0));
    window.draw(innerShade);

    if (isPipeTop(tileMap, col, row)) {
        sf::RectangleShape rim({size + 6.f, 8.f});
        rim.setPosition({x - 3.f, y - 4.f});
        rim.setFillColor(sf::Color(0, 168, 0));
        rim.setOutlineColor(sf::Color(0, 56, 0));
        rim.setOutlineThickness(1.f);
        window.draw(rim);

        sf::RectangleShape rimHighlight({7.f, 6.f});
        rimHighlight.setPosition({x + 2.f, y - 3.f});
        rimHighlight.setFillColor(sf::Color(120, 236, 120));
        window.draw(rimHighlight);
    }
}

void drawStandardTile(sf::RenderWindow& window, const Tile& tile, int col, int row) {
    sf::RectangleShape tileShape({TileMap::TILE_SIZE, TileMap::TILE_SIZE});
    tileShape.setPosition({col * TileMap::TILE_SIZE, row * TileMap::TILE_SIZE});
    tileShape.setFillColor(colorForTile(tile));
    tileShape.setOutlineColor(outlineForTile(tile));
    tileShape.setOutlineThickness(outlineThicknessForTile(tile));
    window.draw(tileShape);
}

void drawTileMap(sf::RenderWindow& window, const TileMap& tileMap) {
    const sf::Vector2u sizeInTiles = tileMap.getSizeInTiles();

    for (unsigned row = 0; row < sizeInTiles.y; ++row) {
        for (unsigned col = 0; col < sizeInTiles.x; ++col) {
            const float sampleX = col * TileMap::TILE_SIZE + TileMap::TILE_SIZE * 0.5f;
            const float sampleY = row * TileMap::TILE_SIZE + TileMap::TILE_SIZE * 0.5f;
            const Tile* tile = tileMap.getTileAt(sampleX, sampleY);
            if (!tile)
                continue;

            if (tile->type == TileType::Platform)
                drawPipeTile(window, tileMap, static_cast<int>(col), static_cast<int>(row));
            else
                drawStandardTile(window, *tile, static_cast<int>(col), static_cast<int>(row));
        }
    }
}

struct TileCounts {
    int ground = 0;
    int question = 0;
    int pipes = 0;
    int brick = 0;
    int metal = 0;
};

TileCounts countTiles(const TileMap& tileMap) {
    TileCounts counts;
    const sf::Vector2u sizeInTiles = tileMap.getSizeInTiles();

    for (unsigned row = 0; row < sizeInTiles.y; ++row) {
        for (unsigned col = 0; col < sizeInTiles.x; ++col) {
            switch (tileMap.getTile(static_cast<int>(col), static_cast<int>(row))) {
            case TileType::Ground:        ++counts.ground; break;
            case TileType::QuestionBlock: ++counts.question; break;
            case TileType::Platform:      ++counts.pipes; break;
            case TileType::Brick:         ++counts.brick; break;
            case TileType::MetalBlock:    ++counts.metal; break;
            default: break;
            }
        }
    }

    return counts;
}

} // namespace

int main() {
    sf::RenderWindow window(sf::VideoMode({800, 480}), "Mario Contra 2");

    TileMap tileMap;
    if (!tileMap.loadFromFile("level1.txt"))
        return 1;

    const TileCounts tileCounts = countTiles(tileMap);

    Player player;
    sf::Clock clock;

    const sf::Vector2f viewSize(
        static_cast<float>(window.getSize().x),
        static_cast<float>(window.getSize().y));
    const float levelWidth = tileMap.getSizeInPixels().x;
    const float maxCameraCenterX = std::max(viewSize.x / 2.f, levelWidth - viewSize.x / 2.f);

    sf::View view(sf::FloatRect({0.f, 0.f}, viewSize));
    view.setCenter(viewSize / 2.f);

    constexpr float cameraLerpSpeed = 5.f;

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();
        }

        const float dt = clock.restart().asSeconds();

        player.update(dt);
        player.resolveCollisions(tileMap);

        const sf::FloatRect playerBounds = player.getBounds();
        float targetCenterX = playerBounds.position.x + playerBounds.size.x / 2.f;

        if (targetCenterX < viewSize.x / 2.f)
            targetCenterX = viewSize.x / 2.f;
        if (targetCenterX > maxCameraCenterX)
            targetCenterX = maxCameraCenterX;

        sf::Vector2f center = view.getCenter();
        const float lerp = std::min(1.f, cameraLerpSpeed * dt);
        center.x += (targetCenterX - center.x) * lerp;
        center.y = viewSize.y / 2.f;
        view.setCenter(center);

        window.setTitle(
            "Mario Contra 2 | zycia: " + std::to_string(player.getLives())
            + " | pkt: " + std::to_string(player.getScore())
            + " | ?: " + std::to_string(tileCounts.question)
            + " | rury: " + std::to_string(tileCounts.pipes));

        window.clear(sf::Color(135, 206, 235));
        window.setView(view);
        drawTileMap(window, tileMap);
        player.draw(window);
        window.display();
    }

    return 0;
}
