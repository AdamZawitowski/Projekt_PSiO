#include "Player.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

// ------------------------------------------------------------------ //
//  Pomocnicza: wyznacz origin na podstawie nieprzezroczystych pikseli  //
// ------------------------------------------------------------------ //
namespace {
sf::Vector2f computeVisibleOrigin(const std::string& path) {
    sf::Image image;
    if (!image.loadFromFile(path))
        return {0.f, 0.f};

    const sf::Vector2u size = image.getSize();
    unsigned minX = size.x, minY = size.y, maxX = 0, maxY = 0;
    bool found = false;

    for (unsigned y = 0; y < size.y; ++y)
        for (unsigned x = 0; x < size.x; ++x)
            if (image.getPixel({x, y}).a > 0) {
                found = true;
                minX = std::min(minX, x); maxX = std::max(maxX, x);
                minY = std::min(minY, y); maxY = std::max(maxY, y);
            }

    if (!found)
        return {size.x / 2.f, static_cast<float>(size.y)};

    return { (minX + maxX + 1) / 2.f, static_cast<float>(maxY + 1) };
}
} // namespace

static constexpr float RUN_FRAME_TIME    = 0.15f;
static constexpr float VELOCITY_EPSILON  = 0.1f;
static constexpr float GROUND_EPSILON    = 0.5f;

// ------------------------------------------------------------------ //
//  Konstruktor                                                          //
// ------------------------------------------------------------------ //
Player::Player()
    : m_velocity(0.f, 0.f)
    , m_spawnPoint(80.f, 70.f)
    , m_onGround(false)
    , m_lives(3)
    , m_score(0)
    , m_state(PlayerState::Idle)
    , m_animTimer(0.f)
    , m_runFrame(false)
    , m_texturesLoaded(false)
{
    m_shape.setSize({ HITBOX_W, HITBOX_H });
    m_shape.setFillColor(sf::Color::Transparent);
    m_shape.setPosition(m_spawnPoint);

    m_texturesLoaded = loadTextures();
    if (m_texturesLoaded)
        setState(PlayerState::Idle);
    else
        m_shape.setFillColor(sf::Color::Red);
}

// ------------------------------------------------------------------ //
//  Ladowanie tekstur                                                    //
// ------------------------------------------------------------------ //
bool Player::loadTextures() {
    bool ok = true;
    auto load = [&](sf::Texture& tex, sf::Vector2f& origin, const char* path) {
        if (!tex.loadFromFile(path)) {
            std::cerr << "[Player] brak: " << path << "\n";
            ok = false;
        } else {
            origin = computeVisibleOrigin(path);
        }
    };

    load(m_texIdle,  m_originIdle,  "assets/static.png");
    load(m_texRunR1, m_originRunR1, "assets/right_run_1.png");
    load(m_texRunR2, m_originRunR2, "assets/right_run_2.png");
    load(m_texJump,  m_originJump,  "assets/jump.png");
    load(m_texCrawl, m_originCrawl, "assets/crawl.png");
    m_originCrawl.y = m_texCrawl.getSize().y;
    m_originCrawl.y -= 27.f;
    load(m_texDeath, m_originDeath, "assets/death.png");

    return ok;
}

// ------------------------------------------------------------------ //
//  Ustawienie tekstury + origin                                         //
// ------------------------------------------------------------------ //
void Player::setTextureWithOrigin(sf::Texture& tex, const sf::Vector2f& origin) {
    m_sprite.emplace(tex);
    m_sprite->setOrigin(origin);
}

// ------------------------------------------------------------------ //
//  Zmiana stanu animacji                                                //
// ------------------------------------------------------------------ //
void Player::setState(PlayerState newState) {
    if (newState == m_state) return;
    m_state = newState;

    switch (m_state) {
    case PlayerState::Idle:
        setTextureWithOrigin(m_texIdle, m_originIdle);
        m_sprite->setScale({ 1.f, 1.f });
        break;
    case PlayerState::RunRight:
        setTextureWithOrigin(m_runFrame ? m_texRunR2 : m_texRunR1,
                             m_runFrame ? m_originRunR2 : m_originRunR1);
        m_sprite->setScale({ -1.f, 1.f });
        break;
    case PlayerState::RunLeft:
        setTextureWithOrigin(m_runFrame ? m_texRunR2 : m_texRunR1,
                             m_runFrame ? m_originRunR2 : m_originRunR1);
        m_sprite->setScale({ 1.f, 1.f });
        break;
    case PlayerState::Jump:
        setTextureWithOrigin(m_texJump, m_originJump);
        break;
    case PlayerState::Crawl:
        setTextureWithOrigin(m_texCrawl, m_originCrawl);
        m_sprite->setScale({ 1.f, 1.f });
        break;
    case PlayerState::Dead:
        setTextureWithOrigin(m_texDeath, m_originDeath);
        m_sprite->setScale({ 1.f, 1.f });
        break;
    }
    applySpriteToHitbox();
}

