#include "EnemyDatabase.h"

namespace ag {

EnemyDatabase& EnemyDatabase::Get()
{
    static EnemyDatabase instance;
    return instance;
}

EnemyDatabase::EnemyDatabase()
{
    // ---- 草原ウルフ ----
    {
        EnemyDef d;
        d.id = "plains_wolf"; d.name = "草原ウルフ"; d.modelKey = "wolf";
        d.look.shape = BodyShape::Beast; d.look.mainColor = 0x8A8F99; d.look.subColor = 0x5E626B; d.look.eyeColor = 0xFFCC33;
        d.level = 2; d.maxHP = 120; d.atk = 18; d.def = 4; d.moveSpeed = 400.0f;
        d.radius = 45.0f; d.height = 100.0f; d.attackRange = 150.0f; d.attackWindup = 0.45f; d.attackCooldown = 1.8f;
        d.exp = 15; d.goldMin = 3; d.goldMax = 8;
        d.drops.entries = { { "wolf_pelt", 0.60f, 1, 2 }, { "wolf_fang", 0.15f, 1, 1 },
                            { "potion_s", 0.12f, 1, 1 }, { "ether_s", 0.06f, 1, 1 } };
        defs_.push_back(d);
    }
    // ---- 岩角イノシシ ----
    {
        EnemyDef d;
        d.id = "rock_boar"; d.name = "岩角イノシシ"; d.modelKey = "boar";
        d.look.shape = BodyShape::Beast; d.look.mainColor = 0x7A5A3C; d.look.subColor = 0x4A3524; d.look.horns = true;
        d.look.scale = 1.15f; d.look.eyeColor = 0xFF5533;
        d.level = 3; d.maxHP = 200; d.atk = 24; d.def = 8; d.moveSpeed = 340.0f;
        d.radius = 52.0f; d.height = 110.0f; d.aggroRange = 700.0f; d.attackRange = 160.0f;
        d.attackWindup = 0.6f; d.attackCooldown = 2.2f; d.attackMultiplier = 1.2f;
        d.exp = 22; d.goldMin = 5; d.goldMax = 10;
        d.drops.entries = { { "boar_hide", 0.55f, 1, 2 }, { "iron_scrap", 0.20f, 1, 1 },
                            { "potion_s", 0.15f, 1, 1 }, { "pill_def", 0.04f, 1, 1 } };
        defs_.push_back(d);
    }
    // ---- ゴブリン兵 ----
    {
        EnemyDef d;
        d.id = "goblin"; d.name = "ゴブリン兵"; d.modelKey = "goblin";
        d.look.shape = BodyShape::Humanoid; d.look.mainColor = 0x6B4F2E; d.look.subColor = 0x3E2E1C;
        d.look.skinColor = 0x6FA84A; d.look.eyeColor = 0xFF2222; d.look.scale = 0.8f; d.look.weapon = WeaponType::Dagger;
        d.level = 4; d.maxHP = 180; d.atk = 28; d.def = 6; d.moveSpeed = 420.0f;
        d.radius = 32.0f; d.height = 175.0f; d.attackRange = 140.0f; d.attackWindup = 0.40f; d.attackCooldown = 1.5f;
        d.exp = 30; d.goldMin = 6; d.goldMax = 14;
        d.drops.entries = { { "goblin_cloth", 0.60f, 1, 2 }, { "iron_scrap", 0.30f, 1, 2 },
                            { "potion_s", 0.15f, 1, 1 }, { "pill_atk", 0.04f, 1, 1 } };
        defs_.push_back(d);
    }
    // ---- 第1層ボス: 灰牙の獣王 グラウゼル ----
    {
        EnemyDef d;
        d.id = "boss_graymane"; d.name = "グラウゼル"; d.title = "灰牙の獣王"; d.modelKey = "boss_floor1";
        d.look.shape = BodyShape::Humanoid; d.look.mainColor = 0x4A4E58; d.look.subColor = 0x2A2C33;
        d.look.skinColor = 0x9AA0AA; d.look.eyeColor = 0xFF3300; d.look.scale = 2.2f; d.look.horns = true;
        d.look.weapon = WeaponType::OneHandMace; d.look.shield = true;
        d.level = 8; d.maxHP = 2500; d.atk = 55; d.def = 15; d.critRate = 0.05f; d.moveSpeed = 360.0f;
        d.radius = 80.0f; d.height = 175.0f; d.aggroRange = 1300.0f; d.leashRange = 2200.0f;
        d.attackRange = 300.0f; d.attackArcDeg = 120.0f; d.attackWindup = 0.7f; d.attackRecover = 0.6f;
        d.attackCooldown = 1.4f; d.attackMultiplier = 1.0f;
        d.exp = 600; d.goldMin = 300; d.goldMax = 500; d.isBoss = true;
        d.drops.entries = { { "graymane_core", 1.0f, 1, 1 }, { "potion_m", 1.0f, 1, 2 }, { "wolf_fang", 1.0f, 2, 4 } };
        d.drops.equipmentPool = {
            { "sword_graymane", 10 }, { "mace_graymane", 10 }, { "dagger_graymane", 10 }, { "rapier_graymane", 10 },
            { "spear_graymane", 10 }, { "head_beastking", 10 }, { "body_beastking", 10 }, { "shield_beastking", 10 },
            { "sword_graymane_ex", 2 },
        };
        d.drops.poolRolls = 1;
        d.drops.bonusRollChance = 0.30f;
        d.drops.luck = 1;
        defs_.push_back(d);
    }
}

const EnemyDef* EnemyDatabase::Find(const std::string& id) const
{
    for (const auto& d : defs_) if (d.id == id) return &d;
    return nullptr;
}

} // namespace ag
