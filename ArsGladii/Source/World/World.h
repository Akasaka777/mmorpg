#pragma once
// 1つの層のワールド（フィールド・敵・湧き管理・戦闘判定）
#include "Field.h"
#include "GameEvent.h"
#include "../Entity/Enemy.h"
#include <memory>
#include <vector>

namespace ag {

class Player;

class World {
public:
    void Load(int floorNumber);
    void Update(float dt, Player& player, EventSink& events);
    void Draw(const VECTOR& cameraPos) const;
    void DrawOverlays(const VECTOR& cameraPos) const; // 敵の頭上HPバー・名前など（2D）

    // プレイヤーの攻撃判定。命中数を返す。
    int PlayerAttack(Player& player, float range, float halfArc, float multiplier, float extraCrit, EventSink& events);
    Enemy* NearestEnemy(const VECTOR& from, float maxDist) const;
    void ResolveCollision(VECTOR& pos, float radius) const { field_.ResolveCollision(pos, radius); }

    const Field& GetField() const { return field_; }
    const FloorDef& Floor() const { return field_.Def(); }
    const Enemy* ActiveBoss() const; // 戦闘中のボス（HPバー表示用）
    const std::vector<std::unique_ptr<Enemy>>& Enemies() const { return enemies_; }
    float BossRespawnRemaining() const { return bossRespawnTimer_; }

private:
    struct Spawner {
        const SpawnZone* zone = nullptr;
        int alive = 0;
        std::vector<float> timers; // 再出現待ち
    };
    void SpawnOne(int spawnerIndex);
    void SpawnBoss();
    void OnEnemyKilled(Enemy& e, EventSink& events);
    void SeparateCharacters(Player& player);

    Field field_;
    std::vector<std::unique_ptr<Enemy>> enemies_;
    std::vector<Spawner> spawners_;
    float bossRespawnTimer_ = 0.0f;
    bool bossAlive_ = false;
};

} // namespace ag
