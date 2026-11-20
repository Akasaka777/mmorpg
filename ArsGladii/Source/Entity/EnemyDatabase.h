#pragma once
// 敵のマスターデータ
#include "PrimitiveRenderer.h"
#include "../Item/DropTable.h"
#include <string>
#include <vector>

namespace ag {

struct EnemyDef {
    std::string id;
    std::string name;
    std::string title;          // ボスの二つ名
    std::string modelKey;       // Assets/Models/<modelKey>/
    PrimitiveLook look;
    int   level = 1;
    int   maxHP = 100;
    int   atk = 10;
    int   def = 5;
    float critRate = 0.03f;
    float moveSpeed = 350.0f;
    float radius = 40.0f;
    float height = 120.0f;
    float aggroRange = 900.0f;   // 索敵距離
    float leashRange = 1800.0f;  // 出現地点からこれ以上離れると戻る
    float attackRange = 160.0f;
    float attackArcDeg = 90.0f;
    float attackWindup = 0.5f;   // 予備動作（この間に回避できる）
    float attackRecover = 0.4f;
    float attackCooldown = 1.6f;
    float attackMultiplier = 1.0f;
    int   exp = 10;
    int   goldMin = 1;
    int   goldMax = 5;
    bool  isBoss = false;
    DropTable drops;
};

class EnemyDatabase {
public:
    static EnemyDatabase& Get();
    const EnemyDef* Find(const std::string& id) const;

private:
    EnemyDatabase();
    std::vector<EnemyDef> defs_;
};

} // namespace ag
