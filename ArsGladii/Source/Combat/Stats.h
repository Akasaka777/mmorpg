#pragma once
// ステータス定義とダメージ計算。設計の詳細は docs/GameDesign.md を参照。

namespace ag {

struct Stats {
    int   maxHP = 100;
    int   maxMP = 50;
    int   atk = 10;          // 攻撃力
    int   def = 5;           // 防御力
    float critRate = 0.05f;  // クリティカル率 (0～1)
    float critDamage = 1.5f; // クリティカル倍率
    float moveSpeed = 480.f; // 移動速度（ワールド単位/秒）
    float hpRegen = 0.0f;    // 毎秒回復（町の中で加算など）
    float mpRegen = 1.5f;

    Stats& operator+=(const Stats& o);
};

// 装備などが加算するボーナス値（すべて加算方式）
struct StatBonus {
    int   maxHP = 0;
    int   maxMP = 0;
    int   atk = 0;
    int   def = 0;
    float critRate = 0.0f;
    float critDamage = 0.0f;
    float moveSpeed = 0.0f;

    StatBonus& operator+=(const StatBonus& o);
    StatBonus Scaled(float s) const;
};

struct DamageResult {
    int  amount = 0;
    bool critical = false;
    bool guarded = false;  // ガードで軽減された
    bool parried = false;  // パリィ成功（ダメージ 0）
};

// 基本ダメージ式:
//   base   = ATK × 技倍率
//   軽減   = base × 100 / (100 + DEF)
//   揺らぎ = ×0.9～1.1、クリティカル時 ×critDamage、最低 1
DamageResult CalcDamage(const Stats& attacker, const Stats& defender, float multiplier, float extraCritRate = 0.0f);

// レベルから基礎ステータスを算出（プレイヤー用）
Stats PlayerBaseStats(int level);
int ExpToNextLevel(int level);

} // namespace ag
