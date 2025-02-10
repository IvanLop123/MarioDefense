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

// Include your unchanged KoopaRandomGenerator
#include "KoopaRandomGenerator.h"

// Wave config
struct RoundConfig {
    uint32_t waveNumber;
    uint32_t randomKoopas;
    uint32_t namedKoopas;
    std::vector<std::string> koopaLines;
};

// Example Koopa struct
class Koopa {
public:
    std::string name;
    uint32_t distanceToCastle;
    uint32_t walkSpeed;
    uint32_t shellHP;
    uint32_t spawnRound;
    uint32_t knockOutRound;
    bool     isActive;
    size_t   spawnOrder;
    uint32_t knockOutOrder;

    Koopa(const std::string &nm,
          uint32_t dist,
          uint32_t sp,
          uint32_t hp,
          uint32_t round,
          size_t order)
     : name(nm),
       distanceToCastle(dist),
       walkSpeed(sp),
       shellHP(hp),
       spawnRound(round),
       knockOutRound(0),
       isActive(true),
       spawnOrder(order),
       knockOutOrder(0)
    {}

    uint32_t getETA() const {
        return distanceToCastle / walkSpeed; 
    }
};

class KoopaComparator {
public:
    bool operator()(const Koopa* a, const Koopa* b) const {
        uint32_t ea=a->getETA(), eb=b->getETA();
        if(ea!=eb) return ea>eb;
        if(a->shellHP!=b->shellHP) return a->shellHP>b->shellHP;
        return a->name>b->name;
    }
};

// Minimal median tracker
class KnockOutMedianTracker {
private:
    std::priority_queue<uint32_t,std::vector<uint32_t>,std::less<uint32_t>> lower;
    std::priority_queue<uint32_t,std::vector<uint32_t>,std::greater<uint32_t>> upper;
public:
    void add(uint32_t val){
        if(lower.empty()||val<=lower.top()) {
            lower.push(val);
        } else {
            upper.push(val);
        }
        if(lower.size()>upper.size()+1) {
            upper.push(lower.top()); lower.pop();
        } else if(upper.size()>lower.size()+1) {
            lower.push(upper.top()); upper.pop();
        }
    }
    bool empty() const { return(lower.empty() && upper.empty()); }
    uint32_t getMedian() const {
        if(lower.empty()&&upper.empty()) return 0;
        if(lower.size()==upper.size()){
            uint64_t a=lower.top(), b=upper.top();
            return (uint32_t)((a+b)/2ULL);
        } else if(lower.size()>upper.size()) {
            return lower.top();
        } else {
            return upper.top();
        }
    }
};

