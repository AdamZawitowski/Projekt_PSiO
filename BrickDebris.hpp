#pragma once
#include <SFML/Graphics.hpp>
#include <array>

// animacja rozpadu bloku brick — 4 kwadraty wylatujące z rogów
class BrickDebris {
public:
    static constexpr float GRAVITY = 600.f;
    static constexpr float LIFETIME = 3.0f;
    static constexpr float PIECE_SIZE = 10.f;

    // blockPos — pozycja lewego górnego rogu zniszczonego bloku (col*TS, row*TS)
    explicit BrickDebris(sf::Vector2f blockPos) {
        const float TS = 32.f;
        const float cx = blockPos.x + TS / 2.f;
        const float cy = blockPos.y + TS / 2.f;

        struct PieceInit { sf::Vector2f pos; sf::Vector2f vel; };
        std::array<PieceInit, 4> init = { {
            { {blockPos.x,           blockPos.y},       {-90.f, -320.f} },
            { {cx,                   blockPos.y},       { 90.f, -320.f} },
            { {blockPos.x,           cy},               {-90.f, -180.f} },
            { {cx,                   cy},               { 90.f, -180.f} },
        } };

        for (int i = 0; i < 4; ++i) {
            m_shapes[i].setSize({ PIECE_SIZE, PIECE_SIZE });
            m_shapes[i].setFillColor(sf::Color(200, 80, 60));
            m_shapes[i].setPosition(init[i].pos);
            m_velocities[i] = init[i].vel;
        }
    }

    void update(float dt) {
        if (!m_active) return;

        m_timer += dt;
        if (m_timer >= LIFETIME) { m_active = false; return; }

        const float alpha = 1.f - (m_timer / LIFETIME);
        const auto a = static_cast<uint8_t>(alpha * 255.f);

        for (int i = 0; i < 4; ++i) {
            m_velocities[i].y += GRAVITY * dt;
            m_shapes[i].move(m_velocities[i] * dt);

            sf::Color c = m_shapes[i].getFillColor();
            c.a = a;
            m_shapes[i].setFillColor(c);
        }
    }

    void draw(sf::RenderWindow& window) const {
        if (!m_active) return;
        for (const auto& s : m_shapes)
            window.draw(s);
    }

    bool isActive() const { return m_active; }

private:
    std::array<sf::RectangleShape, 4> m_shapes;
    std::array<sf::Vector2f, 4>       m_velocities;
    float m_timer = 0.f;
    bool  m_active = true;
};