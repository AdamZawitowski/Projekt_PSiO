#pragma once

#include <SFML/Graphics.hpp>

class Player {
public:
    Player();

    void update(float dt);
    void draw(sf::RenderWindow& window) const;

private:
    sf::RectangleShape m_shape;
    float m_speed;
};