// The function that merges wave-based logic with SFML 3 alpha
int runSimulation(){
    // Read the 'header'
    {
        std::string ignored;
        std::getline(std::cin, ignored); 
    }
    uint32_t bagCapacity, seed, maxDist, maxSpeed, maxHP;
    {
        std::string dummy;
        std::cin >> dummy >> bagCapacity;
        std::cin >> dummy >> seed;
        std::cin >> dummy >> maxDist;
        std::cin >> dummy >> maxSpeed;
        std::cin >> dummy >> maxHP;
        KoopaRandomGenerator::initialize(seed, maxDist, maxSpeed, maxHP);
    }

    // read wave data
    std::vector<RoundConfig> waveConfigs;
    while(true){
        std::string line;
        if(!std::getline(std::cin, line)) break;
        if(line.empty()) continue;
        if(line[0]=='-'){
            RoundConfig rc;
            std::string tmp;
            std::cin >> tmp >> rc.waveNumber;
            std::cin >> tmp >> rc.randomKoopas;
            std::cin >> tmp >> rc.namedKoopas;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            for(uint32_t i=0;i<rc.namedKoopas;i++){
                if(!std::getline(std::cin, line)) break;
                rc.koopaLines.push_back(line);
            }
            waveConfigs.push_back(rc);
        }
    }
    std::sort(waveConfigs.begin(), waveConfigs.end(),
        [](auto &a, auto &b){return a.waveNumber<b.waveNumber;});

    // State
    std::vector<Koopa*> allKoopas;
    bool gameOver=false;
    uint32_t activeCount=0;
    size_t koopaCounter=0;
    uint32_t currentRound=0;
    size_t currentWaveIndex=0;
    uint32_t nextWaveNumber= waveConfigs.empty()? std::numeric_limits<uint32_t>::max(): waveConfigs[0].waveNumber;

    std::priority_queue<Koopa*,std::vector<Koopa*>,KoopaComparator> targetQueue;
    uint32_t knockOutCounter=0;

    // Create an SFML 3 alpha window with Vector2u
    sf::RenderWindow window(
        sf::VideoMode(sf::Vector2u(800,600)),
        "Mario Defense (SFML 3 alpha)"
    );
    window.setFramerateLimit(60);

    sf::Texture koopaTex;
    bool haveTexture=false;
    if(koopaTex.loadFromFile("assets/koopa.png")) {
        haveTexture=true;
    } else {
        std::cerr<<"[simulate] can't load assets/koopa.png => fallback.\n";
    }

    // spawnKoopas
    auto spawnKoopas = [&](uint32_t randomCount,const std::vector<std::string>& lines){
        for(uint32_t i=0;i<randomCount;i++){
            auto nm=KoopaRandomGenerator::getNextKoopaName();
            auto dist=KoopaRandomGenerator::getNextKoopaDistance();
            auto sp=KoopaRandomGenerator::getNextKoopaSpeed();
            auto hp=KoopaRandomGenerator::getNextKoopaHealth();
            Koopa* k=new Koopa(nm, dist, sp, hp, currentRound, koopaCounter++);
            allKoopas.push_back(k);
            activeCount++;
            targetQueue.push(k);
            std::cout<<"Spawned: "<<nm<<" dist="<<dist
                     <<" sp="<<sp<<" hp="<<hp<<"\n";
        }
        for(auto&ln : lines){
            std::istringstream iss(ln);
            std::string nm; iss>>nm;
            uint32_t dist=0, sp=0, hp=0;
            std::string d;
            iss>>d>>dist>>d>>sp>>d>>hp;
            Koopa* k=new Koopa(nm,dist,sp,hp,currentRound,koopaCounter++);
            allKoopas.push_back(k);
            activeCount++;
            targetQueue.push(k);
            std::cout<<"Spawned: "<<nm
                     <<" dist="<<dist
                     <<" sp="<<sp
                     <<" hp="<<hp<<"\n";
        }
    };

    auto moveKoopas = [&](){
        Koopa* killer=nullptr;
        for(auto*k: allKoopas){
            if(!k->isActive) continue;
            if(k->spawnRound>=currentRound) continue;
            uint32_t step=std::min(k->distanceToCastle,k->walkSpeed);
            k->distanceToCastle-=step;
            std::cout<<"Moved: "<<k->name
                     <<" => dist="<<k->distanceToCastle
                     <<" sp="<<k->walkSpeed
                     <<" hp="<<k->shellHP<<"\n";
            if(k->distanceToCastle==0 && !killer){
                killer=k;
                k->knockOutRound=currentRound;
            }
        }
        if(killer){
            gameOver=true;
            std::cout<<"DEFEAT IN ROUND "<<currentRound<<"! "
                     <<killer->name<<" reached castle!\n";
        }
    };

    auto throwRocks=[&](uint32_t ammo){
        while(ammo>0){
            while(!targetQueue.empty() && (!targetQueue.top()->isActive||targetQueue.top()->shellHP==0)){
                targetQueue.pop();
            }
            if(targetQueue.empty()) break;
            Koopa* topK=targetQueue.top(); targetQueue.pop();
            if(!topK->isActive||topK->shellHP==0) continue;
            topK->shellHP--;
            ammo--;
            if(topK->shellHP==0){
                topK->isActive=false;
                topK->knockOutRound=currentRound;
                topK->knockOutOrder=++knockOutCounter;
                activeCount--;
                std::cout<<"Knocked Out: "<<topK->name
                         <<" => dist="<<topK->distanceToCastle
                         <<" sp="<<topK->walkSpeed
                         <<" hp="<<topK->shellHP<<"\n";
            } else {
                targetQueue.push(topK);
            }
        }
    };

    auto checkVictory=[&]()->bool{
        if(activeCount==0){
            if(currentWaveIndex>=waveConfigs.size()){
                gameOver=true;
                Koopa* lastK=nullptr;
                for(auto*k: allKoopas){
                    if(k->knockOutOrder>0){
                        if(!lastK||k->knockOutOrder>lastK->knockOutOrder){
                            lastK=k;
                        }
                    }
                }
                std::cout<<"VICTORY IN ROUND "<<currentRound<<"!";
                if(lastK) {
                    std::cout<<" "<<lastK->name<<" was the final Koopa.";
                }
                std::cout<<"\n";
                return true;
            }
        }
        return false;
    };

    // A function to draw the Koopas
    auto drawKoopas=[&](sf::RenderWindow &win){
        for(auto*k: allKoopas){
            if(!k->isActive) continue;
            float x=700.f - k->distanceToCastle;
            float y=50.f + (k->spawnOrder%12)*25.f;
            if(haveTexture){
                sf::Sprite spr(koopaTex);
                // SFML 3 alpha => setOrigin(...) must pass Vector2f
                spr.setOrigin(sf::Vector2f(16.f,16.f));
                // setPosition => sf::Vector2f
                spr.setPosition(sf::Vector2f(x,y));
                win.draw(spr);
            } else {
                sf::CircleShape shape(15.f);
                shape.setOrigin(sf::Vector2f(15.f,15.f));
                shape.setPosition(sf::Vector2f(x,y));
                shape.setFillColor(sf::Color::Green);
                if(k->shellHP<2) shape.setFillColor(sf::Color::Red);
                win.draw(shape);
            }
        }
    };

    // main loop
    while(!gameOver && window.isOpen()){
        currentRound++;
        std::cout<<"[simulate] Round: "<<currentRound<<"\n";
        moveKoopas();
        if(gameOver) break;

        if(currentWaveIndex<waveConfigs.size() && currentRound==nextWaveNumber){
            auto &cfg=waveConfigs[currentWaveIndex];
            spawnKoopas(cfg.randomKoopas, cfg.koopaLines);
            currentWaveIndex++;
            if(currentWaveIndex<waveConfigs.size()){
                nextWaveNumber=waveConfigs[currentWaveIndex].waveNumber;
            } else {
                nextWaveNumber=std::numeric_limits<uint32_t>::max();
            }
        }
        throwRocks(bagCapacity);
        if(gameOver) break;
        if(checkVictory()) break;

        // Show for 1 second
        sf::Clock clk;
        while(clk.getElapsedTime().asSeconds()<1.f && !gameOver && window.isOpen()){
            // SFML 3 alpha => pollEvent returns std::optional<sf::Event>
            auto eOpt = window.pollEvent();
            while(eOpt.has_value()) {
                sf::Event ev = eOpt.value();
                // If your version truly has ev.kind:
                if(ev.kind == sf::Event::Kind::CloseRequested){
                    window.close();
                    gameOver=true;
                }
                eOpt=window.pollEvent();
            }
            window.clear(sf::Color(30,30,30));
            drawKoopas(window);
            window.display();
        }
    }

    // final display
    if(window.isOpen()){
        sf::Clock endC;
        while(endC.getElapsedTime().asSeconds()<2.f && window.isOpen()){
            auto eOpt = window.pollEvent();
            while(eOpt.has_value()){
                sf::Event ev = eOpt.value();
                if(ev.kind == sf::Event::Kind::CloseRequested){
                    window.close();
                }
                eOpt=window.pollEvent();
            }
            window.clear(sf::Color(10,10,10));
            drawKoopas(window);
            window.display();
        }
        window.close();
    }

    std::cout<<"[simulate] Done.\n";
    return 0;
}
