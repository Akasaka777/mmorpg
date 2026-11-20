#pragma once
// 武器種の定義。新しい武器種は enum と WeaponTypeInfo テーブル（WeaponType.cpp）に追加する。

namespace ag {

enum class WeaponType {
    OneHandSword,  // 片手剣: 標準的なバランス
    OneHandMace,   // 片手棍: 遅いが重く、クリティカル倍率が高い
    Dagger,        // 短剣: 射程は短いが最速、クリティカル率が高い
    Rapier,        // 細剣: 前方への鋭い刺突、射程長め・扇形狭め
    Spear,         // 槍: 最長射程、両手持ち（盾は無効）
    Count,
    None = -1
};

struct WeaponTypeInfo {
    const char* key;          // セーブ・モデルキー用
    const char* name;         // 表示名
    float range;              // 通常攻撃の射程
    float arcDeg;             // 通常攻撃の扇形（全角）
    float attackDuration;     // 通常攻撃1段の時間（秒）
    float hitTiming;          // 攻撃判定が出るタイミング（0～1）
    float comboMultiplier[3]; // 3段コンボの倍率
    float critRateBonus;      // 武器種固有クリティカル率補正
    float critDamageBonus;    // 武器種固有クリティカル倍率補正
    bool  twoHanded;          // 両手持ち（盾不可）
};

const WeaponTypeInfo& GetWeaponTypeInfo(WeaponType t);
WeaponType WeaponTypeFromKey(const char* key);

} // namespace ag
