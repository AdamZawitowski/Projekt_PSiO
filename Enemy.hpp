#pragma once
#include <SFML/Graphics.hpp>
#include <optional>
#include "TileMap.hpp"

// Zapowiedz klasy Player — zeby nie includowac calego headera
class Player;

class Enemy {
public:
    static constexpr float MOVE_SPEED = 120.f;   // px/s
    static constexpr float GRAVITY    = 800.f;  // px/s^2
    static constexpr float MAX_FALL   = 500.f;  // terminal velocity
    static constexpr int MAX_HEALTH = 2;

    // Rozmiar hitboxa — niezalezny od rozmiaru tekstury
    static constexpr float HITBOX_W = 40.f;
    static constexpr float HITBOX_H = 64.f;

    // Konstruktor — podaj pozycje startowa wroga
    explicit Enemy(sf::Vector2f startPosition);

    // Move constructor — niezbedny bo sf::Sprite trzyma wskaznik do sf::Texture.
    // Gdy std::vector realokuje pamiec i przenosi Enemy, sprite musi dostac
    // nowy adres tekstury (ktora tez zostala przesunieta razem z obiektem).
    Enemy(Enemy&& other) noexcept;
    Enemy& operator=(Enemy&& other) noexcept;

    // Kopiowanie zabronione (tekstura nie jest kopiowalna w SFML 3)
    Enemy(const Enemy&)            = delete;
    Enemy& operator=(const Enemy&) = delete;

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
    bool hit();

    sf::FloatRect getBounds() const;

private:
    int m_health = MAX_HEALTH;
    float m_shakeTimer = 0.f;
    float m_flashTimer = 0.f;
    void applyGravity(float dt);
    void patrol(float dt, const TileMap& tileMap);
    void resolveCollisions(const TileMap& tileMap);
    void applySpriteToHitbox();          // synchronizuje pozycje sprajta z hitboxem

    // Hitbox — podstawa kolizji, niewidoczny
    sf::RectangleShape    m_shape;
    sf::Vector2f          m_velocity;

    bool m_alive;
    bool m_movingRight; // kierunek patrolowania

    // Dane wizualne — tekstura musi zyc dluzej niz sprite (SFML 3)
    sf::Texture             m_texture;
    std::optional<sf::Sprite> m_sprite; // SFML 3: Sprite wymaga tekstury przy konstrukcji
    bool                    m_textureLoaded;
};