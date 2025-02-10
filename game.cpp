#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <sstream>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <getopt.h>
#include <limits>
#include "KoopaRandomGenerator.h"

using namespace std;

struct RoundConfig {
    uint32_t waveNumber;
    uint32_t randomKoopas;
    uint32_t namedKoopas;
    vector<string> koopaLines;
};

class Koopa {
public:
    string   name;
    uint32_t distanceToCastle;
    uint32_t walkSpeed;
    uint32_t shellHP;
    uint32_t spawnRound;
    uint32_t knockOutRound;
    bool     isActive;
    size_t   spawnOrder;
    uint32_t knockOutOrder;

    Koopa(const string &n, uint32_t dist, uint32_t sp, uint32_t hp,
          uint32_t sRound, size_t order)
     : name(n),
       distanceToCastle(dist),
       walkSpeed(sp),
       shellHP(hp),
       spawnRound(sRound),
       knockOutRound(0),
       isActive(true),
       spawnOrder(order),
       knockOutOrder(0)
    {}

    uint32_t getETA() const {
        return distanceToCastle / walkSpeed;
    }

    uint32_t getActiveRounds(uint32_t endRound) const {
        if (!isActive && knockOutOrder != 0) {
            return knockOutRound - spawnRound + 1;
        }
        if (!isActive && knockOutOrder == 0) {
            return knockOutRound - spawnRound + 1;
        }
        return endRound - spawnRound + 1;
    }
};

class KoopaComparator {
public:
    bool operator()(const Koopa *a, const Koopa *b) const {
        uint32_t etaA = a->getETA();
        uint32_t etaB = b->getETA();
        if (etaA != etaB) return etaA > etaB;
        if (a->shellHP != b->shellHP) return a->shellHP > b->shellHP;
        return a->name > b->name;
    }
};

class KnockOutMedianTracker {
private:
    priority_queue<uint32_t, vector<uint32_t>, less<uint32_t>> lowerHalf;
    priority_queue<uint32_t, vector<uint32_t>, greater<uint32_t>> upperHalf;
public:
    void add(uint32_t val) {
        if (lowerHalf.empty() || val <= lowerHalf.top()) {
            lowerHalf.push(val);
        } else {
            upperHalf.push(val);
        }
        if (lowerHalf.size() > upperHalf.size() + 1) {
            upperHalf.push(lowerHalf.top());
            lowerHalf.pop();
        } else if (upperHalf.size() > lowerHalf.size() + 1) {
            lowerHalf.push(upperHalf.top());
            upperHalf.pop();
        }
    }
    bool empty() const {
        return lowerHalf.empty() && upperHalf.empty();
    }
    uint32_t getMedian() const {
        if (lowerHalf.empty() && upperHalf.empty()) return 0;
        if (lowerHalf.size() == upperHalf.size()) {
            uint64_t a = lowerHalf.top();
            uint64_t b = upperHalf.top();
            return static_cast<uint32_t>((a + b) / 2ULL);
        } else if (lowerHalf.size() > upperHalf.size()) {
            return lowerHalf.top();
        }
        return upperHalf.top();
    }
};

class MarioCastleDefense {
private:
    uint32_t bagCapacity;
    vector<RoundConfig> waveConfigs;
    size_t currentWaveIndex;
    uint32_t nextWaveNumber;

    vector<Koopa*> allKoopas;
    size_t koopaCounter;

    priority_queue<Koopa*, vector<Koopa*>, KoopaComparator> targetQueue;
    uint32_t activeKoopaCount;
    uint32_t currentRound;
    bool gameOver;

    bool verbose;
    bool trackMedian;
    uint32_t statsCount;

    uint32_t knockOutCounter;
    KnockOutMedianTracker medianTracker;

public:
    MarioCastleDefense(bool v, bool m, uint32_t s)
      : bagCapacity(0),
        currentWaveIndex(0),
        nextWaveNumber(numeric_limits<uint32_t>::max()),
        koopaCounter(0),
        activeKoopaCount(0),
        currentRound(0),
        gameOver(false),
        verbose(v),
        trackMedian(m),
        statsCount(s),
        knockOutCounter(0)
    {}

    ~MarioCastleDefense() {
        for (auto *k : allKoopas) {
            delete k;
        }
    }

    void readHeader() {
        {
            string ignored;
            getline(cin, ignored);
        }
        {
            string dummy;
            uint32_t seed, maxDist, maxSpeed, maxHP;
            cin >> dummy >> bagCapacity;
            cin >> dummy >> seed;
            cin >> dummy >> maxDist;
            cin >> dummy >> maxSpeed;
            cin >> dummy >> maxHP;
            KoopaRandomGenerator::initialize(seed, maxDist, maxSpeed, maxHP);
        }
    }

