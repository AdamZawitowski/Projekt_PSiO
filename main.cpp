#include <SFML/Graphics.hpp>
#include "Player.hpp"
#include "TileMap.hpp"

int main() {
    sf::RenderWindow window(sf::VideoMode({800, 480}), "Mario Contra 2");

    TileMap tileMap;
    if (!tileMap.loadFromFile("level1.txt"))
        return 1;

    Player player;
    sf::Clock clock;

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();
        }

        const float dt = clock.restart().asSeconds();
        player.update(dt);

        window.clear(sf::Color(135, 206, 235));
        tileMap.draw(window);
        player.draw(window);
        window.display();
    }

    return 0;
}
