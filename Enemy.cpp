#include "Enemy.hpp"
#include "Player.hpp"
#include <cmath>

// ------------------------------------------------------------------ //
//  Konstruktor                                                          //
// ------------------------------------------------------------------ //
Enemy::Enemy(sf::Vector2f startPosition)
    : m_velocity(0.f, 0.f)
    , m_alive(true)
    , m_movingRight(true)
{
    m_shape.setSize({28.f, 28.f});      // rozmiar wroga (jeden kafelek = 32px)
    m_shape.setFillColor(sf::Color(180, 60, 60)); // ciemnoczerwony
    m_shape.setPosition(startPosition);
}

// ------------------------------------------------------------------ //
//  Update                                                               //
// ------------------------------------------------------------------ //
void Enemy::update(float dt, const TileMap& tileMap) {
    if (!m_alive) return;

    applyGravity(dt);
    patrol(dt, tileMap);
    resolveCollisions(tileMap);
}

// ------------------------------------------------------------------ //
//  Grawitacja                                                           //
// ------------------------------------------------------------------ //
void Enemy::applyGravity(float dt) {
    m_velocity.y += GRAVITY * dt;
    if (m_velocity.y > MAX_FALL)
        m_velocity.y = MAX_FALL;
}

// ------------------------------------------------------------------ //
//  Patrolowanie — wrog chodzi w lewo/prawo i zawraca                   //
//  na skraju platformy lub przy scianie                                 //
// ------------------------------------------------------------------ //
void Enemy::patrol(float dt, const TileMap& tileMap) {
    m_velocity.x = m_movingRight ? MOVE_SPEED : -MOVE_SPEED;

    sf::FloatRect b = m_shape.getGlobalBounds();
    const float TS  = TileMap::TILE_SIZE;

    // Sprawdz czy przed wrogiem jest podloga (zapobiega spadaniu z platform)
    if (m_movingRight) {
        sf::Vector2f groundCheck = {
            b.position.x + b.size.x + 2.f,   // przed prawym bokiem
            b.position.y + b.size.y + 1.f     // tuż pod stopami
        };
        if (!tileMap.isSolidAtPixel(groundCheck))
            m_movingRight = false;             // zawroc bo skraj platformy
    } else {
        sf::Vector2f groundCheck = {
            b.position.x - 2.f,               // przed lewym bokiem
            b.position.y + b.size.y + 1.f
        };
        if (!tileMap.isSolidAtPixel(groundCheck))
            m_movingRight = true;
    }

    // Przesuniecie
    m_shape.move(m_velocity * dt);
}

// ------------------------------------------------------------------ //
//  Kolizje z tilemap (uproszczone — tylko gora/dol i boki)             //
// ------------------------------------------------------------------ //
void Enemy::resolveCollisions(const TileMap& tileMap) {
    const float TS  = TileMap::TILE_SIZE;
    sf::FloatRect b = m_shape.getGlobalBounds();

    // Kolizja od dolu (ladowanie)
    if (m_velocity.y >= 0.f) {
        bool bl = tileMap.isSolidAtPixel({b.position.x + 2.f,            b.position.y + b.size.y});
        bool br = tileMap.isSolidAtPixel({b.position.x + b.size.x - 2.f, b.position.y + b.size.y});
        if (bl || br) {
            float tileRow = std::floor((b.position.y + b.size.y) / TS);
            m_shape.setPosition({b.position.x, tileRow * TS - b.size.y});
            m_velocity.y = 0.f;
        }
    }

    b = m_shape.getGlobalBounds();

    // Kolizja boczna — zawroc zamiast przechodzic przez sciane
    float y1 = b.position.y + b.size.y * 0.2f;
    float y2 = b.position.y + b.size.y * 0.8f;

    if (m_velocity.x > 0.f) {
        bool cr = tileMap.isSolidAtPixel({b.position.x + b.size.x, y1}) ||
                  tileMap.isSolidAtPixel({b.position.x + b.size.x, y2});
        if (cr) {
            float tileCol = std::floor((b.position.x + b.size.x) / TS);
            m_shape.setPosition({tileCol * TS - b.size.x, b.position.y});
            m_movingRight = false; // zawroc
            m_velocity.x  = 0.f;
        }
    } else if (m_velocity.x < 0.f) {
        bool cl = tileMap.isSolidAtPixel({b.position.x, y1}) ||
                  tileMap.isSolidAtPixel({b.position.x, y2});
        if (cl) {
            float tileCol = std::floor(b.position.x / TS);
            m_shape.setPosition({(tileCol + 1.f) * TS, b.position.y});
            m_movingRight = true; // zawroc
            m_velocity.x  = 0.f;
        }
    }
}

// ------------------------------------------------------------------ //
//  Kolizja z graczem                                                    //
//  Zwraca true  = gracz depce od gory (zabij wroga)                    //
//  Zwraca false = kolizja z boku     (gracz traci zycie)               //
// ------------------------------------------------------------------ //
bool Enemy::checkCollisionWithPlayer(const Player& player) const {
    if (!m_alive) return false;

    sf::FloatRect enemyBounds  = m_shape.getGlobalBounds();
    sf::FloatRect playerBounds = player.getBounds();

    if (!enemyBounds.findIntersection(playerBounds))
        return false; // brak kolizji

    // Jesli dolna krawedz gracza jest powyzej srodka wroga — depce od gory
    float playerBottom = playerBounds.position.y + playerBounds.size.y;
    float enemyMid     = enemyBounds.position.y + enemyBounds.size.y * 0.4f;

    return playerBottom < enemyMid;
}

// ------------------------------------------------------------------ //
//  Rysowanie                                                            //
// ------------------------------------------------------------------ //
void Enemy::draw(sf::RenderWindow& window) const {
    if (!m_alive) return;
    window.draw(m_shape);
}

// ------------------------------------------------------------------ //
//  Gettery / Settery                                                    //
// ------------------------------------------------------------------ //
bool          Enemy::isAlive()   const { return m_alive; }
void          Enemy::kill()            { m_alive = false; }
sf::FloatRect Enemy::getBounds() const { return m_shape.getGlobalBounds(); }