// ------------------------------------------------------------------ //
//  Synchronizacja sprajta z hitboxem                                   //
// ------------------------------------------------------------------ //
void Player::applySpriteToHitbox() {
    if (!m_sprite) return;
    sf::FloatRect b = m_shape.getGlobalBounds();
    m_sprite->setPosition({
        b.position.x + b.size.x / 2.f,
        b.position.y + b.size.y
    });
}

// ------------------------------------------------------------------ //
//  Animacja                                                             //
// ------------------------------------------------------------------ //
void Player::updateAnimation(float dt) {
    if (m_invincible) {
        float t = std::fmod(m_invincibleTimer, 0.2f);
        bool visible = t < 0.1f;
        if (m_sprite)
            m_sprite->setColor(visible ? sf::Color(255, 255, 255, 255)
                : sf::Color(255, 255, 255, 80));
    }
    else {
        if (m_sprite)
            m_sprite->setColor(sf::Color(255, 255, 255, 255));
    }
    if (!m_texturesLoaded) return;
    if (m_state == PlayerState::Dead) { applySpriteToHitbox(); return; }
    if (m_isCrouching) { applySpriteToHitbox(); return; }

    if (m_isCrouching) {
        setState(PlayerState::Crawl);
        applySpriteToHitbox();
        return;
    }

    PlayerState target;
    const bool movingX = std::abs(m_velocity.x) >= VELOCITY_EPSILON;

    if (!m_onGround)              target = PlayerState::Jump;
    else if (movingX && m_velocity.x > 0.f) target = PlayerState::RunRight;
    else if (movingX && m_velocity.x < 0.f) target = PlayerState::RunLeft;
    else                          target = PlayerState::Idle;

    if (target == PlayerState::RunRight || target == PlayerState::RunLeft) {
        m_animTimer += dt;
        if (m_animTimer >= RUN_FRAME_TIME) {
            m_animTimer = 0.f;
            m_runFrame  = !m_runFrame;
            PlayerState prev = m_state;
            m_state = PlayerState::Idle;   // trick: wymusz odswiezenie klatki
            setState(prev);
        }
    } else {
        m_animTimer = 0.f;
        m_runFrame  = false;
    }

    if (target != m_state) setState(target);
    else                   applySpriteToHitbox();
}

// ------------------------------------------------------------------ //
//  Update                                                               //
// ------------------------------------------------------------------ //
void Player::update(float dt) {
    m_justDied = false;  // zeruj impuls co klatke

    // ============================================================
    // TRYB SMIERCI — brak inputu, brak kolizji, swobodny lot
    // ============================================================
    if (m_isDying) {
        m_deathTimer -= dt;

        // Grawitacja dziala — postac leci w gore, potem spada z ekranu
        m_velocity.y += GRAVITY * dt;
        m_shape.move(m_velocity * dt);
        if (m_texturesLoaded) applySpriteToHitbox();

        if (m_deathTimer <= 0.f) {
            m_isDying = false;
            if (m_lives > 0) respawn();
            // jesli lives == 0 — main.cpp przelacza na GameOver
        }
        return;
    }

    // ============================================================
    // TRYB NORMALNY
    // ============================================================

    if (m_invincible) {
        m_invincibleTimer -= dt;
        if (m_invincibleTimer <= 0.f) {
            m_invincible = false;
        }
    }

    m_velocity.x = 0.f;
    applyGravity(dt);
    handleInput(dt);
    applyMovement(dt);
    updateAnimation(dt);
}

// ------------------------------------------------------------------ //
//  Grawitacja                                                           //
// ------------------------------------------------------------------ //
void Player::applyGravity(float dt) {
    if (m_onGround) return;
    m_velocity.y += GRAVITY * dt;
    if (m_velocity.y > MAX_FALL) m_velocity.y = MAX_FALL;
}

// ------------------------------------------------------------------ //
//  Input                                                                //
// ------------------------------------------------------------------ //
void Player::handleInput(float dt) {
    const float speed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)
                        ? SPRINT_SPEED : MOVE_SPEED;

    int horizontal = 0;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)  ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))     --horizontal;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))     ++horizontal;
    m_velocity.x = horizontal * speed;

    const bool wantCrouch =
        (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S));

    if (wantCrouch && !m_isCrouching) {
        m_isCrouching = true;
        const float diff = HITBOX_H - HITBOX_CROUCH_H;

        // ZMNIEJSZ HITBOX OD GÓRY, NIE OD DOŁU
        m_shape.setSize({ HITBOX_W, HITBOX_CROUCH_H });

        // NIE ruszaj pozycji Y — stopy zostają tam gdzie były

        if (m_texturesLoaded) setState(PlayerState::Crawl);

    }
    else if (!wantCrouch && m_isCrouching) {
        m_isCrouching = false;
        m_shape.setSize({ HITBOX_W, HITBOX_H });
        // NIE ruszaj pozycji Y

    }

    if (!m_isCrouching && 
        (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) ||
         sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)    ||
         sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
        && m_onGround) {
        m_velocity.y = JUMP_FORCE;
        m_onGround   = false;
    }
}

