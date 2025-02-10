#ifndef KOOPARANDOMGENERATOR_H
#define KOOPARANDOMGENERATOR_H

#include <vector>
#include <string>
#include <cstdint>

class KoopaRandomGenerator {
public:
    static void initialize(uint32_t seed,
                           uint32_t maxDist,
                           uint32_t maxSpeed,
                           uint32_t maxHealth);

    static std::string getNextKoopaName();
    static uint32_t getNextKoopaDistance();
    static uint32_t getNextKoopaSpeed();
    static uint32_t getNextKoopaHealth();

private:
    enum class GenState : char {
        GenName,
        GenDistance,
        GenSpeed,
        GenHealth
    };

    static GenState genState;
    static uint32_t koopaCounter;
    static uint32_t maxRandDist,
                    maxRandSpeed,
                    maxRandHealth;
    static std::vector<std::string> KOOPA_NAMES;
    static uint32_t getNextInt(uint32_t);

    class MersenneTwister {
    public:
        MersenneTwister();
        ~MersenneTwister();

        MersenneTwister(const MersenneTwister&) = delete;
        MersenneTwister &operator=(const MersenneTwister&) = delete;

        void init_genrand(uint32_t s);
        uint32_t genrand_uint32_t();

    private:
        static const uint32_t N = 624;
        static const uint32_t M = 397;
        static const uint32_t MATRIX_A = 0x9908b0dfU;
        static const uint32_t UPPER_MASK = 0x80000000U;
        static const uint32_t LOWER_MASK = 0x7fffffffU;

        uint32_t *mt_;
        uint32_t mti_;
    };

    static MersenneTwister mt;
};

#endif
