#pragma once
// プレイヤー・敵の共通基底
#include "Animator.h"
#include "PrimitiveRenderer.h"
#include "../Combat/Stats.h"
#include <string>

namespace ag {

class Character {
public:
    virtual ~Character() = default;

    // 被ダメージ。回避中などで無効化された場合は amount=0 を返す。
    virtual DamageResult TakeDamage(const DamageResult& dmg);

    bool IsDead() const { return hp <= 0; }
    float HpRate() const { return stats.maxHP > 0 ? static_cast<float>(hp) / stats.maxHP : 0.0f; }
    float MpRate() const { return stats.maxMP > 0 ? static_cast<float>(mp) / stats.maxMP : 0.0f; }
    float Height() const { return height * look.scale; }

    void InitVisual(const std::string& modelKey, const PrimitiveLook& primitiveLook);
    // モデルがあればモデル、無ければプリミティブで描画
    void DrawBody(float opacity = 1.0f, float spin = 0.0f, unsigned int trailColor = 0) const;

    VECTOR pos = VGet(0, 0, 0);
    float yaw = 0.0f;
    float radius = 35.0f;
    float height = 175.0f;
    int hp = 100;
    int mp = 0;
    Stats stats;

    Animator anim;
    PrimitiveLook look;
    float hitFlash = 0.0f;

protected:
    void UpdateCommon(float dt);
};

} // namespace ag
