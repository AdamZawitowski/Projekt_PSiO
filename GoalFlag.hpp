#pragma once
#include <SFML/Graphics.hpp>

class Player;

class GoalFlag {
public:
    // Rozmiar hitboxa flagi
    static constexpr float WIDTH  = 32.f;
    static constexpr float HEIGHT = 64.f;

    // Konstruktor — podaj pozycje startowa flagi
    explicit GoalFlag(sf::Vector2f position);

    void draw(sf::RenderWindow& window) const;

    // Zwraca true jesli gracz dotknął flagi
    bool checkCollisionWithPlayer(const Player& player) const;

    sf::FloatRect getBounds() const;

private:
    sf::RectangleShape m_pole;   // maszt flagi
    sf::RectangleShape m_flag;   // trojkat / prostokat flagi
};
