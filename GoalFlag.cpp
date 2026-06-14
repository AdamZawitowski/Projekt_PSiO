#include "GoalFlag.hpp"
#include "Player.hpp"

// ------------------------------------------------------------------ //
//  Konstruktor                                                          //
// ------------------------------------------------------------------ //
GoalFlag::GoalFlag(sf::Vector2f position) {
    // Maszt — wysoki, cienki, bialy
    m_pole.setSize({6.f, HEIGHT});
    m_pole.setFillColor(sf::Color(200, 200, 200));
    m_pole.setPosition(position);

    // Flaga — zielony prostokat przy gorze masztu
    m_flag.setSize({24.f, 16.f});
    m_flag.setFillColor(sf::Color(0, 180, 0));
    m_flag.setPosition({position.x + 6.f, position.y});
}

// ------------------------------------------------------------------ //
//  Rysowanie                                                            //
// ------------------------------------------------------------------ //
void GoalFlag::draw(sf::RenderWindow& window) const {
    window.draw(m_pole);
    window.draw(m_flag);
}

// ------------------------------------------------------------------ //
//  Kolizja z graczem                                                    //
// ------------------------------------------------------------------ //
bool GoalFlag::checkCollisionWithPlayer(const Player& player) const {
    return m_pole.getGlobalBounds()
                 .findIntersection(player.getBounds())
                 .has_value();
}

sf::FloatRect GoalFlag::getBounds() const {
    return m_pole.getGlobalBounds();
}
