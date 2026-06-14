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

    // Rozmiar hitboxa — niezalezny od rozmiaru tekstury sprajta
    static constexpr float HITBOX_W = 26.f;
    static constexpr float HITBOX_H = 32.f;

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

    bool isDying() const { return m_isDying; }


private:

    // --- Smierc i respawn ---
    float m_deathTimer = 0.f;
    static constexpr float DEATH_DELAY = 1.2f; // czas animacji smierci w sekundach
    bool  m_isDying = false;

    // --- Stany animacji ---
    enum class PlayerState {
        Idle,
        RunRight,
        RunLeft,
        Jump,
        Crawl,
        Dead
    };

    // --- Pomocnicze metody ---
    void handleInput(float dt);
    void applyGravity(float dt);
    void applyMovement(float dt);
    void updateAnimation(float dt);
    void setState(PlayerState newState);
    void applySpriteToHitbox();          // synchronizuje pozycje sprajta z hitboxem

    // --- Ladowanie tekstur ---
    bool loadTextures();
    void setTextureWithOrigin(sf::Texture& tex, const sf::Vector2f& origin);

    // --- Dane fizyczne (hitbox) ---
    sf::RectangleShape m_shape;          // hitbox — uzywa kolizji, niewidoczny
    sf::Vector2f       m_velocity;
    sf::Vector2f       m_spawnPoint;
    bool               m_onGround;
    int                m_lives;
    int                m_score;

    // --- Dane wizualne ---
    sf::Texture  m_texIdle;
    sf::Texture  m_texRunR1;
    sf::Texture  m_texRunR2;
    sf::Texture  m_texJump;
    sf::Texture  m_texCrawl;
    sf::Texture  m_texDeath;

    sf::Vector2f m_originIdle;
    sf::Vector2f m_originRunR1;
    sf::Vector2f m_originRunR2;
    sf::Vector2f m_originJump;
    sf::Vector2f m_originCrawl;
    sf::Vector2f m_originDeath;

    std::optional<sf::Sprite> m_sprite;  // SFML 3: Sprite wymaga tekstury przy konstrukcji

    PlayerState  m_state;
    float        m_animTimer;            // czas do nastepnej klatki animacji biegu
    bool         m_runFrame;             // false = run_1, true = run_2
    bool         m_texturesLoaded;

};
