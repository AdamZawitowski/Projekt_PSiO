#include <cmath>
#include "Bullet.hpp"
#include "TileMap.hpp"

namespace {
    constexpr float PI = 3.14159265358979323846f;
}

void Bullet::initShape(sf::Vector2f position) {
    m_shape.setSize({ WIDTH, HEIGHT });
    m_shape.setFillColor(sf::Color(255, 220, 50)); // zolty pocisk
    m_shape.setOrigin({ WIDTH / 2.f, HEIGHT / 2.f });
    m_shape.setPosition(position);
}

Bullet::Bullet(sf::Vector2f position, float direction)
    : m_velocity(direction* SPEED, 0.f)
{
    initShape(position);
}

Bullet::Bullet(sf::Vector2f position, float baseDirection, float angleDegrees)
{
    initShape(position);

    float baseAngle = (baseDirection > 0.f) ? 0.f : 180.f;

    float effectiveAngle = (baseDirection > 0.f) ? angleDegrees : -angleDegrees;

    float totalAngleDeg = baseAngle + effectiveAngle;
    float rad = totalAngleDeg * PI / 180.f;

    m_velocity = { std::cos(rad) * SPEED, std::sin(rad) * SPEED };
}

void Bullet::update(float dt, const TileMap& tileMap) {
    if (!m_active) return;

    m_shape.move(m_velocity * dt);

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
    if (b.position.y > mapSize.y || b.position.y + b.size.y < 0.f)
        m_active = false;
}

void Bullet::draw(sf::RenderWindow& window) const {
    if (m_active)
        window.draw(m_shape);
}

sf::FloatRect Bullet::getBounds() const {
    return m_shape.getGlobalBounds();
}
