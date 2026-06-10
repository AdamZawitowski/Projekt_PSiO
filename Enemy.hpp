#pragma once
#include <SFML/Graphics.hpp>
#include "TileMap.hpp"

// Zapowiedz klasy Player — zeby nie includowac calego headera
class Player;

class Enemy {
public:
    static constexpr float MOVE_SPEED = 80.f;   // px/s
    static constexpr float GRAVITY    = 800.f;  // px/s^2
    static constexpr float MAX_FALL   = 500.f;  // terminal velocity

    // Konstruktor — podaj pozycje startowa wroga
    explicit Enemy(sf::Vector2f startPosition);

    // Glowne metody
    void update(float dt, const TileMap& tileMap);
    void draw(sf::RenderWindow& window) const;

    // Kolizja z graczem:
    // - zwraca true jesli gracz depcze wroga od gory (wrog ginie)
    // - zwraca false jesli kolizja z boku (gracz traci zycie)
    // Wywolaj w main.cpp i na podstawie wyniku:
    //   true  -> enemy.kill(), player.addScore(100)
    //   false -> player.loseLife()
    bool checkCollisionWithPlayer(const Player& player) const;

    bool isAlive()  const;
    void kill();

    sf::FloatRect getBounds() const;

private:
    void applyGravity(float dt);
    void patrol(float dt, const TileMap& tileMap);
    void resolveCollisions(const TileMap& tileMap);

    sf::RectangleShape m_shape;
    sf::Vector2f       m_velocity;

    bool m_alive;
    bool m_movingRight; // kierunek patrolowania
};
