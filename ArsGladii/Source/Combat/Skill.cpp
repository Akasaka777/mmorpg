#include "Skill.h"

namespace ag {

SkillDatabase& SkillDatabase::Get()
{
    static SkillDatabase instance;
    return instance;
}

namespace {
SkillDef Make(const char* id, const char* name, const char* desc, WeaponType w, int lv, int mp, float cd,
              float dur, float range, float arc, float lunge, float crit, std::vector<SkillHit> hits, unsigned int color)
{
    SkillDef s;
    s.id = id; s.name = name; s.description = desc; s.weapon = w; s.requiredLevel = lv;
    s.mpCost = mp; s.cooldown = cd; s.duration = dur; s.range = range; s.arcDeg = arc;
    s.lunge = lunge; s.extraCritRate = crit; s.hits = std::move(hits); s.effectColor = color;
    return s;
}
}

SkillDatabase::SkillDatabase()
{
    using W = WeaponType;
    // --- 片手剣 ---
    skills_.push_back(Make("sword_soukou", "蒼閃斬", "踏み込みながら斜めに斬り下ろす一撃。",
        W::OneHandSword, 1, 8, 4.0f, 0.70f, 260, 120, 120, 0.05f, { { 0.45f, 2.2f } }, 0x55AAFF));
    skills_.push_back(Make("sword_senpu", "旋風刃", "その場で回転し、周囲を2回斬りつける。",
        W::OneHandSword, 1, 14, 8.0f, 0.90f, 240, 360, 0, 0.0f, { { 0.35f, 1.2f }, { 0.70f, 1.4f } }, 0x88FFCC));
    skills_.push_back(Make("sword_sanrenga", "三連牙", "前進しながら3連続で斬りつける。",
        W::OneHandSword, 3, 20, 12.0f, 1.20f, 250, 100, 220, 0.05f, { { 0.25f, 1.1f }, { 0.50f, 1.1f }, { 0.80f, 1.9f } }, 0xFFDD55));
    // --- 片手棍 ---
    skills_.push_back(Make("mace_hasai", "破砕撃", "地面ごと叩き潰す重撃。周囲にも衝撃が走る。",
        W::OneHandMace, 1, 12, 7.0f, 1.00f, 260, 360, 40, 0.0f, { { 0.60f, 2.8f } }, 0xFF8844));
    skills_.push_back(Make("mace_souda", "双打", "素早く2回打ち据える。",
        W::OneHandMace, 1, 8, 4.5f, 0.75f, 220, 110, 60, 0.05f, { { 0.35f, 1.3f }, { 0.75f, 1.5f } }, 0xFFAA66));
    // --- 短剣 ---
    skills_.push_back(Make("dagger_kagebashiri", "影走り", "一瞬で間合いを詰め、すれ違いざまに刺す。",
        W::Dagger, 1, 8, 5.0f, 0.50f, 200, 90, 380, 0.25f, { { 0.70f, 2.0f } }, 0xAA66FF));
    skills_.push_back(Make("dagger_midarezashi", "乱れ刺し", "目にも止まらぬ5連続の刺突。",
        W::Dagger, 1, 14, 8.0f, 0.90f, 180, 80, 40, 0.10f,
        { { 0.15f, 0.6f }, { 0.30f, 0.6f }, { 0.45f, 0.6f }, { 0.60f, 0.6f }, { 0.80f, 1.2f } }, 0xCC88FF));
    // --- 細剣 ---
    skills_.push_back(Make("rapier_ryusei", "流星突き", "閃光のごとき長距離の刺突。クリティカルが出やすい。",
        W::Rapier, 1, 10, 5.0f, 0.60f, 360, 40, 260, 0.30f, { { 0.55f, 2.4f } }, 0xFFFFAA));
    skills_.push_back(Make("rapier_kaben", "花弁連突", "花びらを散らすような4連続の突き。",
        W::Rapier, 1, 16, 9.0f, 1.00f, 290, 50, 80, 0.10f,
        { { 0.20f, 0.9f }, { 0.40f, 0.9f }, { 0.60f, 0.9f }, { 0.85f, 1.4f } }, 0xFFAADD));
    // --- 槍 ---
    skills_.push_back(Make("spear_senku", "穿空槍", "槍を大きく突き出し、遠くの敵を貫く。",
        W::Spear, 1, 10, 5.0f, 0.75f, 450, 40, 100, 0.05f, { { 0.55f, 2.3f } }, 0x66FFFF));
    skills_.push_back(Make("spear_engetsu", "円月薙ぎ", "槍を振り回し、周囲をまとめて薙ぎ払う。",
        W::Spear, 1, 16, 9.0f, 1.00f, 340, 360, 0, 0.0f, { { 0.40f, 1.3f }, { 0.75f, 1.5f } }, 0x88DDFF));
}

const SkillDef* SkillDatabase::Find(const std::string& id) const
{
    for (const auto& s : skills_) if (s.id == id) return &s;
    return nullptr;
}

std::vector<const SkillDef*> SkillDatabase::ForWeapon(WeaponType w) const
{
    std::vector<const SkillDef*> out;
    for (const auto& s : skills_) if (s.weapon == w) out.push_back(&s);
    return out;
}

} // namespace ag
