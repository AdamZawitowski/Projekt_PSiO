#pragma once
#include <SFML/Graphics.hpp>

enum class ItemType {
    Coin,
    Mushroom,
    Heart
};

class Item {
public:
    Item(sf::Vector2f pos, ItemType type);

    void update(float dt);
    void draw(sf::RenderWindow& window) const;

    sf::FloatRect getBounds() const;
    ItemType getType() const { return m_type; }
    bool isCollected() const { return m_collected; }
    void collect() { m_collected = true; }

    void applyGravity(float dt);
    void stopFalling();
    bool isFalling() const { return m_velocity.y > 0.f; }

    int m_sourceRow = -1;
    int m_sourceCol = -1;

    float m_ignoreCollisionTime = 0.55f;

    void setSourceBlock(int col, int row) {
        m_sourceCol = col;
        m_sourceRow = row;
    }

    void snapToGround(float groundY);
private:
    sf::RectangleShape m_shape;
    sf::Vector2f m_velocity;
    ItemType m_type;
    bool m_collected = false;
};