    void readWaves() {
        while (true) {
            string firstLine;
            if (!getline(cin, firstLine)) break;
            if (firstLine.empty()) continue;
            if (firstLine[0] == '-') {
                RoundConfig rc;
                string tmp;
                cin >> tmp >> rc.waveNumber;
                cin >> tmp >> rc.randomKoopas;
                cin >> tmp >> rc.namedKoopas;
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                for (uint32_t i = 0; i < rc.namedKoopas; i++) {
                    if (!getline(cin, firstLine)) break;
                    rc.koopaLines.push_back(firstLine);
                }
                waveConfigs.push_back(rc);
            }
        }
        sort(waveConfigs.begin(), waveConfigs.end(),
             [](const RoundConfig &a, const RoundConfig &b){
                 return a.waveNumber < b.waveNumber;
             });
        currentWaveIndex = 0;
        if (!waveConfigs.empty()) {
            nextWaveNumber = waveConfigs[0].waveNumber;
        }
    }

    void moveKoopas() {
        Koopa* castleBreacher = nullptr;
        for (auto *k : allKoopas) {
            if (!k->isActive) continue;
            if (k->spawnRound >= currentRound) continue;
            uint32_t step = min(k->distanceToCastle, k->walkSpeed);
            k->distanceToCastle -= step;
            if (verbose) {
                cout << "Moved: " << k->name
                     << " (distance: " << k->distanceToCastle
                     << ", speed: " << k->walkSpeed
                     << ", health: " << k->shellHP << ")\n";
            }
            if (k->distanceToCastle == 0 && !castleBreacher) {
                castleBreacher = k;
                k->knockOutRound = currentRound;
            }
        }
        if (castleBreacher) {
            gameOver = true;
            cout << "DEFEAT IN ROUND " << currentRound << "! "
                 << castleBreacher->name << " reached the castle!\n";
            if (statsCount > 0) {
                printStats();
            }
        }
    }

    void spawnKoopas(uint32_t randomCount, const vector<string> &lines) {
        for (uint32_t i = 0; i < randomCount; i++) {
            string nm = KoopaRandomGenerator::getNextKoopaName();
            uint32_t dist = KoopaRandomGenerator::getNextKoopaDistance();
            uint32_t sp   = KoopaRandomGenerator::getNextKoopaSpeed();
            uint32_t hp   = KoopaRandomGenerator::getNextKoopaHealth();
            Koopa *k = new Koopa(nm, dist, sp, hp, currentRound, koopaCounter++);
            allKoopas.push_back(k);
            activeKoopaCount++;
            targetQueue.push(k);
            if (verbose) {
                cout << "Spawned: " << nm
                     << " (distance: " << dist
                     << ", speed: " << sp
                     << ", health: " << hp << ")\n";
            }
        }
        for (auto &ln : lines) {
            istringstream iss(ln);
            string nm; iss >> nm;
            uint32_t dist=0, sp=0, hp=0;
            string d;
            iss >> d >> dist >> d >> sp >> d >> hp;
            Koopa *k = new Koopa(nm, dist, sp, hp, currentRound, koopaCounter++);
            allKoopas.push_back(k);
            activeKoopaCount++;
            targetQueue.push(k);
            if (verbose) {
                cout << "Spawned: " << nm
                     << " (distance: " << dist
                     << ", speed: " << sp
                     << ", health: " << hp << ")\n";
            }
        }
    }

    void throwRocks() {
        uint32_t rocks = bagCapacity;
        while (rocks > 0) {
            while (!targetQueue.empty() &&
                   (!targetQueue.top()->isActive ||
                    targetQueue.top()->shellHP == 0)) {
                targetQueue.pop();
            }
            if (targetQueue.empty()) break;

            Koopa *k = targetQueue.top();
            targetQueue.pop();
            if (!k->isActive || k->shellHP == 0) continue;
            k->shellHP--;
            rocks--;
            if (k->shellHP == 0) {
                k->isActive = false;
                k->knockOutRound = currentRound;
                k->knockOutOrder = ++knockOutCounter;
                activeKoopaCount--;
                if (verbose) {
                    cout << "Knocked Out: " << k->name
                         << " (distance: " << k->distanceToCastle
                         << ", speed: " << k->walkSpeed
                         << ", health: " << k->shellHP << ")\n";
                }
                if (trackMedian) {
                    uint32_t life = k->getActiveRounds(currentRound);
                    medianTracker.add(life);
                }
            }
            else {
                targetQueue.push(k);
            }
        }
        if (trackMedian && !medianTracker.empty()) {
            cout << "At the end of round " << currentRound
                 << ", the median Koopa active-time is "
                 << medianTracker.getMedian() << "\n";
        }
    }

