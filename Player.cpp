#include "Player.hpp"

#include <cmath>

Player::Player()
    : m_velocity(0.f, 0.f)
    , m_spawnPoint(80.f, 70.f)
    , m_onGround(false)
    , m_lives(3)
    , m_score(0) {
    m_shape.setSize({26.f, 32.f});
    m_shape.setFillColor(sf::Color::Red);
    m_shape.setPosition(m_spawnPoint);
}

// ------------------------------------------------------------------ //
//  Update — kolejnosc ma znaczenie!                                     //
// ------------------------------------------------------------------ //
void Player::update(float dt) {
    applyGravity(dt);
    handleInput(dt);
    applyMovement(dt);
}

void Player::applyGravity(float dt) {
    if (m_onGround)
        return;

    m_velocity.y += GRAVITY * dt;
    if (m_velocity.y > MAX_FALL)
        m_velocity.y = MAX_FALL;
}

void Player::handleInput(float dt) {
    (void)dt;

    const float speed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)
        ? SPRINT_SPEED
        : MOVE_SPEED;

    m_velocity.x = 0.f;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)
        || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
        m_velocity.x = -speed;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)
        || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
        m_velocity.x = speed;

    if ((sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)
         || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)
         || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
        && m_onGround) {
        m_velocity.y = JUMP_FORCE;
        m_onGround = false;
    }
}

void Player::applyMovement(float dt) {
    m_shape.move(m_velocity * dt);
}

void Player::resolveCollisions(const TileMap& tileMap) {
    const float tileSize = TileMap::TILE_SIZE;

    sf::FloatRect bounds = m_shape.getGlobalBounds();

    if (m_velocity.y >= 0.f) {
        const bool bottomLeftSolid = tileMap.isSolidAtPixel(
            {bounds.position.x + 2.f, bounds.position.y + bounds.size.y});
        const bool bottomRightSolid = tileMap.isSolidAtPixel(
            {bounds.position.x + bounds.size.x - 2.f, bounds.position.y + bounds.size.y});

        if (bottomLeftSolid || bottomRightSolid) {
            const float tileRow = std::floor((bounds.position.y + bounds.size.y) / tileSize);
            m_shape.setPosition({bounds.position.x, tileRow * tileSize - bounds.size.y});
            m_velocity.y = 0.f;
            m_onGround = true;
        } else {
            m_onGround = false;
        }
    } else {
        m_onGround = false;
        const bool topLeftSolid = tileMap.isSolidAtPixel({bounds.position.x + 2.f, bounds.position.y});
        const bool topRightSolid = tileMap.isSolidAtPixel(
            {bounds.position.x + bounds.size.x - 2.f, bounds.position.y});

        if (topLeftSolid || topRightSolid) {
            const float tileRow = std::floor(bounds.position.y / tileSize);
            m_shape.setPosition({bounds.position.x, (tileRow + 1.f) * tileSize});
            m_velocity.y = 0.f;
        }
    }

    bounds = m_shape.getGlobalBounds();

    const float sampleY1 = bounds.position.y + bounds.size.y * 0.2f;
    const float sampleY2 = bounds.position.y + bounds.size.y * 0.8f;

    if (m_velocity.x < 0.f) {
        const bool leftBlocked = tileMap.isSolidAtPixel({bounds.position.x, sampleY1})
            || tileMap.isSolidAtPixel({bounds.position.x, sampleY2});

        if (leftBlocked) {
            const float tileCol = std::floor(bounds.position.x / tileSize);
            m_shape.setPosition({(tileCol + 1.f) * tileSize, bounds.position.y});
            m_velocity.x = 0.f;
        }
    }

    if (m_velocity.x > 0.f) {
        bounds = m_shape.getGlobalBounds();
        const bool rightBlocked = tileMap.isSolidAtPixel({bounds.position.x + bounds.size.x, sampleY1})
            || tileMap.isSolidAtPixel({bounds.position.x + bounds.size.x, sampleY2});

        if (rightBlocked) {
            const float tileCol = std::floor((bounds.position.x + bounds.size.x) / tileSize);
            m_shape.setPosition({tileCol * tileSize - bounds.size.x, bounds.position.y});
            m_velocity.x = 0.f;
        }
    }
}

// ------------------------------------------------------------------ //
//  Rysowanie                                                            //
// ------------------------------------------------------------------ //
void Player::draw(sf::RenderWindow& window) const {
    window.draw(m_shape);
}

sf::FloatRect Player::getBounds() const {
    return m_shape.getGlobalBounds();
}

sf::Vector2f Player::getPosition() const {
    return m_shape.getPosition();
}

int Player::getLives() const {
    return m_lives;
}

int Player::getScore() const {
    return m_score;
}

void Player::addScore(int points) {
    m_score += points;
}

void Player::loseLife() {
    m_lives--;
    if (m_lives > 0)
        respawn();
}

void Player::respawn() {
    m_shape.setPosition(m_spawnPoint);
    m_velocity = {0.f, 0.f};
    m_onGround = false;
}
