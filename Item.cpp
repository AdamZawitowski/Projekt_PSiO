#include "Item.hpp"

Item::Item(sf::Vector2f pos, ItemType type)
    : m_type(type), m_velocity(0.f, -150.f)
{
    m_shape.setSize({ 20.f, 20.f });
    m_shape.setFillColor(type == ItemType::Coin ? sf::Color::Yellow : sf::Color::Red);
    m_shape.setPosition(pos);
}

void Item::update(float dt) {
    m_velocity.y += 600.f * dt;
    m_shape.move(m_velocity * dt);
}

void Item::draw(sf::RenderWindow& window) const {
    if (!m_collected)
        window.draw(m_shape);
}

sf::FloatRect Item::getBounds() const {
    return m_shape.getGlobalBounds();
}