// ------------------------------------------------------------------ //
//  Ruch                                                                 //
// ------------------------------------------------------------------ //
void Player::applyMovement(float dt) {
    m_shape.move(m_velocity * dt);
}

// ------------------------------------------------------------------ //
//  Kolizje z tilemap — pomijane podczas animacji smierci               //
// ------------------------------------------------------------------ //
void Player::resolveCollisions(const TileMap& tileMap) {
    // Podczas smierci postac leci swobodnie — brak kolizji
    if (m_isDying) return;

    const float TS      = TileMap::TILE_SIZE;
    bool landedOnGround = false;
    m_onGround          = false;

    for (int iter = 0; iter < 8; ++iter) {
        sf::FloatRect b = m_shape.getGlobalBounds();

        const int leftCol   = static_cast<int>(std::floor(b.position.x / TS));
        const int rightCol  = static_cast<int>(std::floor((b.position.x + b.size.x - 0.001f) / TS));
        const int topRow    = static_cast<int>(std::floor(b.position.y / TS));
        const int bottomRow = static_cast<int>(std::floor((b.position.y + b.size.y - 0.001f) / TS));

        bool resolved = false;
        for (int row = topRow; row <= bottomRow && !resolved; ++row) {
            for (int col = leftCol; col <= rightCol && !resolved; ++col) {
                if (!tileMap.isSolid(col, row)) continue;

                sf::FloatRect tileBounds({ col * TS, row * TS }, { TS, TS });
                auto intersection = b.findIntersection(tileBounds);
                if (!intersection) continue;

                const float pcx = b.position.x + b.size.x / 2.f;
                const float tcx = tileBounds.position.x + tileBounds.size.x / 2.f;
                const float pcy = b.position.y + b.size.y / 2.f;
                const float tcy = tileBounds.position.y + tileBounds.size.y / 2.f;

                if (intersection->size.x < intersection->size.y) {
                    m_shape.move({ pcx < tcx ? -intersection->size.x
                                              :  intersection->size.x, 0.f });
                    m_velocity.x = 0.f;
                } else {
                    if (pcy < tcy) {
                        m_shape.setPosition({ b.position.x,
                                              tileBounds.position.y - b.size.y });
                        m_velocity.y   = 0.f;
                        landedOnGround = true;
                    } else {
                        m_shape.move({ 0.f, intersection->size.y });
                        m_velocity.y = 0.f;
                    }
                }
                resolved = true;
            }
        }
        if (!resolved) break;
    }

    sf::FloatRect b = m_shape.getGlobalBounds();
    const float footY = b.position.y + b.size.y + GROUND_EPSILON;
    const bool groundSupport =
        tileMap.isSolidAtPixel({ b.position.x + 2.f,            footY }) ||
        tileMap.isSolidAtPixel({ b.position.x + b.size.x - 2.f, footY });

    m_onGround = landedOnGround || groundSupport;
    if (m_onGround && std::abs(m_velocity.y) < VELOCITY_EPSILON)
        m_velocity.y = 0.f;

    // Wypadniecie z mapy
    if (m_shape.getPosition().y > tileMap.getSizeInPixels().y)
        loseLife();

    if (m_texturesLoaded) applySpriteToHitbox();
}

// ------------------------------------------------------------------ //
//  Rysowanie                                                            //
// ------------------------------------------------------------------ //
void Player::draw(sf::RenderWindow& window) const {
    if (m_sprite) window.draw(*m_sprite);
    else          window.draw(m_shape);
}

// ------------------------------------------------------------------ //
//  Gettery                                                              //
// ------------------------------------------------------------------ //
sf::FloatRect Player::getBounds()   const { return m_shape.getGlobalBounds(); }
sf::Vector2f  Player::getPosition() const { return m_shape.getPosition(); }
int           Player::getLives()    const { return m_lives; }
int           Player::getScore()    const { return m_score; }

// ------------------------------------------------------------------ //
//  Gameplay                                                             //
// ------------------------------------------------------------------ //
void Player::addScore(int points) { m_score += points; }

void Player::loseLife() {
    if (m_isDying || m_lives <= 0) return;

    m_isDying    = true;
    m_justDied   = true;          // impuls dla screen shake w main.cpp
    m_deathTimer = DEATH_DELAY;
    m_lives--;

    // Death bounce — ostry impuls w gore, grawitacja potem sciagnie w dol
    m_velocity = { 0.f, -600.f };

    if (m_texturesLoaded) setState(PlayerState::Dead);
}

void Player::respawn() {
    m_shape.setPosition(m_spawnPoint);
    m_velocity  = { 0.f, 0.f };
    m_onGround  = false;
    m_isDying   = false;
    m_justDied  = false;

    if (m_isCrouching) {
        m_isCrouching = false;
        m_shape.setSize({ HITBOX_W, HITBOX_H });
    }

    if (m_texturesLoaded) {
        m_state = PlayerState::Idle;
        setState(PlayerState::Idle);
    }

    m_invincible = true;
    m_invincibleTimer = 1.5f;

}