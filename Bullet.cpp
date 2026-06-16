#include "Bullet.hpp"
#include "TileMap.hpp"

Bullet::Bullet(sf::Vector2f position, float direction)
    : m_direction(direction)
{
    m_shape.setSize({ WIDTH, HEIGHT });
    m_shape.setFillColor(sf::Color(255, 220, 50)); // zolty pocisk
    // Wyśrodkuj pocisk w pionie względem podanej pozycji
    m_shape.setOrigin({ WIDTH / 2.f, HEIGHT / 2.f });
    m_shape.setPosition(position);
}

void Bullet::update(float dt, const TileMap& tileMap) {
    if (!m_active) return;

    m_shape.move({ m_direction * SPEED * dt, 0.f });

    // Dezaktywuj po trafieniu w solidny kafelek
    sf::FloatRect b = m_shape.getGlobalBounds();
    sf::Vector2f  center = { b.position.x + b.size.x / 2.f,
                              b.position.y + b.size.y / 2.f };
    if (tileMap.isSolidAtPixel(center))
        m_active = false;

    // Dezaktywuj gdy wyleci poza mape
    sf::Vector2f mapSize = tileMap.getSizeInPixels();
    if (b.position.x > mapSize.x || b.position.x + b.size.x < 0.f)
        m_active = false;
}

void Bullet::draw(sf::RenderWindow& window) const {
    if (m_active)
        window.draw(m_shape);
}

sf::FloatRect Bullet::getBounds() const {
    return m_shape.getGlobalBounds();
}
