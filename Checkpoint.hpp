#pragma once
#include <SFML/Graphics.hpp>
#include "Player.hpp"

class Checkpoint {
public:
    static constexpr float WIDTH = 20.f;
    static constexpr float HEIGHT = 40.f;

    explicit Checkpoint(sf::Vector2f pos)
        : m_position(pos), m_activated(false)
    {
        m_shape.setSize({ WIDTH, HEIGHT });
        m_shape.setFillColor(sf::Color(200, 200, 50));
        m_shape.setPosition(pos);
    }

    void draw(sf::RenderWindow& window) const {
        window.draw(m_shape);
    }

    bool checkCollision(const Player& player) {
        if (m_activated) return false;

        if (m_shape.getGlobalBounds().findIntersection(player.getBounds())) {
            m_activated = true;
            m_shape.setFillColor(sf::Color(0, 255, 0));
            return true;
        }
        return false;
    }
    void reset() {
        m_activated = false;
        m_shape.setFillColor(sf::Color(200, 200, 50));
    }

    bool isActivated() const { return m_activated; }
    sf::Vector2f getPosition() const { return m_position; }

private:
    sf::RectangleShape m_shape;
    sf::Vector2f m_position;
    bool m_activated;
};
