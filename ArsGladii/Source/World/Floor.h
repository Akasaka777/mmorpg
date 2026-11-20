#pragma once
// 層（フロア）の定義。1～100 層をデータとして追加していく想定。
// 各層: 町（安全地帯・将来の冒険者ギルド）／フィールドの湧き地点／ボス部屋
#include "DxLib.h"
#include <string>
#include <vector>

namespace ag {

struct SpawnZone {
    std::string enemyId;
    VECTOR center = VGet(0, 0, 0);
    float radius = 600.0f;
    int count = 3;
    float respawnTime = 15.0f;
};

struct FloorDef {
    int number = 1;
    std::string name;
    std::string townName;
    float halfSize = 8000.0f;          // フィールドの半径（正方形 -halfSize～+halfSize）
    VECTOR playerStart = VGet(0, 0, 0);
    VECTOR townCenter = VGet(0, 0, 0);
    float townRadius = 1200.0f;
    std::vector<SpawnZone> zones;
    std::string bossId;
    VECTOR bossPos = VGet(0, 0, 0);
    float bossArenaRadius = 1500.0f;
    float bossRespawnTime = 60.0f;     // プロトタイプ: ドロップ検証のため再出現させる
    unsigned int groundColorA = 0x5C8A3A;
    unsigned int groundColorB = 0x4E7A30;
    unsigned int pathColor = 0x9C8460;
    unsigned int skyColor = 0x8EC5F0;
    unsigned int seed = 1;
    int treeCount = 140;
    int rockCount = 60;
};

class FloorDatabase {
public:
    static FloorDatabase& Get();
    const FloorDef* Find(int number) const;
    int MaxFloor() const { return static_cast<int>(floors_.size()); }

private:
    FloorDatabase();
    std::vector<FloorDef> floors_;
};

} // namespace ag
