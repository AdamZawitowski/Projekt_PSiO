#include "Player.hpp"

// ------------------------------------------------------------------ //
//  Konstruktor                                                          //
// ------------------------------------------------------------------ //
Player::Player()
    : m_velocity(0.f, 0.f)
    , m_spawnPoint(80.f, 70.f)
    , m_onGround(false)
    , m_lives(3)
    , m_score(0)
{
    m_shape.setSize({ 26.f, 32.f }); //wielkość postaci jeszcze do ustalenia
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
//  Input gracza                                                         //
// ------------------------------------------------------------------ //
void Player::handleInput(float dt) {
    float speed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)
                  ? SPRINT_SPEED : MOVE_SPEED;

    // Ruch poziomy — zerujemy co klatke
    m_velocity.x = 0.f;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
        m_velocity.x = -speed;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
        m_velocity.x = speed;

    // Skok — tylko gdy stoi na ziemi
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
//  Ruch — przesuniecie pozycji na podstawie velocity                   //
// ------------------------------------------------------------------ //
void Player::applyMovement(float dt) {
    m_shape.move(m_velocity * dt);
}

// ------------------------------------------------------------------ //
//  Kolizje z tilemap                                           
// ------------------------------------------------------------------ //
void Player::resolveCollisions(const TileMap& tileMap) {
    const float TS = TileMap::TILE_SIZE;

    // ============================================================
    // kolizje PIONOWE
    // ============================================================
    sf::FloatRect b = m_shape.getGlobalBounds();

    if (m_velocity.y >= 0.f) {
        // Spadanie
        bool bl = tileMap.isSolidAtPixel({ b.position.x + 2.f,            b.position.y + b.size.y });
        bool br = tileMap.isSolidAtPixel({ b.position.x + b.size.x - 2.f, b.position.y + b.size.y });
        if (bl || br) {
            float tileRow = std::floor((b.position.y + b.size.y) / TS);
            m_shape.setPosition({ b.position.x, tileRow * TS - b.size.y });
            m_velocity.y = 0.f;
            m_onGround = true;
        }
        else {
            m_onGround = false;
        }
    }
    else {
        // Skok w gore 
        m_onGround = false;
        bool tl = tileMap.isSolidAtPixel({ b.position.x + 2.f,            b.position.y });
        bool tr = tileMap.isSolidAtPixel({ b.position.x + b.size.x - 2.f, b.position.y });
        if (tl || tr) {
            float tileRow = std::floor(b.position.y / TS);
            m_shape.setPosition({ b.position.x, (tileRow + 1.f) * TS });
            m_velocity.y = 0.f;
        }
    }

    // ============================================================
    // kolizje POZIOME
    // ============================================================
    b = m_shape.getGlobalBounds();

    // Punkty kontrolne — 20% i 80% wysokosci gracza
    float y1 = b.position.y + b.size.y * 0.2f;
    float y2 = b.position.y + b.size.y * 0.8f;

    // Lewa strona
    if (m_velocity.x < 0.f) {
        bool cl = tileMap.isSolidAtPixel({ b.position.x,        y1 }) ||
            tileMap.isSolidAtPixel({ b.position.x,        y2 });
        if (cl) {
            float tileCol = std::floor(b.position.x / TS);
            m_shape.setPosition({ (tileCol + 1.f) * TS, b.position.y });
            m_velocity.x = 0.f;
        }
    }

    // Prawa strona
    if (m_velocity.x > 0.f) {
        b = m_shape.getGlobalBounds();
        bool cr = tileMap.isSolidAtPixel({ b.position.x + b.size.x, y1 }) ||
            tileMap.isSolidAtPixel({ b.position.x + b.size.x, y2 });
        if (cr) {
            float tileCol = std::floor((b.position.x + b.size.x) / TS);
            m_shape.setPosition({ tileCol * TS - b.size.x, b.position.y });
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
    m_lives--;
    if (m_lives > 0) respawn();
}

void Player::respawn() {
    m_shape.setPosition(m_spawnPoint);
    m_velocity  = {0.f, 0.f};
    m_onGround  = false;
}
