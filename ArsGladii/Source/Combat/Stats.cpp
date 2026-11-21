#include "Stats.h"
#include "../Core/Random.h"
#include <algorithm>
#include <cmath>

namespace ag {

Stats& Stats::operator+=(const Stats& o)
{
    maxHP += o.maxHP;
    maxMP += o.maxMP;
    atk += o.atk;
    def += o.def;
    critRate += o.critRate;
    critDamage += o.critDamage;
    moveSpeed += o.moveSpeed;
    hpRegen += o.hpRegen;
    mpRegen += o.mpRegen;
    return *this;
}

StatBonus& StatBonus::operator+=(const StatBonus& o)
{
    maxHP += o.maxHP;
    maxMP += o.maxMP;
    atk += o.atk;
    def += o.def;
    critRate += o.critRate;
    critDamage += o.critDamage;
    moveSpeed += o.moveSpeed;
    return *this;
}

StatBonus StatBonus::Scaled(float s) const
{
    StatBonus b;
    b.maxHP = static_cast<int>(std::lround(maxHP * s));
    b.maxMP = static_cast<int>(std::lround(maxMP * s));
    b.atk = static_cast<int>(std::lround(atk * s));
    b.def = static_cast<int>(std::lround(def * s));
    b.critRate = critRate * s;
    b.critDamage = critDamage * s;
    b.moveSpeed = moveSpeed * s;
    return b;
}

DamageResult CalcDamage(const Stats& attacker, const Stats& defender, float multiplier, float extraCritRate)
{
    DamageResult r;
    float base = attacker.atk * multiplier;
    float reduced = base * 100.0f / (100.0f + std::max(0, defender.def));
    reduced *= Random::Float(0.9f, 1.1f);
    r.critical = Random::Chance(attacker.critRate + extraCritRate);
    if (r.critical) reduced *= attacker.critDamage;
    r.amount = std::max(1, static_cast<int>(std::lround(reduced)));
    return r;
}

Stats PlayerBaseStats(int level)
{
    Stats s;
    const int lv = std::max(1, level) - 1;
    s.maxHP = 200 + lv * 30;
    s.maxMP = 60 + lv * 5;
    s.atk = 12 + lv * 3;
    s.def = 5 + lv * 2;
    s.critRate = 0.05f;
    s.critDamage = 1.5f;
    s.moveSpeed = 480.0f;
    s.hpRegen = 0.5f + lv * 0.1f;
    s.mpRegen = 2.0f + lv * 0.1f;
    return s;
}

int ExpToNextLevel(int level)
{
    // Lv1→2: 100, 以降ゆるやかに増加
    const int lv = std::max(1, level);
    return static_cast<int>(100.0 * std::pow(lv, 1.6));
}

} // namespace ag