    bool checkVictory() {
        if (activeKoopaCount == 0) {
            if (currentWaveIndex >= waveConfigs.size()) {
                gameOver = true;
                Koopa* lastKnocked = nullptr;
                for (auto *k : allKoopas) {
                    if (k->knockOutOrder > 0) {
                        if (!lastKnocked ||
                            k->knockOutOrder > lastKnocked->knockOutOrder) {
                            lastKnocked = k;
                        }
                    }
                }
                cout << "VICTORY IN ROUND " << currentRound << "!";
                if (lastKnocked) {
                    cout << " " << lastKnocked->name
                         << " was the final Koopa.";
                }
                cout << "\n";
                if (statsCount > 0) {
                    printStats();
                }
                return true;
            }
        }
        return false;
    }

    void printStats() {
        cout << "Koopas still active: " << activeKoopaCount << "\n";
        vector<Koopa*> knocked;
        for (auto *k : allKoopas) {
            if (k->knockOutOrder != 0) {
                knocked.push_back(k);
            }
        }
        size_t c = knocked.size();
        size_t n = min<size_t>(c, statsCount);

        cout << "First Koopas knocked out:\n";
        if (!knocked.empty()) {
            sort(knocked.begin(), knocked.end(),
                 [](auto a, auto b){
                     return a->knockOutOrder < b->knockOutOrder;
                 });
            for (size_t i=0; i<n; i++) {
                cout << knocked[i]->name << " " << (i+1) << "\n";
            }
        }

        cout << "Last Koopas knocked out:\n";
        if (!knocked.empty()) {
            sort(knocked.begin(), knocked.end(),
                 [](auto a, auto b){
                     return a->knockOutOrder > b->knockOutOrder;
                 });
            for (size_t i=0; i<n; i++) {
                cout << knocked[i]->name << " " << (n - i) << "\n";
            }
        }

        uint32_t endR = currentRound;
        vector<Koopa*> allK = allKoopas;
        sort(allK.begin(), allK.end(),
             [endR](auto a, auto b){
                 uint32_t aa = a->getActiveRounds(endR);
                 uint32_t bb = b->getActiveRounds(endR);
                 if (aa != bb) return aa > bb;
                 return a->name < b->name;
             });
        size_t tot = allK.size();
        size_t lim = min<size_t>(tot, statsCount);

        cout << "Most active Koopas:\n";
        for (size_t i=0; i<lim; i++) {
            cout << allK[i]->name << " "
                 << allK[i]->getActiveRounds(endR) << "\n";
        }

        sort(allK.begin(), allK.end(),
             [endR](auto a, auto b){
                 uint32_t aa = a->getActiveRounds(endR);
                 uint32_t bb = b->getActiveRounds(endR);
                 if (aa != bb) return aa < bb;
                 return a->name < b->name;
             });
        cout << "Least active Koopas:\n";
        for (size_t i=0; i<lim; i++) {
            cout << allK[i]->name << " "
                 << allK[i]->getActiveRounds(endR) << "\n";
        }
    }

    void runSimulation() {
        readHeader();
        readWaves();
        currentRound = 0;
        while (!gameOver) {
            currentRound++;
            if (verbose) {
                cout << "Round: " << currentRound << "\n";
            }
            moveKoopas();
            if (gameOver) break;

            if (currentWaveIndex < waveConfigs.size() &&
                currentRound == nextWaveNumber) {
                auto &cfg = waveConfigs[currentWaveIndex];
                spawnKoopas(cfg.randomKoopas, cfg.koopaLines);
                currentWaveIndex++;
                if (currentWaveIndex < waveConfigs.size()) {
                    nextWaveNumber = waveConfigs[currentWaveIndex].waveNumber;
                } else {
                    nextWaveNumber = numeric_limits<uint32_t>::max();
                }
            }
            throwRocks();
            if (gameOver) break;
            if (checkVictory()) break;
        }
    }
};

int main(int argc, char* argv[]){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    bool v=false, m=false;
    uint32_t s=0;

    static struct option longOpts[] = {
        {"verbose",    no_argument,       nullptr, 'v'},
        {"median",     no_argument,       nullptr, 'm'},
        {"statistics", required_argument, nullptr, 's'},
        {"help",       no_argument,       nullptr, 'h'},
        {nullptr, 0, nullptr, 0}
    };

    int opt, idx;
    while ((opt = getopt_long(argc, argv, "vms:h", longOpts, &idx)) != -1) {
        switch(opt) {
            case 'v': v=true; break;
            case 'm': m=true; break;
            case 's':
                s = static_cast<uint32_t>(strtoul(optarg, nullptr, 10));
                break;
            case 'h':
                cout << "Usage: ./mario_defense [--verbose|-v] [--median|-m]"
                     << " [--statistics N|-s N] [--help|-h]\n";
                return 0;
        }
    }
    MarioCastleDefense game(v, m, s);
    game.runSimulation();
    return 0;
}
