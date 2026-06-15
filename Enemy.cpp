#include "Enemy.hpp"
#include "Player.hpp"
#include <cmath>
#include <iostream>

// ------------------------------------------------------------------ //
//  Konstruktor                                                          //
// ------------------------------------------------------------------ //
Enemy::Enemy(sf::Vector2f startPosition)
    : m_velocity(0.f, 0.f)
    , m_alive(true)
    , m_movingRight(true)
    , m_textureLoaded(false)
{
    // Hitbox — przezroczysty, uzywa do kolizji
    m_shape.setSize({HITBOX_W, HITBOX_H});
    m_shape.setFillColor(sf::Color::Transparent);
    m_shape.setPosition(startPosition);

    // Ladowanie tekstury
    if (m_texture.loadFromFile("assets/enemy.png")) {
        m_sprite.emplace(m_texture);

        // Origin na dolny srodek tekstury — identycznie jak u Playera
        sf::Vector2u sz = m_texture.getSize();
        m_sprite->setOrigin({ sz.x / 2.f, static_cast<float>(sz.y) });

        m_textureLoaded = true;
        applySpriteToHitbox();
    } else {
        std::cerr << "[Enemy] Nie mozna wczytac tekstury: assets/enemy.png\n";
        // Fallback — widoczny czerwony prostokat
        m_shape.setFillColor(sf::Color(180, 60, 60));
    }
}

// ------------------------------------------------------------------ //
//  Move constructor i move assignment                                   //
//  Po przeniesieniu Enemy w pamieci (np. przez std::vector::emplace_back //
//  z realokacja) sf::Sprite musi dostac zaktualizowany adres tekstury. //
// ------------------------------------------------------------------ //
Enemy::Enemy(Enemy&& other) noexcept
    : m_shape        (std::move(other.m_shape))
    , m_velocity     (other.m_velocity)
    , m_alive        (other.m_alive)
    , m_movingRight  (other.m_movingRight)
    , m_texture      (std::move(other.m_texture))
    , m_textureLoaded(other.m_textureLoaded)
{
    if (m_textureLoaded) {
        // Sprite musi wskazywac na NOWY adres tekstury (po przeniesieniu)
        m_sprite.emplace(m_texture);
        sf::Vector2u sz = m_texture.getSize();
        m_sprite->setOrigin({ sz.x / 2.f, static_cast<float>(sz.y) });
        // Przywroc skalowanie kierunku
        m_sprite->setScale(other.m_sprite ? other.m_sprite->getScale()
                                          : sf::Vector2f(1.f, 1.f));
        applySpriteToHitbox();
    }
}

Enemy& Enemy::operator=(Enemy&& other) noexcept {
    if (this == &other) return *this;
    m_shape         = std::move(other.m_shape);
    m_velocity      = other.m_velocity;
    m_alive         = other.m_alive;
    m_movingRight   = other.m_movingRight;
    m_texture       = std::move(other.m_texture);
    m_textureLoaded = other.m_textureLoaded;
    m_sprite.reset();
    if (m_textureLoaded) {
        m_sprite.emplace(m_texture);
        sf::Vector2u sz = m_texture.getSize();
        m_sprite->setOrigin({ sz.x / 2.f, static_cast<float>(sz.y) });
        m_sprite->setScale(other.m_sprite ? other.m_sprite->getScale()
                                          : sf::Vector2f(1.f, 1.f));
        applySpriteToHitbox();
    }
    return *this;
}

// ------------------------------------------------------------------ //
//  Synchronizacja pozycji sprajta z hitboxem                           //
//  Origin sprajta = dolny srodek — wiec pozycja = srodek dolnej       //
//  krawedzi hitboxa                                                     //
// ------------------------------------------------------------------ //
void Enemy::applySpriteToHitbox() {
    if (!m_sprite) return;
    sf::FloatRect b = m_shape.getGlobalBounds();
    m_sprite->setPosition({
        b.position.x + b.size.x / 2.f,  // srodek X hitboxa
        b.position.y + b.size.y          // dolna krawedz hitboxa
    });
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

    // Odbicie lustrzane sprajta w zaleznosci od kierunku ruchu
    if (m_sprite) {
        m_sprite->setScale({ m_movingRight ? 1.f : -1.f, 1.f });
    }

    sf::FloatRect b = m_shape.getGlobalBounds();

    // Sprawdz czy przed wrogiem jest podloga (zapobiega spadaniu z platform)
    if (m_movingRight) {
        sf::Vector2f groundCheck = {
            b.position.x + b.size.x + 2.f,  // przed prawym bokiem
            b.position.y + b.size.y + 1.f   // tuz pod stopami
        };
        if (!tileMap.isSolidAtPixel(groundCheck))
            m_movingRight = false;
    } else {
        sf::Vector2f groundCheck = {
            b.position.x - 2.f,             // przed lewym bokiem
            b.position.y + b.size.y + 1.f
        };
        if (!tileMap.isSolidAtPixel(groundCheck))
            m_movingRight = true;
    }

    // Przesuniecie
    m_shape.move(m_velocity * dt);
}

// ------------------------------------------------------------------ //
//  Kolizje z tilemap (gora/dol i boki)                                 //
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
            m_movingRight = false;
            m_velocity.x  = 0.f;
        }
    } else if (m_velocity.x < 0.f) {
        bool cl = tileMap.isSolidAtPixel({b.position.x, y1}) ||
                  tileMap.isSolidAtPixel({b.position.x, y2});
        if (cl) {
            float tileCol = std::floor(b.position.x / TS);
            m_shape.setPosition({(tileCol + 1.f) * TS, b.position.y});
            m_movingRight = true;
            m_velocity.x  = 0.f;
        }
    }

    // Synchronizuj sprajt po rozwiazaniu kolizji
    applySpriteToHitbox();
}

// ------------------------------------------------------------------ //
//  Kolizja z graczem                                                    //
//  Zwraca true  = gracz depce od gory (zabij wroga)                    //
//  Zwraca false = kolizja z boku     (gracz traci zycie)               //
// ------------------------------------------------------------------ //
bool Enemy::checkCollisionWithPlayer(const Player& player) const {
    if (!m_alive) return false;

    sf::FloatRect enemyBounds  = m_shape.getGlobalBounds();  // hitbox, nie sprite
    sf::FloatRect playerBounds = player.getBounds();

    if (!enemyBounds.findIntersection(playerBounds))
        return false;

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

    if (m_sprite)
        window.draw(*m_sprite);
    else
        window.draw(m_shape);  // fallback gdy brak tekstury
}

// ------------------------------------------------------------------ //
//  Gettery / Settery                                                    //
// ------------------------------------------------------------------ //
bool          Enemy::isAlive()   const { return m_alive; }
void          Enemy::kill()            { m_alive = false; }
sf::FloatRect Enemy::getBounds() const { return m_shape.getGlobalBounds(); }