#pragma once

#include <SFML/Graphics.hpp>
#include <cstdint>
#include <string>
#include <vector>

enum class TileType : std::uint8_t {
    Empty = 0,
    Ground,
    Block,
    Platform
};

class TileMap {
public:
    static constexpr float TILE_SIZE = 32.f;

    TileMap();

    bool loadFromFile(const std::string& filename);

    void draw(sf::RenderWindow& window) const;

    TileType getTile(int col, int row) const;
    bool isSolid(int col, int row) const;
    bool isSolidAtPixel(sf::Vector2f position) const;

    sf::Vector2u getSizeInTiles() const;
    sf::Vector2f getSizeInPixels() const;

private:
    static TileType charToTile(char cell);
    static sf::Color colorFor(TileType type);
    void buildFromLevelData();

    std::vector<std::vector<TileType>> m_level;
    std::vector<sf::RectangleShape> m_tiles;
};
