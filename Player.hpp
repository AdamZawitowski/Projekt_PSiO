#pragma once
#include <SFML/Graphics.hpp>
#include "TileMap.hpp"

class Player {
public:
    static constexpr float MOVE_SPEED = 160.f;
    static constexpr float SPRINT_SPEED = 260.f;
    static constexpr float JUMP_FORCE = -520.f;
    static constexpr float GRAVITY = 800.f;
    static constexpr float MAX_FALL = 500.f;

    Player();

    void update(float dt);
    void draw(sf::RenderWindow& window) const;

    void resolveCollisions(const TileMap& tileMap);

    sf::FloatRect getBounds() const;
    sf::Vector2f getPosition() const;
    int getLives() const;
    int getScore() const;

    void addScore(int points);
    void loseLife();
    void respawn();

private:
    void handleInput(float dt);
    void applyGravity(float dt);
    void applyMovement(float dt);

    sf::RectangleShape m_shape;
    sf::Vector2f m_velocity;
    sf::Vector2f m_spawnPoint;

    bool m_onGround;
    int m_lives;
    int m_score;
};
