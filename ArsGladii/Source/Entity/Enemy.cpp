#include "Enemy.h"
#include "../Core/MathUtil.h"
#include "../Core/Random.h"
#include "../Core/AssetManager.h"
#include <algorithm>

namespace ag {

namespace {
constexpr float kSlamRadius = 450.0f;
constexpr float kChargeLength = 900.0f;
constexpr float kChargeWidth = 160.0f;
constexpr float kChargeSpeed = 1800.0f;
constexpr float kTurnSpeed = 6.0f;
}

Enemy::Enemy(const EnemyDef& def, const VECTOR& spawnPos, int spawnerIndex)
    : def_(&def), spawnPos_(spawnPos), spawnerIndex_(spawnerIndex)
{
    pos = spawnPos;
    yaw = Random::Float(-kPi, kPi);
    radius = def.radius;
    height = def.height;
    stats.maxHP = def.maxHP;
    stats.maxMP = 0;
    stats.atk = def.atk;
    stats.def = def.def;
    stats.critRate = def.critRate;
    stats.critDamage = 1.5f;
    stats.moveSpeed = def.moveSpeed;
    hp = def.maxHP;
    InitVisual(def.modelKey, def.look);
    timer_ = Random::Float(0.5f, 3.0f);
}

float Enemy::WindupTime() const
{
    float t = def_->attackWindup;
    if (IsBoss()) {
        if (move_ == Move::Slam) t = 1.2f;
        else if (move_ == Move::Charge) t = 0.9f;
        if (enraged_) t *= 0.75f;
    }
    return t;
}

DamageResult Enemy::TakeDamage(const DamageResult& dmg)
{
    if (ai_ == AI::Dead) return DamageResult{};
    DamageResult r = Character::TakeDamage(dmg);
    if (IsDead()) {
        ai_ = AI::Dead;
        deadTimer_ = 0.0f;
        telegraph_.active = false;
        anim.Play(AnimState::Death, 1.2f, true);
        return r;
    }
    // 雑魚は怯む（予備動作中は怯まない）。ボスはスーパーアーマー。
    if (!IsBoss() && ai_ != AI::Windup && ai_ != AI::Active) {
        ai_ = AI::Stagger;
        timer_ = 0.25f;
        anim.Play(AnimState::Hit, 0.25f, true);
    } else if (ai_ == AI::Idle || ai_ == AI::Wander || ai_ == AI::Return) {
        ai_ = AI::Chase;
    }
    return r;
}

void Enemy::MoveToward(const VECTOR& target, float speed, float dt)
{
    VECTOR d = VSub(target, pos);
    d.y = 0.0f;
    const float len = VSize(d);
    if (len < 1.0f) return;
    const float step = std::min(len, speed * dt);
    pos = VAdd(pos, VScale(d, step / len));
    yaw = ApproachAngle(yaw, YawFromDir(d), kTurnSpeed * dt);
}

void Enemy::ChooseAttack(float distToPlayer)
{
    if (!IsBoss()) {
        move_ = Move::Swing;
        return;
    }
    if (distToPlayer > 600.0f && chargeCd_ <= 0.0f) move_ = Move::Charge;
    else move_ = Random::Chance(enraged_ ? 0.45f : 0.35f) ? Move::Slam : Move::Swing;
}

void Enemy::StartWindup(Move m, const Character& player)
{
    move_ = m;
    ai_ = AI::Windup;
    timer_ = 0.0f;
    yaw = YawFromDir(VSub(player.pos, pos));
    telegraph_.active = true;
    telegraph_.origin = pos;
    telegraph_.yaw = yaw;
    telegraph_.progress = 0.0f;
    switch (m) {
    case Move::Swing:
        telegraph_.shape = Telegraph::Shape::Sector;
        telegraph_.radius = def_->attackRange + 20.0f;
        telegraph_.halfArc = DegToRad(def_->attackArcDeg * 0.5f);
        break;
    case Move::Slam:
        telegraph_.shape = Telegraph::Shape::Circle;
        telegraph_.radius = enraged_ ? kSlamRadius * 1.2f : kSlamRadius;
        break;
    case Move::Charge:
        telegraph_.shape = Telegraph::Shape::Line;
        telegraph_.radius = kChargeLength;
        telegraph_.width = kChargeWidth;
        chargeDir_ = DirFromYaw(yaw);
        break;
    }
    anim.Play(AnimState::Attack, WindupTime() + 0.35f, true);
}

void Enemy::ResolveHit(Character& player, EventSink& events, float multiplier)
{
    bool inside = false;
    switch (telegraph_.shape) {
    case Telegraph::Shape::Sector:
        inside = InArc(pos, yaw, player.pos, telegraph_.radius, telegraph_.halfArc, player.radius);
        break;
    case Telegraph::Shape::Circle:
        inside = DistXZ(pos, player.pos) <= telegraph_.radius + player.radius;
        break;
    case Telegraph::Shape::Line:
        inside = DistXZ(pos, player.pos) <= radius + player.radius + 20.0f;
        break;
    }
    if (!inside || player.IsDead()) return;

    DamageResult dmg = CalcDamage(stats, player.stats, def_->attackMultiplier * multiplier);
    DamageResult applied = player.TakeDamage(dmg);
    if (applied.amount <= 0) return;
    GameEvent e;
    e.type = GameEvent::Type::DamageToPlayer;
    e.pos = VAdd(player.pos, VGet(0, player.Height(), 0));
    e.amount = applied.amount;
    e.critical = applied.critical;
    events.Push(e);
}

void Enemy::Update(float dt, Character& player, bool playerTargetable, EventSink& events)
{
    UpdateCommon(dt);
    attackCd_ = std::max(0.0f, attackCd_ - dt);
    chargeCd_ = std::max(0.0f, chargeCd_ - dt);

    if (ai_ == AI::Dead) {
        deadTimer_ += dt;
        return;
    }
    if (IsEngaged()) engageTime_ += dt;

    // ボスの発狂（HP 50% 以下）
    if (IsBoss() && !enraged_ && HpRate() <= 0.5f) {
        enraged_ = true;
        GameEvent e;
        e.type = GameEvent::Type::BossEnraged;
        e.enemy = def_;
        e.text = def_->name + " が怒り狂っている！";
        events.Push(e);
    }

    const float distPlayer = DistXZ(pos, player.pos);
    const float distSpawn = DistXZ(pos, spawnPos_);
    const float speedMul = (IsBoss() && enraged_) ? 1.2f : 1.0f;

    switch (ai_) {
    case AI::Idle:
        anim.Play(AnimState::Idle);
        timer_ -= dt;
        if (playerTargetable && distPlayer < def_->aggroRange) {
            ai_ = AI::Chase;
            if (IsBoss()) {
                engageTime_ = 0.0f;
                GameEvent e;
                e.type = GameEvent::Type::BossEngaged;
                e.enemy = def_;
                events.Push(e);
            }
        } else if (timer_ <= 0.0f && !IsBoss()) {
            const float a = Random::Float(-kPi, kPi), r = Random::Float(100.0f, 350.0f);
            wanderTarget_ = VAdd(spawnPos_, VGet(std::sin(a) * r, 0.0f, std::cos(a) * r));
            ai_ = AI::Wander;
        }
        break;

    case AI::Wander:
        anim.Play(AnimState::Move);
        MoveToward(wanderTarget_, def_->moveSpeed * 0.4f, dt);
        if (playerTargetable && distPlayer < def_->aggroRange) ai_ = AI::Chase;
        else if (DistXZ(pos, wanderTarget_) < 20.0f) {
            ai_ = AI::Idle;
            timer_ = Random::Float(2.0f, 5.0f);
        }
        break;

    case AI::Chase:
        if (!playerTargetable || distSpawn > def_->leashRange) {
            ai_ = AI::Return;
            break;
        }
        if (distPlayer <= def_->attackRange * 0.85f + player.radius || (IsBoss() && distPlayer > 600.0f && chargeCd_ <= 0.0f)) {
            yaw = ApproachAngle(yaw, YawFromDir(VSub(player.pos, pos)), kTurnSpeed * dt);
            if (attackCd_ <= 0.0f) {
                ChooseAttack(distPlayer);
                StartWindup(move_, player);
            } else if (distPlayer <= def_->attackRange * 0.85f + player.radius) {
                anim.Play(AnimState::Idle);
            } else {
                anim.Play(AnimState::Move);
                MoveToward(player.pos, def_->moveSpeed * speedMul, dt);
            }
        } else {
            anim.Play(AnimState::Move);
            MoveToward(player.pos, def_->moveSpeed * speedMul, dt);
        }
        break;

    case AI::Windup: {
        timer_ += dt;
        const float wt = WindupTime();
        telegraph_.progress = Clamp(timer_ / wt, 0.0f, 1.0f);
        telegraph_.origin = pos;
        if (timer_ >= wt) {
            telegraph_.active = false;
            timer_ = 0.0f;
            if (move_ == Move::Charge) {
                ai_ = AI::Active;
                chargeTravel_ = 0.0f;
                chargeHit_ = false;
                anim.Play(AnimState::Skill, kChargeLength / kChargeSpeed + 0.1f, true);
            } else {
                const float mult = (move_ == Move::Slam) ? 1.8f : 1.0f;
                telegraph_.yaw = yaw;
                ResolveHit(player, events, mult);
                ai_ = AI::Recover;
                timer_ = def_->attackRecover;
            }
        }
        break;
    }

    case AI::Active: { // ボスの突進
        const float step = kChargeSpeed * dt;
        pos = VAdd(pos, VScale(chargeDir_, step));
        chargeTravel_ += step;
        if (!chargeHit_ && DistXZ(pos, player.pos) <= radius + player.radius + 20.0f) {
            chargeHit_ = true;
            telegraph_.shape = Telegraph::Shape::Line;
            ResolveHit(player, events, 1.4f);
        }
        if (chargeTravel_ >= kChargeLength || DistXZ(pos, spawnPos_) > def_->leashRange) {
            ai_ = AI::Recover;
            timer_ = def_->attackRecover + 0.3f;
            chargeCd_ = enraged_ ? 6.0f : 9.0f;
        }
        break;
    }

    case AI::Recover:
        timer_ -= dt;
        if (anim.Progress() >= 1.0f) anim.Play(AnimState::Idle);
        if (timer_ <= 0.0f) {
            attackCd_ = def_->attackCooldown * ((IsBoss() && enraged_) ? 0.7f : 1.0f);
            ai_ = AI::Chase;
        }
        break;

    case AI::Stagger:
        timer_ -= dt;
        if (timer_ <= 0.0f) ai_ = AI::Chase;
        break;

    case AI::Return:
        anim.Play(AnimState::Move);
        MoveToward(spawnPos_, def_->moveSpeed * 1.3f, dt);
        hp = std::min(stats.maxHP, hp + static_cast<int>(stats.maxHP * 0.5f * dt) + 1);
        if (distSpawn < 30.0f) {
            hp = stats.maxHP;
            enraged_ = false;
            ai_ = AI::Idle;
            timer_ = Random::Float(1.0f, 3.0f);
        }
        break;

    case AI::Dead:
        break;
    }
}

void Enemy::Draw() const
{
    float opacity = 1.0f;
    if (ai_ == AI::Dead) opacity = Clamp(1.0f - (deadTimer_ - 1.8f) / 1.2f, 0.0f, 1.0f);
    DrawBody(opacity);
}

void Enemy::DrawTelegraph() const
{
    if (!telegraph_.active) return;
    const unsigned int red = 0xFF3020;
    const int alpha = 60 + static_cast<int>(telegraph_.progress * 90);
    switch (telegraph_.shape) {
    case Telegraph::Shape::Sector:
        PrimitiveRenderer::DrawGroundSector(telegraph_.origin, telegraph_.yaw, telegraph_.radius, telegraph_.halfArc, red, 50);
        PrimitiveRenderer::DrawGroundSector(telegraph_.origin, telegraph_.yaw, telegraph_.radius * telegraph_.progress, telegraph_.halfArc, red, alpha);
        break;
    case Telegraph::Shape::Circle:
        PrimitiveRenderer::DrawGroundCircle(telegraph_.origin, telegraph_.radius, red, 50, 48);
        PrimitiveRenderer::DrawGroundCircle(telegraph_.origin, telegraph_.radius * telegraph_.progress, red, alpha, 48);
        break;
    case Telegraph::Shape::Line: {
        // 長方形を三角形2枚で描く
        const VECTOR f = DirFromYaw(telegraph_.yaw);
        const VECTOR r = VGet(f.z, 0.0f, -f.x);
        const float hw = telegraph_.width * 0.5f;
        const float len = telegraph_.radius * telegraph_.progress;
        auto quad = [&](float l, int a) {
            const VECTOR o = VGet(telegraph_.origin.x, 2.0f, telegraph_.origin.z);
            const VECTOR p0 = VAdd(o, VScale(r, -hw));
            const VECTOR p1 = VAdd(o, VScale(r, hw));
            const VECTOR p2 = VAdd(p1, VScale(f, l));
            const VECTOR p3 = VAdd(p0, VScale(f, l));
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, a);
            SetUseLighting(FALSE);
            SetWriteZBuffer3D(FALSE);
            DrawTriangle3D(p0, p1, p2, PrimitiveRenderer::ToDx(red), TRUE);
            DrawTriangle3D(p0, p2, p3, PrimitiveRenderer::ToDx(red), TRUE);
            SetWriteZBuffer3D(TRUE);
            SetUseLighting(TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        };
        quad(telegraph_.radius, 50);
        quad(len, alpha);
        break;
    }
    }
}

} // namespace ag
