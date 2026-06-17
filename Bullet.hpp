#pragma once
#include <SFML/Graphics.hpp>

class TileMap;

class Bullet {
public:
    static constexpr float SPEED  = 500.f;   // px/s
    static constexpr float WIDTH  = 8.f;
    static constexpr float HEIGHT = 5.f;

    // direction: +1.f = prawo, -1.f = lewo
    Bullet(sf::Vector2f position, float direction);

    Bullet(sf::Vector2f position, float baseDirection, float angleDegrees);

    void update(float dt, const TileMap& tileMap);
    void draw(sf::RenderWindow& window) const;

    sf::FloatRect getBounds() const;
    bool          isActive()  const { return m_active; }
    void          deactivate()      { m_active = false; }

private:
    void initShape(sf::Vector2f position);

    sf::RectangleShape m_shape;
    sf::Vector2f m_velocity;
    bool m_active = true;
};
