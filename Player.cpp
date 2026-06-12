#include "Player.hpp"
#include <cmath>
#include <iostream>

// ------------------------------------------------------------------ //
//  Stale animacji                                                       //
// ------------------------------------------------------------------ //
static constexpr float RUN_FRAME_TIME = 0.15f;  // sekund na klatke biegu

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
    // Hitbox — przezroczysty, uzywa do kolizji
    m_shape.setSize({ HITBOX_W, HITBOX_H });
    m_shape.setFillColor(sf::Color::Transparent);
    m_shape.setPosition(m_spawnPoint);

    m_texturesLoaded = loadTextures();
    if (m_texturesLoaded) {
        setState(PlayerState::Idle);
    } else {
        // Fallback — czerwony prostokat (stare zachowanie), zeby gra nie crashowala
        m_shape.setFillColor(sf::Color::Red);
    }
}

// ------------------------------------------------------------------ //
//  Ladowanie tekstur                                                    //
// ------------------------------------------------------------------ //
bool Player::loadTextures() {
    struct { sf::Texture& tex; const char* path; } assets[] = {
        { m_texIdle,  "assets/static.png"      },
        { m_texRunR1, "assets/right_run_1.png" },
        { m_texRunR2, "assets/right_run_2.png" },
        { m_texJump,  "assets/jump.png"        },
        { m_texCrawl, "assets/crawl.png"       },
        { m_texDeath, "assets/death.png"       },
    };

    bool ok = true;
    for (auto& a : assets) {
        if (!a.tex.loadFromFile(a.path)) {
            std::cerr << "[Player] Nie mozna wczytac tekstury: " << a.path << "\n";
            ok = false;
        }
    }
    return ok;
}

// ------------------------------------------------------------------ //
//  Ustawienie tekstury + origin na dolny srodek                        //
// ------------------------------------------------------------------ //
void Player::setTextureWithOrigin(sf::Texture& tex) {
    // SFML 3: Sprite musi byc skonstruowany z tekstura — uzywamy emplace
    m_sprite.emplace(tex);
    sf::Vector2u sz = tex.getSize();
    m_sprite->setOrigin({ sz.x / 2.f, static_cast<float>(sz.y) });
}

// ------------------------------------------------------------------ //
//  Zmiana stanu — ustawia teksture, scale i origin                     //
// ------------------------------------------------------------------ //
void Player::setState(PlayerState newState) {
    if (newState == m_state && newState != PlayerState::RunRight
                             && newState != PlayerState::RunLeft)
        return;   // bez zbednych przelaczen (animacja biegu odswiezana wewnetrznie)

    m_state = newState;

    switch (m_state) {
    case PlayerState::Idle:
        setTextureWithOrigin(m_texIdle);
        m_sprite->setScale({ 1.f, 1.f });
        break;

    case PlayerState::RunRight:
        setTextureWithOrigin(m_runFrame ? m_texRunR2 : m_texRunR1);
        m_sprite->setScale({ 1.f, 1.f });
        break;

    case PlayerState::RunLeft:
        setTextureWithOrigin(m_runFrame ? m_texRunR2 : m_texRunR1);
        m_sprite->setScale({ -1.f, 1.f });  // lustrzane odbicie
        break;

    case PlayerState::Jump:
        setTextureWithOrigin(m_texJump);
        // zachowaj scale (kierunek) z poprzedniego stanu
        break;

    case PlayerState::Crawl:
        setTextureWithOrigin(m_texCrawl);
        m_sprite->setScale({ 1.f, 1.f });
        break;

    case PlayerState::Dead:
        setTextureWithOrigin(m_texDeath);
        m_sprite->setScale({ 1.f, 1.f });
        break;
    }

    applySpriteToHitbox();
}

// ------------------------------------------------------------------ //
//  Synchronizacja pozycji sprajta z hitboxem                           //
//  Origin sprajta = dolny srodek tekstury                              //
//  Wiec pozycja sprajta = srodek dolnej krawedzi hitboxa               //
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

    // Okreslamy docelowy stan
    PlayerState target;

    if (!m_onGround) {
        target = PlayerState::Jump;
    } else if (m_velocity.x > 0.f) {
        target = PlayerState::RunRight;
    } else if (m_velocity.x < 0.f) {
        target = PlayerState::RunLeft;
    } else {
        target = PlayerState::Idle;
    }

    // Jesli gracz nie zyje — nie zmieniamy stanu
    if (m_state == PlayerState::Dead) {
        applySpriteToHitbox();
        return;
    }

    // Animacja biegu — przelaczanie klatek
    if (target == PlayerState::RunRight || target == PlayerState::RunLeft) {
        m_animTimer += dt;
        if (m_animTimer >= RUN_FRAME_TIME) {
            m_animTimer = 0.f;
            m_runFrame  = !m_runFrame;
        }
        // Wymuszamy odswiezenie tekstury (klatka mogla sie zmienic)
        m_state = PlayerState::Idle;   // trick: resetuj stan zeby setState zadzialalo
    } else {
        m_animTimer = 0.f;
        m_runFrame  = false;
    }

    setState(target);
}

// ------------------------------------------------------------------ //
//  Update — kolejnosc ma znaczenie!                                     //
// ------------------------------------------------------------------ //
void Player::update(float dt) {
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
//  Input gracza                                                         //
// ------------------------------------------------------------------ //
void Player::handleInput(float dt) {
    float speed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)
                  ? SPRINT_SPEED : MOVE_SPEED;

    m_velocity.x = 0.f;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
        m_velocity.x = -speed;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
        m_velocity.x = speed;

    // Czolganie — wdol + gracz na ziemi
    if ((sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) ||
         sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
        && m_onGround && m_texturesLoaded)
    {
        setState(PlayerState::Crawl);
    }

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

    // ============================================================
    // kolizje PIONOWE
    // ============================================================
    sf::FloatRect b = m_shape.getGlobalBounds();

    if (m_velocity.y >= 0.f) {
        bool bl = tileMap.isSolidAtPixel({ b.position.x + 2.f,            b.position.y + b.size.y });
        bool br = tileMap.isSolidAtPixel({ b.position.x + b.size.x - 2.f, b.position.y + b.size.y });
        if (bl || br) {
            float tileRow = std::floor((b.position.y + b.size.y) / TS);
            m_shape.setPosition({ b.position.x, tileRow * TS - b.size.y });
            m_velocity.y = 0.f;
            m_onGround = true;
        } else {
            m_onGround = false;
        }
    } else {
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

    float y1 = b.position.y + b.size.y * 0.2f;
    float y2 = b.position.y + b.size.y * 0.8f;

    if (m_velocity.x < 0.f) {
        bool cl = tileMap.isSolidAtPixel({ b.position.x, y1 }) ||
                  tileMap.isSolidAtPixel({ b.position.x, y2 });
        if (cl) {
            float tileCol = std::floor(b.position.x / TS);
            m_shape.setPosition({ (tileCol + 1.f) * TS, b.position.y });
            m_velocity.x = 0.f;
        }
    }

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

    // Synchronizuj sprajt po kazdej rozdzielczosci kolizji
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
        window.draw(m_shape);  // fallback gdy brak tekstur
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
    if (m_texturesLoaded)
        setState(PlayerState::Dead);
    m_lives--;
    if (m_lives > 0) respawn();
}

void Player::respawn() {
    m_shape.setPosition(m_spawnPoint);
    m_velocity = { 0.f, 0.f };
    m_onGround = false;
    if (m_texturesLoaded) {
        m_state = PlayerState::Idle;   // reset stanu przed setState
        setState(PlayerState::Idle);
    }
}