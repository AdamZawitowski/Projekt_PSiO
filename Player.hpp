#pragma once
#include <SFML/Graphics.hpp>
#include <optional>
#include "TileMap.hpp"

class Player {
public:
    // --- Stale ---
    static constexpr float MOVE_SPEED   = 160.f;
    static constexpr float SPRINT_SPEED = 260.f;
    static constexpr float JUMP_FORCE   = -520.f;
    static constexpr float GRAVITY      = 800.f;
    static constexpr float MAX_FALL     = 500.f;
    static constexpr float DEATH_DELAY  = 1.5f;  // czas trwania animacji smierci

    static constexpr float HITBOX_W = 26.f;
    static constexpr float HITBOX_H = 32.f;
    static constexpr float HITBOX_CROUCH_H = 18.f; 
    Player();

    void update(float dt);
    void draw(sf::RenderWindow& window) const;
    void resolveCollisions(const TileMap& tileMap);

    sf::FloatRect getBounds()   const;
    sf::Vector2f  getPosition() const;
    int           getLives()    const;
    int           getScore()    const;

    void addScore(int points);
    void loseLife();
    void respawn();

    // --- Gettery stanu dla main.cpp ---
    bool isDying()        const { return m_isDying; }
    // Zwraca true przez pierwsza klatke smierci — main.cpp uzywa do screen shake
    bool justDied()       const { return m_justDied; }
    void clearJustDied()        { m_justDied = false; }

    void setSpawnPoint(sf::Vector2f pos) { m_spawnPoint = pos; }

    void bounce() {
        m_velocity.y = JUMP_FORCE * 0.7f;}

    bool isInvincible() const { return m_invincible; }

    sf::Vector2f getVelocity() const { return m_velocity; }

private:
    // --- Smierc ---
    float m_deathTimer = 0.f;
    bool  m_isDying    = false;
    bool  m_justDied   = false;   // impuls dla screen shake — true przez 1 klatke
    bool  m_isCrouching = false;

    // --- Stany animacji ---
    enum class PlayerState {
        Idle, RunRight, RunLeft, Jump, Crawl, Dead
    };

    void handleInput(float dt);
    void applyGravity(float dt);
    void applyMovement(float dt);
    void updateAnimation(float dt);
    void setState(PlayerState newState);
    void applySpriteToHitbox();

    bool loadTextures();
    void setTextureWithOrigin(sf::Texture& tex, const sf::Vector2f& origin);

    // --- Fizyka ---
    sf::RectangleShape m_shape;
    sf::Vector2f       m_velocity;
    sf::Vector2f       m_spawnPoint;
    bool               m_onGround;
    int                m_lives;
    int                m_score;

    // --- Wizualia ---
    sf::Texture  m_texIdle, m_texRunR1, m_texRunR2;
    sf::Texture  m_texJump, m_texCrawl, m_texDeath;

    sf::Vector2f m_originIdle,  m_originRunR1, m_originRunR2;
    sf::Vector2f m_originJump,  m_originCrawl, m_originDeath;

    std::optional<sf::Sprite> m_sprite;

    PlayerState m_state;
    float       m_animTimer;
    bool        m_runFrame;
    bool        m_texturesLoaded;

    float m_invincibleTimer = 0.f;
    bool  m_invincible = false;

};