#pragma once
// 敵（雑魚・ボス共通）。AI は状態機械で、ボスは複数の攻撃パターンを持つ。
#include "Character.h"
#include "EnemyDatabase.h"
#include "../World/GameEvent.h"

namespace ag {

struct Telegraph {
    enum class Shape { Sector, Circle, Line };
    bool active = false;
    Shape shape = Shape::Sector;
    VECTOR origin = VGet(0, 0, 0);
    float yaw = 0.0f;
    float radius = 0.0f;   // Sector/Circle: 半径, Line: 長さ
    float halfArc = 0.0f;  // Sector
    float width = 0.0f;    // Line
    float progress = 0.0f; // 0～1（発動までの割合）
};

class Enemy : public Character {
public:
    enum class AI { Idle, Wander, Chase, Windup, Active, Recover, Return, Stagger, Dead };
    enum class Move { Swing, Slam, Charge };

    Enemy(const EnemyDef& def, const VECTOR& spawnPos, int spawnerIndex);

    // playerTargetable: プレイヤーが町の中・死亡中なら false
    void Update(float dt, Character& player, bool playerTargetable, EventSink& events);
    void Draw() const;
    void DrawTelegraph() const;

    DamageResult TakeDamage(const DamageResult& dmg) override;

    const EnemyDef& Def() const { return *def_; }
    int SpawnerIndex() const { return spawnerIndex_; }
    bool IsBoss() const { return def_->isBoss; }
    bool IsEngaged() const { return ai_ == AI::Chase || ai_ == AI::Windup || ai_ == AI::Active || ai_ == AI::Recover || ai_ == AI::Stagger; }
    bool ReadyToRemove() const { return ai_ == AI::Dead && deadTimer_ > 3.0f; }
    bool RewardGiven() const { return rewardGiven_; }
    void MarkRewardGiven() { rewardGiven_ = true; }
    bool IsEnraged() const { return enraged_; }
    float EngageTime() const { return engageTime_; }

private:
    void ChooseAttack(float distToPlayer);
    void StartWindup(Move m, const Character& player);
    void ResolveHit(Character& player, EventSink& events, float multiplier);
    void MoveToward(const VECTOR& target, float speed, float dt);
    float WindupTime() const;

    const EnemyDef* def_;
    VECTOR spawnPos_;
    int spawnerIndex_;
    AI ai_ = AI::Idle;
    Move move_ = Move::Swing;
    float timer_ = 0.0f;
    float attackCd_ = 0.0f;
    float chargeCd_ = 0.0f;
    float deadTimer_ = 0.0f;
    float engageTime_ = 0.0f;
    VECTOR wanderTarget_ = VGet(0, 0, 0);
    VECTOR chargeDir_ = VGet(0, 0, 1);
    float chargeTravel_ = 0.0f;
    bool chargeHit_ = false;
    bool enraged_ = false;
    bool rewardGiven_ = false;
    Telegraph telegraph_;
};

} // namespace ag
