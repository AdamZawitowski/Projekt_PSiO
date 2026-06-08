#include "Player.hpp"

Player::Player() : m_speed(200.f) {
    m_shape.setSize({40.f, 60.f});
    m_shape.setFillColor(sf::Color::Red);
    m_shape.setPosition({80.f, 70.f});
}

void Player::update(float dt) {
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
        m_shape.move({-m_speed * dt, 0.f});

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
        m_shape.move({m_speed * dt, 0.f});
}

void Player::draw(sf::RenderWindow& window) const {
    window.draw(m_shape);
}
