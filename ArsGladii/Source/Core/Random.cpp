#include "Random.h"

namespace ag {

std::mt19937& Random::Engine()
{
    static std::mt19937 engine{ std::random_device{}() };
    return engine;
}

void Random::Seed(unsigned int seed) { Engine().seed(seed); }

int Random::Int(int minInclusive, int maxInclusive)
{
    if (maxInclusive <= minInclusive) return minInclusive;
    std::uniform_int_distribution<int> dist(minInclusive, maxInclusive);
    return dist(Engine());
}

float Random::Float(float minInclusive, float maxExclusive)
{
    if (maxExclusive <= minInclusive) return minInclusive;
    std::uniform_real_distribution<float> dist(minInclusive, maxExclusive);
    return dist(Engine());
}

bool Random::Chance(float probability01)
{
    if (probability01 <= 0.0f) return false;
    if (probability01 >= 1.0f) return true;
    return Float(0.0f, 1.0f) < probability01;
}

} // namespace ag
