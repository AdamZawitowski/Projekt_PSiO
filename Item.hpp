#pragma once
#include <SFML/Graphics.hpp>
#include <array>
#include <optional>

enum class ItemType {
    Coin,
    Mushroom,
    Heart
};

class Item {
public:
    Item(sf::Vector2f pos, ItemType type);

    void update(float dt);
    void draw(sf::RenderWindow& window) const;

    sf::FloatRect getBounds() const;
    ItemType getType() const { return m_type; }
    bool isCollected() const { return m_collected; }
    void collect() { m_collected = true; }

    void applyGravity(float dt);
    void stopFalling();
    bool isFalling() const { return m_velocity.y > 0.f; }

    int m_sourceRow = -1;
    int m_sourceCol = -1;

    float m_ignoreCollisionTime = 0.55f;

    void setSourceBlock(int col, int row) {
        m_sourceCol = col;
        m_sourceRow = row;
    }

    void snapToGround(float groundY);

private:
    // Rozmiar logicznego hitboxa (taki sam jak dawny kolorowy kwadracik,
    // żeby kolizje w main.cpp / TileMap działały bez zmian).
    static constexpr float HITBOX_SIZE = 20.f;

    // Liczba klatek animacji monety oraz czas trwania jednej klatki.
    static constexpr int   COIN_FRAME_COUNT = 5;
    static constexpr float COIN_FRAME_TIME  = 0.15f;

    void loadTextures();
    void applyCurrentTexture();

    // Pozycja logiczna (lewy-górny rog hitboxa) — niezależna od sprite'a,
    // dzięki temu getBounds()/snapToGround() są proste i przewidywalne.
    sf::Vector2f m_position;
    sf::Vector2f m_velocity;
    ItemType     m_type;
    bool         m_collected = false;

    std::optional<sf::Sprite> m_sprite;

    // --- Tekstury ---
    std::array<sf::Texture, COIN_FRAME_COUNT> m_coinTextures;
    sf::Texture m_mushroomTexture;
    sf::Texture m_heartTexture;
    bool        m_texturesLoaded = false;

    // --- Animacja monety ---
    float m_animationTimer = 0.f;
    int   m_currentFrame = 0;
};