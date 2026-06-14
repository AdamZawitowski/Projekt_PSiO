#include "Player.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>

namespace {

sf::Vector2f computeVisibleOrigin(const std::string& path) {
    sf::Image image;
    if (!image.loadFromFile(path))
        return {0.f, 0.f};

    const sf::Vector2u size = image.getSize();
    unsigned minX = size.x;
    unsigned minY = size.y;
    unsigned maxX = 0;
    unsigned maxY = 0;
    bool found = false;

    for (unsigned y = 0; y < size.y; ++y) {
        for (unsigned x = 0; x < size.x; ++x) {
            if (image.getPixel({x, y}).a == 0)
                continue;

            found = true;
            minX = std::min(minX, x);
            minY = std::min(minY, y);
            maxX = std::max(maxX, x);
            maxY = std::max(maxY, y);
        }
    }

    if (!found)
        return {size.x / 2.f, static_cast<float>(size.y)};

    return {
        (minX + maxX + 1) / 2.f,
        static_cast<float>(maxY + 1)
    };
}

}

static constexpr float RUN_FRAME_TIME = 0.15f;
static constexpr float VELOCITY_EPSILON = 0.1f;
static constexpr float GROUND_CHECK_EPSILON = 0.5f;

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
    if (m_texturesLoaded) {
        setState(PlayerState::Idle);
    } else {
        m_shape.setFillColor(sf::Color::Red);
    }
}

// ------------------------------------------------------------------ //
//  Ladowanie tekstur — osobne wywolania, bez tablicy referencji        //
// ------------------------------------------------------------------ //
bool Player::loadTextures() {
    bool ok = true;

    if (!m_texIdle .loadFromFile("assets/static.png"))
        { std::cerr << "[Player] brak: assets/static.png\n";      ok = false; }
    if (!m_texRunR1.loadFromFile("assets/right_run_1.png"))
        { std::cerr << "[Player] brak: assets/right_run_1.png\n"; ok = false; }
    if (!m_texRunR2.loadFromFile("assets/right_run_2.png"))
        { std::cerr << "[Player] brak: assets/right_run_2.png\n"; ok = false; }
    if (!m_texJump .loadFromFile("assets/jump.png"))
        { std::cerr << "[Player] brak: assets/jump.png\n";        ok = false; }
    if (!m_texCrawl.loadFromFile("assets/crawl.png"))
        { std::cerr << "[Player] brak: assets/crawl.png\n";       ok = false; }
    if (!m_texDeath.loadFromFile("assets/death.png"))
        { std::cerr << "[Player] brak: assets/death.png\n";       ok = false; }

    m_originIdle  = computeVisibleOrigin("assets/static.png");
    m_originRunR1  = computeVisibleOrigin("assets/right_run_1.png");
    m_originRunR2  = computeVisibleOrigin("assets/right_run_2.png");
    m_originJump   = computeVisibleOrigin("assets/jump.png");
    m_originCrawl  = computeVisibleOrigin("assets/crawl.png");
    m_originDeath  = computeVisibleOrigin("assets/death.png");

    return ok;
}

// ------------------------------------------------------------------ //
//  Ustawienie tekstury + origin na dolny srodek                        //
// ------------------------------------------------------------------ //
void Player::setTextureWithOrigin(sf::Texture& tex, const sf::Vector2f& origin) {
    m_sprite.emplace(tex);
    m_sprite->setOrigin(origin);
}

