#include "TileMap.hpp"

#include <fstream>
#include <string>

TileMap::TileMap() = default;

bool TileMap::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file)
        return false;

    std::vector<std::vector<TileType>> level;
    std::string line;

    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        if (line.empty())
            continue;

        std::vector<TileType> row;
        row.reserve(line.size());
        for (char cell : line)
            row.push_back(charToTile(cell));

        level.push_back(std::move(row));
    }

    if (level.empty())
        return false;

    const std::size_t width = level[0].size();
    for (const std::vector<TileType>& row : level) {
        if (row.size() != width)
            return false;
    }

    m_level = std::move(level);
    buildFromLevelData();
    return true;
}

void TileMap::draw(sf::RenderWindow& window) const {
    for (const sf::RectangleShape& tile : m_tiles)
        window.draw(tile);
}

TileType TileMap::getTile(int col, int row) const {
    if (m_level.empty() || m_level[0].empty())
        return TileType::Empty;

    if (col < 0 || row < 0 || col >= static_cast<int>(m_level[0].size()) || row >= static_cast<int>(m_level.size()))
        return TileType::Empty;

    return m_level[row][col];
}

bool TileMap::isSolid(int col, int row) const {
    const TileType tile = getTile(col, row);
    return tile == TileType::Ground || tile == TileType::Block || tile == TileType::Platform;
}

bool TileMap::isSolidAtPixel(sf::Vector2f position) const {
    const int col = static_cast<int>(position.x / TILE_SIZE);
    const int row = static_cast<int>(position.y / TILE_SIZE);
    return isSolid(col, row);
}

sf::Vector2u TileMap::getSizeInTiles() const {
    if (m_level.empty() || m_level[0].empty())
        return {0, 0};

    return {static_cast<unsigned>(m_level[0].size()), static_cast<unsigned>(m_level.size())};
}

sf::Vector2f TileMap::getSizeInPixels() const {
    const sf::Vector2u sizeInTiles = getSizeInTiles();
    return {sizeInTiles.x * TILE_SIZE, sizeInTiles.y * TILE_SIZE};
}

TileType TileMap::charToTile(char cell) {
    switch (cell) {
    case '#': return TileType::Ground;
    case '?': return TileType::Block;
    case 'T': return TileType::Platform;
    default:  return TileType::Empty;
    }
}

sf::Color TileMap::colorFor(TileType type) {
    switch (type) {
    case TileType::Ground:   return sf::Color(139, 90, 43);
    case TileType::Block:    return sf::Color(255, 200, 64);
    case TileType::Platform: return sf::Color(34, 139, 34);
    default:                 return sf::Color::Transparent;
    }
}

void TileMap::buildFromLevelData() {
    m_tiles.clear();

    for (int row = 0; row < static_cast<int>(m_level.size()); ++row) {
        for (int col = 0; col < static_cast<int>(m_level[row].size()); ++col) {
            const TileType type = m_level[row][col];
            if (type == TileType::Empty)
                continue;

            sf::RectangleShape tile({TILE_SIZE, TILE_SIZE});
            tile.setPosition({col * TILE_SIZE, row * TILE_SIZE});
            tile.setFillColor(colorFor(type));
            tile.setOutlineColor(sf::Color(40, 40, 40));
            tile.setOutlineThickness(1.f);
            m_tiles.push_back(tile);
        }
    }
}
