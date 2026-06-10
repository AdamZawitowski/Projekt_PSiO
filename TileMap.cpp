#include "TileMap.hpp"

#include <fstream>
#include <string>

bool Tile::isSolid() const {
    return type == TileType::Ground
        || type == TileType::Brick
        || type == TileType::QuestionBlock
        || type == TileType::MetalBlock
        || type == TileType::Platform;
}

TileMap::TileMap() = default;

bool TileMap::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file)
        return false;

    initTileTexture();

    std::vector<std::vector<Tile>> level;
    std::string line;

    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        if (line.empty())
            continue;

        std::vector<Tile> row;
        row.reserve(line.size());
        for (char cell : line) {
            Tile tile;
            applyCharToTile(tile, cell);
            row.push_back(std::move(tile));
        }

        level.push_back(std::move(row));
    }

    if (level.empty())
        return false;

    const std::size_t width = level[0].size();
    for (const std::vector<Tile>& row : level) {
        if (row.size() != width)
            return false;
    }

    m_level = std::move(level);
    buildFromLevelData();
    return true;
}

void TileMap::draw(sf::RenderWindow& window) const {
    for (const std::vector<Tile>& row : m_level) {
        for (const Tile& tile : row) {
            if (tile.sprite)
                window.draw(*tile.sprite);
        }
    }
}

TileType TileMap::getTile(int col, int row) const {
    if (m_level.empty() || m_level[0].empty())
        return TileType::Empty;

    if (col < 0 || row < 0 || col >= static_cast<int>(m_level[0].size()) || row >= static_cast<int>(m_level.size()))
        return TileType::Empty;

    return m_level[row][col].type;
}

Tile* TileMap::getTileAt(float x, float y) {
    const int col = static_cast<int>(x / TILE_SIZE);
    const int row = static_cast<int>(y / TILE_SIZE);

    if (m_level.empty() || m_level[0].empty())
        return nullptr;

    if (col < 0 || row < 0 || col >= static_cast<int>(m_level[0].size()) || row >= static_cast<int>(m_level.size()))
        return nullptr;

    Tile& tile = m_level[row][col];
    if (tile.type == TileType::Empty)
        return nullptr;

    return &tile;
}

const Tile* TileMap::getTileAt(float x, float y) const {
    return const_cast<TileMap*>(this)->getTileAt(x, y);
}

bool TileMap::modifyTile(int row, int col, char newType) {
    if (m_level.empty() || m_level[0].empty())
        return false;

    if (row < 0 || col < 0 || row >= static_cast<int>(m_level.size()) || col >= static_cast<int>(m_level[0].size()))
        return false;

    applyCharToTile(m_level[row][col], newType);
    rebuildTile(row, col);
    return true;
}

bool TileMap::isSolid(int col, int row) const {
    if (m_level.empty() || m_level[0].empty())
        return false;

    if (col < 0 || row < 0 || col >= static_cast<int>(m_level[0].size()) || row >= static_cast<int>(m_level.size()))
        return false;

    return m_level[row][col].isSolid();
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
    case '#':
    case 'G': return TileType::Ground;
    case 'B': return TileType::Brick;
    case '?':
    case 'Q': return TileType::QuestionBlock;
    case 'M': return TileType::MetalBlock;
    case 'T':
    case 'P': return TileType::Platform;
    default:  return TileType::Empty;
    }
}

void TileMap::applyCharToTile(Tile& tile, char cell) {
    tile.type = charToTile(cell);
    tile.destructible = false;
    tile.hasBonus = false;
    tile.activated = false;
    tile.bonus = BonusType::None;

    switch (cell) {
    case 'B':
        tile.destructible = true;
        break;
    case '?':
    case 'Q':
        tile.hasBonus = true;
        tile.bonus = BonusType::Coin;
        break;
    case 'M':
        tile.activated = true;
        break;
    default:
        break;
    }
}

sf::Color TileMap::colorFor(TileType type) {
    switch (type) {
    case TileType::Ground:         return sf::Color(139, 90, 43);
    case TileType::Brick:          return sf::Color(200, 80, 60);
    case TileType::QuestionBlock:  return sf::Color(255, 200, 64);
    case TileType::MetalBlock:     return sf::Color(160, 160, 170);
    case TileType::Platform:       return sf::Color(34, 139, 34);
    default:                       return sf::Color::Transparent;
    }
}

void TileMap::initTileTexture() {
    if (m_textureReady)
        return;

    sf::Image image;
    image.resize({static_cast<unsigned>(TILE_SIZE), static_cast<unsigned>(TILE_SIZE)}, sf::Color::White);
    (void)m_tileTexture.loadFromImage(image);
    m_textureReady = true;
}

void TileMap::buildFromLevelData() {
    for (int row = 0; row < static_cast<int>(m_level.size()); ++row) {
        for (int col = 0; col < static_cast<int>(m_level[row].size()); ++col)
            rebuildTile(row, col);
    }
}

void TileMap::rebuildTile(int row, int col) {
    Tile& tile = m_level[row][col];

    if (tile.type == TileType::Empty) {
        tile.sprite.reset();
        return;
    }

    tile.sprite.emplace(m_tileTexture);
    tile.sprite->setPosition({col * TILE_SIZE, row * TILE_SIZE});
    tile.sprite->setColor(colorFor(tile.type));
}
