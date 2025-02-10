#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <optional>
#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <sstream>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <cstdlib>
#include "KoopaRandomGenerator.h"

struct Koopa {
    std::string name;
    uint32_t distanceToCastle;
    uint32_t walkSpeed;
    uint32_t shellHP;
    uint32_t spawnRound;
    bool isActive;
    size_t spawnOrder;

    Koopa(const std::string& nm, uint32_t dist, uint32_t sp, uint32_t hp, uint32_t round, size_t order)
        : name(nm), distanceToCastle(dist), walkSpeed(sp), shellHP(hp), spawnRound(round), isActive(true), spawnOrder(order) {}

    uint32_t getETA() const {
        return distanceToCastle / walkSpeed;
    }
};

class KoopaComparator {
public:
    bool operator()(const Koopa* a, const Koopa* b) const {
        return a->getETA() > b->getETA();
    }
};

int runGame() {
    uint32_t bagCapacity = 5;
    uint32_t seed = 12345;
    uint32_t maxDist = 700, maxSpeed = 5, maxHP = 3;

    KoopaRandomGenerator::initialize(seed, maxDist, maxSpeed, maxHP);

    sf::RenderWindow window(
        sf::VideoMode(sf::Vector2u(800, 600)),
        "Mario Castle Defense - Play Mode"
    );
    window.setFramerateLimit(60);

    sf::Texture koopaTex, rockTex;
    bool haveKoopaTex = koopaTex.loadFromFile("assets/koopa.png");
    bool haveRockTex = rockTex.loadFromFile("assets/rock.png");

    std::vector<Koopa*> koopas;
    std::priority_queue<Koopa*, std::vector<Koopa*>, KoopaComparator> targetQueue;

    bool gameOver = false;
    uint32_t activeKoopaCount = 0;
    uint32_t roundCounter = 0;
    size_t koopaCounter = 0;

    auto spawnKoopa = [&]() {
        std::string name = KoopaRandomGenerator::getNextKoopaName();
        uint32_t dist = KoopaRandomGenerator::getNextKoopaDistance();
        uint32_t sp = KoopaRandomGenerator::getNextKoopaSpeed();
        uint32_t hp = KoopaRandomGenerator::getNextKoopaHealth();
        Koopa* k = new Koopa(name, dist, sp, hp, roundCounter, koopaCounter++);
        koopas.push_back(k);
        targetQueue.push(k);
        activeKoopaCount++;
        std::cout << "Spawned: " << name << " (distance: " << dist
                  << ", speed: " << sp << ", health: " << hp << ")\n";
    };

    for (int i = 0; i < 5; ++i) {
        spawnKoopa();
    }

    auto throwRock = [&]() {
        if (bagCapacity == 0) return;
        bagCapacity--;

        if (!targetQueue.empty()) {
            Koopa* target = targetQueue.top();
            targetQueue.pop();
            if (!target->isActive || target->shellHP == 0) return;
            target->shellHP--;
            if (target->shellHP == 0) {
                target->isActive = false;
                activeKoopaCount--;
                std::cout << "Knocked Out: " << target->name << "\n";
            } else {
                targetQueue.push(target);
            }
        }
    };

    auto moveKoopas = [&]() {
        Koopa* castleBreacher = nullptr;
        for (auto* k : koopas) {
            if (!k->isActive) continue;
            uint32_t step = std::min(k->distanceToCastle, k->walkSpeed);
            k->distanceToCastle -= step;
            if (k->distanceToCastle == 0 && !castleBreacher) {
                castleBreacher = k;
            }
        }
        if (castleBreacher) {
            gameOver = true;
            std::cout << "DEFEAT! " << castleBreacher->name << " reached the castle!\n";
        }
    };

    auto drawKoopas = [&](sf::RenderWindow& win) {
        for (auto* k : koopas) {
            if (!k->isActive) continue;
            float x = 700.f - k->distanceToCastle;
            float y = 50.f + (k->spawnOrder % 10) * 25.f;
            if (haveKoopaTex) {
                sf::Sprite spr(koopaTex);
                spr.setOrigin(sf::Vector2f(16.f, 16.f));
                spr.setPosition(sf::Vector2f(x, y));
                win.draw(spr);
            } else {
                sf::CircleShape shape(15.f);
                shape.setOrigin(sf::Vector2f(15.f, 15.f));
                shape.setPosition(sf::Vector2f(x, y));
                shape.setFillColor(sf::Color::Green);
                if (k->shellHP < 2) shape.setFillColor(sf::Color::Red);
                win.draw(shape);
            }
        }
    };

    auto drawRocks = [&](sf::RenderWindow& win) {
        if (haveRockTex) {
            for (uint32_t i = 0; i < bagCapacity; i++) {
                sf::Sprite rock(rockTex);
                rock.setPosition(sf::Vector2f(30.f + i * 25.f, 550.f));
                win.draw(rock);
            }
        }
    };

    while (!gameOver && window.isOpen()) {
        roundCounter++;
        moveKoopas();

        if (gameOver) break;

        auto eOpt = window.pollEvent();
        while (eOpt.has_value()) {
            sf::Event ev = eOpt.value();
            if (ev.kind == sf::Event::Kind::CloseRequested) {
                window.close();
                gameOver = true;
            }
            if (ev.kind == sf::Event::Kind::MouseButtonPressed) {
                if (ev.mouseButton.button == sf::Mouse::Left) {
                    throwRock();
                }
            }
            eOpt = window.pollEvent();
        }

        window.clear(sf::Color(50, 50, 50));
        drawKoopas(window);
        drawRocks(window);
        window.display();

        sf::sleep(sf::milliseconds(100));
    }

    sf::Clock endClock;
    while (endClock.getElapsedTime().asSeconds() < 2.f && window.isOpen()) {
        auto eOpt = window.pollEvent();
        while (eOpt.has_value()) {
            sf::Event ev = eOpt.value();
            if (ev.kind == sf::Event::Kind::CloseRequested) {
                window.close();
            }
            eOpt = window.pollEvent();
        }
        window.clear(sf::Color(20, 20, 20));
        window.display();
    }

    window.close();
    std::cout << "[play] Game Over.\n";
    return 0;
}
