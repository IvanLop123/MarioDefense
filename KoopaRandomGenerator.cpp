// KoopaRandomGenerator.cpp   Mersenne Twister code by T. Nishimura & M. Matsumoto.


#include <iostream>
#include "KoopaRandomGenerator.h"

KoopaRandomGenerator::MersenneTwister KoopaRandomGenerator::mt;

std::vector<std::string> KoopaRandomGenerator::KOOPA_NAMES = {
    "greenKoopa",
    "redKoopa",
    "spiny",
    "hammerBro",
    "dryBones",
    "paraTroopa",
};

KoopaRandomGenerator::GenState KoopaRandomGenerator::genState;
uint32_t KoopaRandomGenerator::koopaCounter;
uint32_t KoopaRandomGenerator::maxRandDist;
uint32_t KoopaRandomGenerator::maxRandSpeed;
uint32_t KoopaRandomGenerator::maxRandHealth;

void KoopaRandomGenerator::initialize(uint32_t seed,
                                      uint32_t maxDist,
                                      uint32_t maxSpeed,
                                      uint32_t maxHealth) {
    genState = GenState::GenName;
    koopaCounter = 0;
    maxRandDist = maxDist;
    maxRandSpeed = maxSpeed;
    maxRandHealth = maxHealth;
    mt.init_genrand(seed);
}

std::string KoopaRandomGenerator::getNextKoopaName() {
    if (genState != GenState::GenName) {
        std::cerr << "Koopa generator functions called out of order\n";
        exit(1);
    }
    genState = GenState::GenDistance;
    uint32_t idx = koopaCounter++;
    const auto &baseName = KOOPA_NAMES[idx % KOOPA_NAMES.size()];
    return baseName + std::to_string(idx);
}

uint32_t KoopaRandomGenerator::getNextKoopaDistance() {
    if (genState != GenState::GenDistance) {
        std::cerr << "Koopa generator functions called out of order\n";
        exit(1);
    }
    genState = GenState::GenSpeed;
    return getNextInt(maxRandDist);
}

uint32_t KoopaRandomGenerator::getNextKoopaSpeed() {
    if (genState != GenState::GenSpeed) {
        std::cerr << "Koopa generator functions called out of order\n";
        exit(1);
    }
    genState = GenState::GenHealth;
    return getNextInt(maxRandSpeed);
}

uint32_t KoopaRandomGenerator::getNextKoopaHealth() {
    if (genState != GenState::GenHealth) {
        std::cerr << "Koopa generator functions called out of order\n";
        exit(1);
    }
    genState = GenState::GenName;
    return getNextInt(maxRandHealth);
}

uint32_t KoopaRandomGenerator::getNextInt(uint32_t maxVal) {
    // Mersenne Twister usage: random in [1..maxVal]
    return (mt.genrand_uint32_t() % maxVal) + 1;
}

/* The Mersenne Twister code plus license, adapted from
   Takuji Nishimura and Makoto Matsumoto, is retained below:
*/

KoopaRandomGenerator::MersenneTwister::MersenneTwister()
  : mt_(new uint32_t[N]), mti_(N + 1) {
    init_genrand(0);
}

KoopaRandomGenerator::MersenneTwister::~MersenneTwister() {
    delete[] mt_;
    mt_ = nullptr;
}

void KoopaRandomGenerator::MersenneTwister::init_genrand(uint32_t s) {
    mt_[0] = s & 0xffffffffU;
    for (mti_ = 1; mti_ < N; mti_++) {
        mt_[mti_] = (1812433253U * (mt_[mti_ - 1] ^ (mt_[mti_ - 1] >> 30))
                      + static_cast<uint32_t>(mti_));
        mt_[mti_] &= 0xffffffffU;
    }
}

uint32_t KoopaRandomGenerator::MersenneTwister::genrand_uint32_t() {
    uint32_t y;
    static uint32_t mag01[2] = { 0x0U, MATRIX_A };
    if (mti_ >= N) {
        uint32_t kk;
        if (mti_ == N + 1)
            init_genrand(5489U);
        for (kk = 0; kk < N - M; kk++) {
            y = (mt_[kk] & UPPER_MASK) | (mt_[kk + 1] & LOWER_MASK);
            mt_[kk] = mt_[kk + M] ^ (y >> 1) ^ mag01[y & 1U];
        }
        for (; kk < N - 1; kk++) {
            y = (mt_[kk] & UPPER_MASK) | (mt_[kk + 1] & LOWER_MASK);
            mt_[kk] = mt_[kk + (M - N)] ^ (y >> 1) ^ mag01[y & 1U];
        }
        y = (mt_[N - 1] & UPPER_MASK) | (mt_[0] & LOWER_MASK);
        mt_[N - 1] = mt_[M - 1] ^ (y >> 1) ^ mag01[y & 1U];
        mti_ = 0;
    }
    y = mt_[mti_++];
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680U;
    y ^= (y << 15) & 0xefc60000U;
    y ^= (y >> 18);
    return y;
}
