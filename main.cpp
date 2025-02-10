#include <SFML/Graphics.hpp>
#include <iostream>
#include <cstdlib>  // For system() calls

int main() {
    // Create a window
    sf::RenderWindow window(sf::VideoMode(600, 400), "Mario Castle Defense");

    // Load font
    sf::Font font;
    if (!font.loadFromFile("assets/font.ttf")) {  // Ensure you have a font in assets/
        std::cerr << "Error loading font!\n";
        return 1;
    }

    // Create "Simulate" button
    sf::RectangleShape simulateButton(sf::Vector2f(200, 50));
    simulateButton.setPosition(200, 120);
    simulateButton.setFillColor(sf::Color::Blue);

    sf::Text simulateText("Simulate", font, 24);
    simulateText.setPosition(250, 130);
    simulateText.setFillColor(sf::Color::White);

    // Create "Play" button
    sf::RectangleShape playButton(sf::Vector2f(200, 50));
    playButton.setPosition(200, 200);
    playButton.setFillColor(sf::Color::Green);

    sf::Text playText("Play", font, 24);
    playText.setPosition(270, 210);
    playText.setFillColor(sf::Color::White);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::MouseButtonPressed) {
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);

                // Check if "Simulate" button is clicked
                if (simulateButton.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
                    std::cout << "Running Simulation Mode...\n";
                    system("./simulate");  // Run simulate program
                }

                // Check if "Play" button is clicked
                if (playButton.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
                    std::cout << "Running Play Mode...\n";
                    system("./play");  // Run play program
                }
            }
        }

        // Render
        window.clear(sf::Color::Black);
        window.draw(simulateButton);
        window.draw(simulateText);
        window.draw(playButton);
        window.draw(playText);
        window.display();
    }

    return 0;
}
