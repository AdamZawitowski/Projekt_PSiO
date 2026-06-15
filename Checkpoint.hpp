#pragma once
#include <SFML/Graphics.hpp>
#include <iostream>
#include <optional>
#include "Player.hpp"

class Checkpoint {
public:
    static constexpr float WIDTH  = 32.f;
    static constexpr float HEIGHT = 32.f;

    explicit Checkpoint(sf::Vector2f pos)
        : m_position(pos)
        , m_activated(false)
    {
        if (!m_texInactive.loadFromFile("assets/yellow_checkpoint_flag.png"))
            std::cerr << "[Checkpoint] brak: assets/yellow_checkpoint_flag.png\n";

        if (!m_texActive.loadFromFile("assets/green_checkpoint_flag.png"))
            std::cerr << "[Checkpoint] brak: assets/green_checkpoint_flag.png\n";

        m_sprite.emplace(m_texInactive);
        m_sprite->setPosition(pos);

        m_hitbox.setSize({ WIDTH, HEIGHT });
        m_hitbox.setFillColor(sf::Color::Transparent);
        m_hitbox.setPosition(pos);
    }

    // ----------------------------------------------------------------
    //  Move constructor — niezbedny gdy std::vector realokuje pamiec.
    //  sf::Sprite trzyma wskaznik do sf::Texture — po przeniesieniu
    //  obiektu tekstury sa pod nowym adresem, wiec sprite musi zostac
    //  odtworzony aby wskazywac na wlasne (juz przesuniete) tekstury.
    // ----------------------------------------------------------------
    Checkpoint(Checkpoint&& other) noexcept
        : m_position    (other.m_position)
        , m_activated   (other.m_activated)
        , m_texInactive (std::move(other.m_texInactive))
        , m_texActive   (std::move(other.m_texActive))
        , m_hitbox      (std::move(other.m_hitbox))
    {
        // Odtworz sprite wskazujac na NOWY adres odpowiedniej tekstury
        const sf::Texture& tex = m_activated ? m_texActive : m_texInactive;
        m_sprite.emplace(tex);
        m_sprite->setPosition(m_position);
    }

    Checkpoint& operator=(Checkpoint&& other) noexcept {
        if (this == &other) return *this;
        m_position    = other.m_position;
        m_activated   = other.m_activated;
        m_texInactive = std::move(other.m_texInactive);
        m_texActive   = std::move(other.m_texActive);
        m_hitbox      = std::move(other.m_hitbox);
        m_sprite.reset();
        const sf::Texture& tex = m_activated ? m_texActive : m_texInactive;
        m_sprite.emplace(tex);
        m_sprite->setPosition(m_position);
        return *this;
    }

    // Kopiowanie zabronione — sf::Texture nie jest kopiowalna w SFML 3
    Checkpoint(const Checkpoint&)            = delete;
    Checkpoint& operator=(const Checkpoint&) = delete;

    // ----------------------------------------------------------------

    void draw(sf::RenderWindow& window) const {
        if (m_sprite) window.draw(*m_sprite);
    }

    bool checkCollision(const Player& player) {
        if (m_activated) return false;

        if (!m_hitbox.getGlobalBounds().findIntersection(player.getBounds()))
            return false;

        m_activated = true;
        if (m_sprite)
            m_sprite->setTexture(m_texActive);

        return true;
    }

    void reset() {
        m_activated = false;
        if (m_sprite)
            m_sprite->setTexture(m_texInactive);
    }

    bool         isActivated() const { return m_activated; }
    sf::Vector2f getPosition() const { return m_position;  }

private:
    sf::Vector2f m_position;
    bool         m_activated;

    sf::Texture               m_texInactive;
    sf::Texture               m_texActive;
    std::optional<sf::Sprite> m_sprite;
    sf::RectangleShape        m_hitbox;
};