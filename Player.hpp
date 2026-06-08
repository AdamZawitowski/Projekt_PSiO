#pragma once
#include <SFML/Graphics.hpp>
#include "TileMap.hpp"

class Player {
public:
    // --- Stale ---
    static constexpr float MOVE_SPEED = 160.f;
    static constexpr float SPRINT_SPEED = 260.f;
    static constexpr float JUMP_FORCE = -520.f;
    static constexpr float GRAVITY = 800.f;
    static constexpr float MAX_FALL = 500.f;
    // parametry postaci też do ustalenia
    

    // --- Konstruktor ---
    Player();

    // --- Glowne metody ---
    void update(float dt);
    void draw(sf::RenderWindow& window) const;

    // --- Kolizje z tilemap (wywolaj po update) ---
    void resolveCollisions(const TileMap& tileMap);

    // --- Gettery ---
    sf::FloatRect getBounds()   const;
    sf::Vector2f  getPosition() const;
    int           getLives()    const;
    int           getScore()    const;

    // --- Gameplay ---
    void addScore(int points);
    void loseLife();
    void respawn();

private:
    // --- Pomocnicze metody ---
    void handleInput(float dt);
    void applyGravity(float dt);
    void applyMovement(float dt);

    // --- Dane ---
    sf::RectangleShape m_shape;
    sf::Vector2f       m_velocity;
    sf::Vector2f       m_spawnPoint;

    bool m_onGround;
    int  m_lives;
    int  m_score;
};
