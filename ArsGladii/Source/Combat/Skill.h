#pragma once
// ソードスキル（モーション固定・クールダウン制）
// 発動中はプレイヤーの移動入力を受け付けず、定義された前進量・判定タイミングで攻撃する。
#include "WeaponType.h"
#include <string>
#include <vector>

namespace ag {

struct SkillHit {
    float timing;      // 0～1（スキル時間に対する割合）
    float multiplier;  // ダメージ倍率
};

struct SkillDef {
    std::string id;
    std::string name;
    std::string description;
    WeaponType weapon = WeaponType::OneHandSword;
    int   requiredLevel = 1;
    int   mpCost = 10;
    float cooldown = 5.0f;     // 秒
    float duration = 0.8f;     // モーション固定時間（秒）
    float range = 250.0f;
    float arcDeg = 120.0f;     // 360 で全周
    float lunge = 0.0f;        // 発動中に前進する距離
    float extraCritRate = 0.0f;
    std::vector<SkillHit> hits;
    unsigned int effectColor = 0x66CCFF; // 0xRRGGBB
};

class SkillDatabase {
public:
    static SkillDatabase& Get();
    const SkillDef* Find(const std::string& id) const;
    std::vector<const SkillDef*> ForWeapon(WeaponType w) const;
    const std::vector<SkillDef>& All() const { return skills_; }

private:
    SkillDatabase();
    std::vector<SkillDef> skills_;
};

constexpr int kSkillSlotCount = 4;

} // namespace ag
