#pragma once
// 乱数ユーティリティ（ドロップ判定・個体値ロール・ダメージ揺らぎ）
#include <random>

namespace ag {

class Random {
public:
    static void Seed(unsigned int seed);
    static int Int(int minInclusive, int maxInclusive);
    static float Float(float minInclusive, float maxExclusive);
    static bool Chance(float probability01);

private:
    static std::mt19937& Engine();
};

} // namespace ag