// ------------------------------------------------------------------ //
//  Zmiana stanu                                                         //
// ------------------------------------------------------------------ //
void Player::setState(PlayerState newState) {
    if (newState == m_state)
        return;

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
//  Synchronizacja pozycji sprajta z hitboxem                           //
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
//  Aktualizacja animacji                                                //
// ------------------------------------------------------------------ //
void Player::updateAnimation(float dt) {
    if (!m_texturesLoaded) return;

    PlayerState target;
    const bool movingX = std::abs(m_velocity.x) >= VELOCITY_EPSILON;
    const bool movingY = std::abs(m_velocity.y) >= VELOCITY_EPSILON;

    if (!m_onGround && movingY)      target = PlayerState::Jump;
    else if (movingX && m_velocity.x > 0.f) target = PlayerState::RunRight;
    else if (movingX && m_velocity.x < 0.f) target = PlayerState::RunLeft;
    else                            target = PlayerState::Idle;

    if (m_state == PlayerState::Dead) {
        applySpriteToHitbox();
        return;
    }

    if (target == PlayerState::RunRight || target == PlayerState::RunLeft) {
        m_animTimer += dt;
        if (m_animTimer >= RUN_FRAME_TIME) {
            m_animTimer = 0.f;
            m_runFrame  = !m_runFrame;
            PlayerState previousState = m_state;
            m_state = PlayerState::Idle;
            setState(previousState);
        }
    } else {
        m_animTimer = 0.f;
        m_runFrame  = false;
    }

    if (target != m_state)
        setState(target);
    else
        applySpriteToHitbox();
}

// ------------------------------------------------------------------ //
//  Update                                                               //
// ------------------------------------------------------------------ //
void Player::update(float dt) {
    // Jesli gracz jest w trakcie animacji smierci — czekaj, potem respawnuj
    if (m_isDying) {
        m_deathTimer -= dt;
        if (m_deathTimer <= 0.f) {
            m_isDying = false;

            if (m_lives > 0) {
                respawn();
            }
        }

        applySpriteToHitbox();
        return; // blokuj input i grawitacje podczas animacji smierci
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
    if (m_velocity.y > MAX_FALL)
        m_velocity.y = MAX_FALL;
}

// ------------------------------------------------------------------ //
//  Input                                                                //
// ------------------------------------------------------------------ //
void Player::handleInput(float dt) {
    float speed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)
                  ? SPRINT_SPEED : MOVE_SPEED;

    int horizontal = 0;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
        --horizontal;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
        ++horizontal;

    m_velocity.x = horizontal * speed;

    if ((sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) ||
         sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
        && m_onGround && m_texturesLoaded)
    {
        setState(PlayerState::Crawl);
    }

    if ((sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) ||
         sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)    ||
         sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
        && m_onGround)
    {
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
//  Kolizje z tilemap                                                    //
// ------------------------------------------------------------------ //
void Player::resolveCollisions(const TileMap& tileMap) {
    const float TS = TileMap::TILE_SIZE;

    bool landedOnGround = false;
    bool resolved = true;
    m_onGround = false;

    for (int iteration = 0; resolved && iteration < 8; ++iteration) {
        resolved = false;
        sf::FloatRect b = m_shape.getGlobalBounds();

        const int leftCol   = static_cast<int>(std::floor(b.position.x / TS));
        const int rightCol  = static_cast<int>(std::floor((b.position.x + b.size.x - 0.001f) / TS));
        const int topRow    = static_cast<int>(std::floor(b.position.y / TS));
        const int bottomRow = static_cast<int>(std::floor((b.position.y + b.size.y - 0.001f) / TS));

        for (int row = topRow; row <= bottomRow && !resolved; ++row) {
            for (int col = leftCol; col <= rightCol; ++col) {
                if (!tileMap.isSolid(col, row))
                    continue;

                const sf::FloatRect tileBounds(
                    { col * TS, row * TS },
                    { TS, TS });

                const std::optional<sf::FloatRect> intersection = b.findIntersection(tileBounds);
                if (!intersection)
                    continue;

                const float playerCenterX = b.position.x + b.size.x / 2.f;
                const float tileCenterX   = tileBounds.position.x + tileBounds.size.x / 2.f;
                const float playerCenterY = b.position.y + b.size.y / 2.f;
                const float tileCenterY   = tileBounds.position.y + tileBounds.size.y / 2.f;

                if (intersection->size.x < intersection->size.y) {
                    if (playerCenterX < tileCenterX) {
                        m_shape.move({ -intersection->size.x, 0.f });
                    } else {
                        m_shape.move({ intersection->size.x, 0.f });
                    }
                    m_velocity.x = 0.f;
                } else {
                    if (playerCenterY < tileCenterY) {
                        m_shape.setPosition({
                            b.position.x,
                            tileBounds.position.y - b.size.y
                        });
                        m_velocity.y = 0.f;
                        landedOnGround = true;
                    } else {
                        m_shape.move({ 0.f, intersection->size.y });
                        m_velocity.y = 0.f;
                    }
                }

                resolved = true;
                break;
            }

            
        }
    }

    sf::FloatRect b = m_shape.getGlobalBounds();
    const float footY = b.position.y + b.size.y + GROUND_CHECK_EPSILON;
    const bool hasGroundSupport =
        tileMap.isSolidAtPixel({ b.position.x + 2.f, footY }) ||
        tileMap.isSolidAtPixel({ b.position.x + b.size.x - 2.f, footY });

    m_onGround = landedOnGround || hasGroundSupport;
    if (m_onGround && std::abs(m_velocity.y) < VELOCITY_EPSILON)
        m_velocity.y = 0.f;

    // Wypadniecie z mapy — traci zycie
    const sf::Vector2f mapSize = tileMap.getSizeInPixels();
    if (m_shape.getPosition().y > mapSize.y) {
        loseLife();
    }

    if (m_texturesLoaded)
        applySpriteToHitbox();

}

// ------------------------------------------------------------------ //
//  Rysowanie                                                            //
// ------------------------------------------------------------------ //
void Player::draw(sf::RenderWindow& window) const {
    if (m_sprite)
        window.draw(*m_sprite);
    else
        window.draw(m_shape);
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
    if (m_isDying) return;
    if (m_lives <= 0) return;

    m_isDying = true;
    m_deathTimer = DEATH_DELAY;
    m_velocity = { 0.f, 0.f };
    m_lives--;

    if (m_texturesLoaded) setState(PlayerState::Dead);
}


void Player::respawn() {
    m_shape.setPosition(m_spawnPoint);
    m_velocity = { 0.f, 0.f };
    m_onGround = false;
    m_isDying = false;
    if (m_texturesLoaded) {
        m_state = PlayerState::Idle; // wymus reset stanu
        setState(PlayerState::Idle);
    }
}
