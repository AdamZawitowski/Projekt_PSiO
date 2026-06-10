#pragma once

#include <SFML/Graphics.hpp>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

enum class TileType : std::uint8_t {
    Empty = 0,
    Ground,
    Brick,
    QuestionBlock,
    MetalBlock,
    Platform
};

enum class BonusType : std::uint8_t {
    None,
    Coin,
    Mushroom
};

struct Tile {
    TileType type = TileType::Empty;
    std::optional<sf::Sprite> sprite;
    bool destructible = false;
    bool hasBonus = false;
    bool activated = false;
    BonusType bonus = BonusType::None;

    bool isSolid() const;
};

class TileMap {
public:
    static constexpr float TILE_SIZE = 32.f;

    TileMap();

    bool loadFromFile(const std::string& filename);

    void draw(sf::RenderWindow& window) const;

    TileType getTile(int col, int row) const;
    Tile* getTileAt(float x, float y);
    const Tile* getTileAt(float x, float y) const;
    bool modifyTile(int row, int col, char newType);

    bool isSolid(int col, int row) const;
    bool isSolidAtPixel(sf::Vector2f position) const;

    sf::Vector2u getSizeInTiles() const;
    sf::Vector2f getSizeInPixels() const;

private:
    static TileType charToTile(char cell);
    static void applyCharToTile(Tile& tile, char cell);
    static sf::Color colorFor(TileType type);

    void initTileTexture();
    void buildFromLevelData();
    void rebuildTile(int row, int col);

    std::vector<std::vector<Tile>> m_level;
    sf::Texture m_tileTexture;
    bool m_textureReady = false;
};
