#pragma once

#include <random>
#include <ctime>

class RandomGenerator {
public:
    static int GenerateUpTo(int roof) {
        std::srand(std::time(0));
        return std::rand()%(roof+1);
    }
private:
    RandomGenerator() {}
};