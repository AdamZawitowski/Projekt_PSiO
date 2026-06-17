#include "Item.hpp"
#include <iostream>

Item::Item(sf::Vector2f pos, ItemType type)
    : m_position(pos), m_velocity(0.f, -150.f), m_type(type)
{
    loadTextures();
    applyCurrentTexture();
}

void Item::loadTextures() {
    bool ok = true;

    if (m_type == ItemType::Coin) {
        static const char* paths[COIN_FRAME_COUNT] = {
            "assets/coin_1.png",
            "assets/coin_2.png",
            "assets/coin_3.png",
            "assets/coin_4.png",
            "assets/coin_5.png"
        };
        for (int i = 0; i < COIN_FRAME_COUNT; ++i) {
            if (!m_coinTextures[i].loadFromFile(paths[i])) {
                std::cerr << "[Item] brak: " << paths[i] << "\n";
                ok = false;
            }
        }
    }
    else if (m_type == ItemType::Mushroom) {
        if (!m_mushroomTexture.loadFromFile("assets/mushroom_1.png")) {
            std::cerr << "[Item] brak: assets/mushroom_1.png\n";
            ok = false;
        }
    }
    else if (m_type == ItemType::Heart) {
        if (!m_heartTexture.loadFromFile("assets/heart.png")) {
            std::cerr << "[Item] brak: assets/heart.png\n";
            ok = false;
        }
    }

    m_texturesLoaded = ok;
}

// Ustawia sprajt na aktualną teksturę (zależnie od typu / klatki animacji)
// i dopasowuje skalę + origin tak, by sprite idealnie wypełniał logiczny
// hitbox HITBOX_SIZE x HITBOX_SIZE, z origin w lewym górnym rogu —
// dzięki temu setPosition(m_position) odpowiada zachowaniu starego
// sf::RectangleShape.
void Item::applyCurrentTexture() {
    if (!m_texturesLoaded) return;

    const sf::Texture* tex = nullptr;

    switch (m_type) {
    case ItemType::Coin:
        tex = &m_coinTextures[m_currentFrame];
        break;
    case ItemType::Mushroom:
        tex = &m_mushroomTexture;
        break;
    case ItemType::Heart:
        tex = &m_heartTexture;
        break;
    }

    if (!tex) return;

    if (!m_sprite)
        m_sprite.emplace(*tex);
    else
        m_sprite->setTexture(*tex, true);

    m_sprite->setOrigin({ 0.f, 0.f });

    const sf::Vector2u texSize = tex->getSize();
    if (texSize.x > 0 && texSize.y > 0) {
        m_sprite->setScale({
            HITBOX_SIZE / static_cast<float>(texSize.x),
            HITBOX_SIZE / static_cast<float>(texSize.y)
        });
    }

    m_sprite->setPosition(m_position);
}

void Item::applyGravity(float dt) {
    m_velocity.y += 600.f * dt;
    if (m_velocity.y > 400.f)
        m_velocity.y = 400.f;
}

void Item::stopFalling() {
    m_velocity.y = 0.f;
}

void Item::update(float dt) {
    if (m_ignoreCollisionTime > 0.f) {
        m_ignoreCollisionTime -= dt;
    }

    applyGravity(dt);
    m_position += m_velocity * dt;

    // --- Animacja monety ---
    if (m_type == ItemType::Coin && m_texturesLoaded) {
        m_animationTimer += dt;
        if (m_animationTimer >= COIN_FRAME_TIME) {
            m_animationTimer -= COIN_FRAME_TIME;
            m_currentFrame = (m_currentFrame + 1) % COIN_FRAME_COUNT;
        }
    }

    if (m_texturesLoaded && m_sprite) {
        if (m_type == ItemType::Coin)
            m_sprite->setTexture(m_coinTextures[m_currentFrame], true);
        m_sprite->setPosition(m_position);
    }
}

void Item::draw(sf::RenderWindow& window) const {
    if (m_collected) return;

    if (m_texturesLoaded && m_sprite)
        window.draw(*m_sprite);
}

sf::FloatRect Item::getBounds() const {
    return sf::FloatRect(m_position, { HITBOX_SIZE, HITBOX_SIZE });
}

void Item::snapToGround(float groundY) {
    m_position.y = groundY - HITBOX_SIZE;
    m_velocity.y = 0.f;

    if (m_texturesLoaded && m_sprite)
        m_sprite->setPosition(m_position);
}