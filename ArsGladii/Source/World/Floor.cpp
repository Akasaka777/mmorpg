#include "Floor.h"

namespace ag {

FloorDatabase& FloorDatabase::Get()
{
    static FloorDatabase instance;
    return instance;
}

FloorDatabase::FloorDatabase()
{
    // ---- 第1層 ----
    FloorDef f;
    f.number = 1;
    f.name = "第1層 灰風の草原";
    f.townName = "はじまりの町 アルカ";
    f.halfSize = 8000.0f;
    f.townCenter = VGet(0, 0, -6000);
    f.townRadius = 1300.0f;
    f.playerStart = VGet(0, 0, -5600);
    f.zones = {
        { "plains_wolf", VGet(-2500, 0, -3000), 900.0f, 5, 15.0f },
        { "plains_wolf", VGet(2600, 0, -2600), 900.0f, 5, 15.0f },
        { "rock_boar",   VGet(-3500, 0, 0),    1000.0f, 4, 20.0f },
        { "rock_boar",   VGet(3200, 0, 600),   900.0f, 4, 20.0f },
        { "goblin",      VGet(-1800, 0, 2800), 900.0f, 5, 20.0f },
        { "goblin",      VGet(2000, 0, 3200),  900.0f, 5, 20.0f },
    };
    f.bossId = "boss_graymane";
    f.bossPos = VGet(0, 0, 6200);
    f.bossArenaRadius = 1500.0f;
    f.seed = 20261120;
    floors_.push_back(f);

    // 第2層以降はここに追加していく（町・湧き・ボスを定義するだけで遊べる構造）
}

const FloorDef* FloorDatabase::Find(int number) const
{
    for (const auto& f : floors_) if (f.number == number) return &f;
    return nullptr;
}

} // namespace ag
