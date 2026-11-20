#include "Player.h"
#include "Enemy.h"
#include "../World/World.h"
#include "../Core/Camera.h"
#include "../Core/Input.h"
#include "../Core/MathUtil.h"
#include "../Core/Random.h"
#include "../Core/AssetManager.h"
#include "../Core/FontManager.h"
#include "../Item/ItemDatabase.h"
#include <algorithm>
#include <cmath>

namespace ag {

namespace {
constexpr float kDodgeDuration = 0.35f;
constexpr float kDodgeDistance = 380.0f;
constexpr float kDodgeCooldown = 0.8f;
constexpr float kDodgeInvuln = 0.30f;
constexpr float kTurnSpeed = 14.0f;
constexpr float kComboWindow = 0.6f;
}

Player::Player()
{
    radius = 35.0f;
    height = 175.0f;
    PrimitiveLook l;
    l.shape = BodyShape::Humanoid;
    l.mainColor = 0x2E4C8C;
    l.subColor = 0x1E2433;
    l.skinColor = 0xF2CBA6;
    l.eyeColor = 0x1A2A50;
    InitVisual("player", l);
}

void Player::NewGame(const VECTOR& startPos)
{
    level = 1;
    exp = 0;
    gold = 50;
    inventory.Clear();
    equipment.Clear();
    cooldowns.clear();
    for (auto& slots : skillSlots) slots.fill("");
    atkBuffRate = atkBuffTime = defBuffRate = defBuffTime = 0.0f;

    ItemInstance sword = CreateItem("sword_training");
    sword.iv[static_cast<int>(IvStat::Atk)] = 50;
    equipment.Set(EquipSlot::Weapon, sword);
    equipment.Set(EquipSlot::Body, CreateItem("body_cloth"));
    inventory.Add(CreateItem("potion_s", 5));
    inventory.Add(CreateItem("ether_s", 2));

    FillDefaultSkillSlots();
    RecalcStats();
    hp = stats.maxHP;
    mp = stats.maxMP;
    Respawn(startPos);
}

void Player::FillDefaultSkillSlots()
{
    for (int w = 0; w < static_cast<int>(WeaponType::Count); ++w) {
        auto& slots = skillSlots[w];
        bool empty = true;
        for (auto& s : slots) if (!s.empty()) empty = false;
        if (!empty) continue;
        auto list = SkillDatabase::Get().ForWeapon(static_cast<WeaponType>(w));
        for (int i = 0; i < kSkillSlotCount && i < static_cast<int>(list.size()); ++i) slots[i] = list[i]->id;
    }
}

void Player::Respawn(const VECTOR& p)
{
    pos = p;
    yaw = 0.0f;
    action_ = Action::Free;
    activeSkill_ = nullptr;
    hp = std::max(hp, stats.maxHP);
    hp = stats.maxHP;
    mp = stats.maxMP;
    anim.Play(AnimState::Idle, 0.0f, true);
}

void Player::UpdateLook()
{
    const WeaponType w = equipment.Has(EquipSlot::Weapon) ? CurrentWeapon() : WeaponType::None;
    look.weapon = w;
    look.thrustStyle = (w == WeaponType::Rapier || w == WeaponType::Spear);
    look.shield = equipment.Has(EquipSlot::Shield) && !equipment.IsShieldDisabled();
    look.horns = false;
    // 頭装備があれば髪色（頭部）を鎧色に
    look.skinColor = 0xF2CBA6;
    look.mainColor = equipment.Has(EquipSlot::Body) && equipment.Get(EquipSlot::Body).defId != "body_cloth" ? 0x5A5F70 : 0x2E4C8C;
}

void Player::RecalcStats()
{
    const float hpRate = stats.maxHP > 0 ? static_cast<float>(hp) / stats.maxHP : 1.0f;
    const float mpRate = stats.maxMP > 0 ? static_cast<float>(mp) / stats.maxMP : 1.0f;

    Stats s = PlayerBaseStats(level);
    const StatBonus b = equipment.TotalBonus();
    const WeaponTypeInfo& wt = GetWeaponTypeInfo(CurrentWeapon());
    s.maxHP += b.maxHP;
    s.maxMP += b.maxMP;
    s.atk += b.atk;
    s.def += b.def;
    s.critRate += b.critRate + wt.critRateBonus;
    s.critDamage += b.critDamage + wt.critDamageBonus;
    s.moveSpeed += b.moveSpeed;
    if (atkBuffTime > 0.0f) s.atk = static_cast<int>(std::lround(s.atk * (1.0f + atkBuffRate)));
    if (defBuffTime > 0.0f) s.def = static_cast<int>(std::lround(s.def * (1.0f + defBuffRate)));
    s.critRate = Clamp(s.critRate, 0.0f, 1.0f);
    stats = s;

    if (hp > 0) hp = std::max(1, std::min(stats.maxHP, static_cast<int>(std::lround(hpRate * stats.maxHP))));
    mp = std::min(stats.maxMP, static_cast<int>(std::lround(mpRate * stats.maxMP)));
    UpdateLook();
}

int Player::AddExp(int amount)
{
    exp += amount;
    int gained = 0;
    while (exp >= ExpToNextLevel(level) && level < 100) {
        exp -= ExpToNextLevel(level);
        ++level;
        ++gained;
    }
    if (gained > 0) {
        RecalcStats();
        hp = stats.maxHP;
        mp = stats.maxMP;
    }
    return gained;
}

float Player::CooldownRemaining(const std::string& skillId) const
{
    auto it = cooldowns.find(skillId);
    return it == cooldowns.end() ? 0.0f : it->second;
}

const SkillDef* Player::SlotSkill(int slot) const
{
    if (slot < 0 || slot >= kSkillSlotCount) return nullptr;
    const std::string& id = skillSlots[static_cast<int>(CurrentWeapon())][slot];
    return id.empty() ? nullptr : SkillDatabase::Get().Find(id);
}

void Player::SetSlotSkill(int slot, const std::string& skillId)
{
    if (slot < 0 || slot >= kSkillSlotCount) return;
    auto& slots = skillSlots[static_cast<int>(CurrentWeapon())];
    for (auto& s : slots) if (s == skillId) s.clear(); // 重複登録を防ぐ
    slots[slot] = skillId;
}

// ---------------------------------------------------------------------------
// アイテム

bool Player::UseItem(int invIndex, EventSink& events)
{
    if (invIndex < 0 || invIndex >= inventory.Size() || IsDead()) return false;
    ItemInstance& it = inventory.At(invIndex);
    const ItemDef* d = it.Def();
    if (!d || d->category != ItemCategory::Consumable) return false;
    const ConsumableEffect& e = d->effect;
    if (e.healHP > 0 && hp >= stats.maxHP && e.healMP <= 0 && e.buffDuration <= 0.0f) {
        events.Warning("HP は満タンです");
        return false;
    }
    if (e.healHP > 0) {
        const int before = hp;
        hp = std::min(stats.maxHP, hp + e.healHP);
        GameEvent ev;
        ev.type = GameEvent::Type::PlayerHealed;
        ev.pos = VAdd(pos, VGet(0, Height(), 0));
        ev.amount = hp - before;
        events.Push(ev);
    }
    if (e.healMP > 0) mp = std::min(stats.maxMP, mp + e.healMP);
    if (e.atkBuffRate > 0.0f) { atkBuffRate = e.atkBuffRate; atkBuffTime = e.buffDuration; }
    if (e.defBuffRate > 0.0f) { defBuffRate = e.defBuffRate; defBuffTime = e.buffDuration; }
    events.Info(d->name + " を使用した");
    SoundManager::Get().PlaySE("use_item");
    inventory.RemoveAt(invIndex, 1);
    RecalcStats();
    return true;
}

bool Player::UseQuickItem(EventSink& events)
{
    const int idx = inventory.FindIndex(quickItemId);
    if (idx < 0) {
        const ItemDef* d = ItemDatabase::Get().Find(quickItemId);
        events.Warning((d ? d->name : quickItemId) + " を持っていない");
        return false;
    }
    return UseItem(idx, events);
}

bool Player::Equip(int invIndex, EventSink& events)
{
    if (invIndex < 0 || invIndex >= inventory.Size()) return false;
    if (action_ == Action::Skill || action_ == Action::Attack) return false;
    ItemInstance item = inventory.At(invIndex);
    const ItemDef* d = item.Def();
    if (!d || !d->IsEquipment()) return false;
    const EquipSlot slot = SlotForCategory(d->category);
    inventory.RemoveAt(invIndex, 1);
    if (equipment.Has(slot)) inventory.Add(equipment.Take(slot));
    equipment.Set(slot, item);
    events.Info(d->name + " を装備した");
    if (slot == EquipSlot::Shield && equipment.IsShieldDisabled()) events.Warning("両手武器を装備中のため盾は無効です");
    SoundManager::Get().PlaySE("equip");
    RecalcStats();
    return true;
}

bool Player::Unequip(EquipSlot slot, EventSink& events)
{
    if (!equipment.Has(slot)) return false;
    if (inventory.IsFull()) {
        events.Warning("所持品がいっぱいです");
        return false;
    }
    ItemInstance item = equipment.Take(slot);
    events.Info(item.Def()->name + " を外した");
    inventory.Add(item);
    RecalcStats();
    return true;
}

void Player::WearEquipment(EquipSlot slot, int amount)
{
    if (!equipment.Has(slot)) return;
    ItemInstance& it = equipment.Get(slot);
    if (it.maxDurability <= 0 || it.durability <= 0) return;
    const bool wasOk = !it.IsBroken();
    it.durability = std::max(0, it.durability - amount);
    if (wasOk && it.IsBroken()) RecalcStats();
}

// ---------------------------------------------------------------------------
// 戦闘

DamageResult Player::TakeDamage(const DamageResult& dmg)
{
    if (IsDead() || invulnTimer_ > 0.0f) return DamageResult{};
    DamageResult r = Character::TakeDamage(dmg);

    // 防具の耐久値を減らす（装備中の防具からランダムに1つ）
    EquipSlot armor[3];
    int n = 0;
    if (equipment.Has(EquipSlot::Head)) armor[n++] = EquipSlot::Head;
    if (equipment.Has(EquipSlot::Body)) armor[n++] = EquipSlot::Body;
    if (equipment.Has(EquipSlot::Shield) && !equipment.IsShieldDisabled()) armor[n++] = EquipSlot::Shield;
    if (n > 0) WearEquipment(armor[Random::Int(0, n - 1)], 1);

    SoundManager::Get().PlaySE("player_hit");
    if (IsDead()) {
        action_ = Action::Dead;
        activeSkill_ = nullptr;
        anim.Play(AnimState::Death, 1.4f, true);
    } else if (action_ != Action::Skill && r.amount >= stats.maxHP / 10) {
        // スキル発動中はスーパーアーマー。それ以外で大きな被弾は怯む
        action_ = Action::Stagger;
        actionTimer_ = 0.0f;
        anim.Play(AnimState::Hit, 0.3f, true);
    }
    return r;
}

void Player::FaceNearestEnemy(World& world, float range)
{
    if (Enemy* e = world.NearestEnemy(pos, range)) yaw = YawFromDir(VSub(e->pos, pos));
}

void Player::StartAttack(int step, World& world)
{
    const WeaponTypeInfo& wt = GetWeaponTypeInfo(CurrentWeapon());
    action_ = Action::Attack;
    actionTimer_ = 0.0f;
    comboStep_ = step;
    comboQueued_ = false;
    hitDone_ = false;
    FaceNearestEnemy(world, wt.range * 1.8f);
    const float dur = wt.attackDuration * (step == 2 ? 1.25f : 1.0f);
    anim.Play(AnimState::Attack, dur, true);
    SoundManager::Get().PlaySE("swing");
}

bool Player::TryStartSkill(int slot, World& world, EventSink& events)
{
    const SkillDef* s = SlotSkill(slot);
    if (!s) return false;
    if (!IsSkillUnlocked(*s)) {
        events.Warning(Format("%s は Lv%d で解放されます", s->name.c_str(), s->requiredLevel));
        return false;
    }
    if (CooldownRemaining(s->id) > 0.0f) {
        events.Warning(Format("%s はクールダウン中（残り %.1f 秒）", s->name.c_str(), CooldownRemaining(s->id)));
        return false;
    }
    if (mp < s->mpCost) {
        events.Warning("MP が足りない");
        return false;
    }
    mp -= s->mpCost;
    cooldowns[s->id] = s->cooldown;
    activeSkill_ = s;
    skillHitIndex_ = 0;
    skillSpin_ = 0.0f;
    action_ = Action::Skill;
    actionTimer_ = 0.0f;
    FaceNearestEnemy(world, s->range + s->lunge + 150.0f);
    anim.Play(AnimState::Skill, s->duration, true);
    SoundManager::Get().PlaySE("skill");
    return true;
}

void Player::HandleMovement(float dt, const Camera& cam, World& world, bool inputEnabled, float speedScale)
{
    Input& in = Input::Get();
    VECTOR move = VGet(0, 0, 0);
    if (inputEnabled) {
        if (in.ActionDown(ag::Action::MoveForward)) move = VAdd(move, cam.ForwardXZ());
        if (in.ActionDown(ag::Action::MoveBack))    move = VSub(move, cam.ForwardXZ());
        if (in.ActionDown(ag::Action::MoveRight))   move = VAdd(move, cam.RightXZ());
        if (in.ActionDown(ag::Action::MoveLeft))    move = VSub(move, cam.RightXZ());
    }
    const float len = VSize(move);
    if (len > 0.01f) {
        move = VScale(move, 1.0f / len);
        const bool dash = in.ActionDown(ag::Action::Dash) && action_ == Action::Free;
        const float speed = stats.moveSpeed * (dash ? 1.6f : 1.0f) * speedScale;
        pos = VAdd(pos, VScale(move, speed * dt));
        if (action_ == Action::Free) yaw = ApproachAngle(yaw, YawFromDir(move), kTurnSpeed * dt);
        world.ResolveCollision(pos, radius);
        if (action_ == Action::Free) anim.Play(AnimState::Move, dash ? 0.45f : 0.7f);
    } else if (action_ == Action::Free) {
        anim.Play(AnimState::Idle);
    }
}

void Player::Update(float dt, const Camera& cam, World& world, EventSink& events, bool inputEnabled)
{
    UpdateCommon(dt);
    Input& in = Input::Get();

    // クールダウン・バフ・無敵時間
    for (auto& cd : cooldowns) cd.second = std::max(0.0f, cd.second - dt);
    dodgeCd_ = std::max(0.0f, dodgeCd_ - dt);
    invulnTimer_ = std::max(0.0f, invulnTimer_ - dt);
    bool buffChanged = false;
    if (atkBuffTime > 0.0f) { atkBuffTime -= dt; if (atkBuffTime <= 0.0f) { atkBuffTime = 0.0f; buffChanged = true; } }
    if (defBuffTime > 0.0f) { defBuffTime -= dt; if (defBuffTime <= 0.0f) { defBuffTime = 0.0f; buffChanged = true; } }
    if (buffChanged) { RecalcStats(); events.Info("バフの効果が切れた"); }

    if (action_ == Action::Dead) return;

    // 自然回復（町の中では 10 倍）
    const float regenMul = inTown ? 10.0f : 1.0f;
    hpRegenAcc_ += stats.hpRegen * regenMul * dt;
    mpRegenAcc_ += stats.mpRegen * regenMul * dt;
    if (hpRegenAcc_ >= 1.0f) { const int a = static_cast<int>(hpRegenAcc_); hp = std::min(stats.maxHP, hp + a); hpRegenAcc_ -= a; }
    if (mpRegenAcc_ >= 1.0f) { const int a = static_cast<int>(mpRegenAcc_); mp = std::min(stats.maxMP, mp + a); mpRegenAcc_ -= a; }

    comboResetTimer_ = std::max(0.0f, comboResetTimer_ - dt);
    const bool attackPressed = inputEnabled && in.MousePressed(MouseButton::Left);

    // スキル入力（Free / 通常攻撃中ならキャンセルして発動）
    if (inputEnabled && (action_ == Action::Free || action_ == Action::Attack)) {
        const ag::Action keys[kSkillSlotCount] = { ag::Action::Skill1, ag::Action::Skill2, ag::Action::Skill3, ag::Action::Skill4 };
        for (int i = 0; i < kSkillSlotCount; ++i) {
            if (in.ActionPressed(keys[i]) && TryStartSkill(i, world, events)) break;
        }
    }
    // 回避
    if (inputEnabled && in.ActionPressed(ag::Action::Dodge) && dodgeCd_ <= 0.0f
        && (action_ == Action::Free || action_ == Action::Attack || action_ == Action::Stagger)) {
        VECTOR dir = VGet(0, 0, 0);
        if (in.ActionDown(ag::Action::MoveForward)) dir = VAdd(dir, cam.ForwardXZ());
        if (in.ActionDown(ag::Action::MoveBack))    dir = VSub(dir, cam.ForwardXZ());
        if (in.ActionDown(ag::Action::MoveRight))   dir = VAdd(dir, cam.RightXZ());
        if (in.ActionDown(ag::Action::MoveLeft))    dir = VSub(dir, cam.RightXZ());
        if (VSize(dir) < 0.01f) dir = VScale(DirFromYaw(yaw), -1.0f);
        dodgeDir_ = VNorm(dir);
        action_ = Action::Dodge;
        actionTimer_ = 0.0f;
        dodgeCd_ = kDodgeCooldown;
        invulnTimer_ = kDodgeInvuln;
        anim.Play(AnimState::Move, 0.35f, true);
        SoundManager::Get().PlaySE("dodge");
    }
    // クイックアイテム
    if (inputEnabled && in.ActionPressed(ag::Action::UseItem) && action_ != Action::Skill) UseQuickItem(events);

    actionTimer_ += dt;
    switch (action_) {
    case Action::Free:
        HandleMovement(dt, cam, world, inputEnabled, 1.0f);
        if (attackPressed) StartAttack(comboResetTimer_ > 0.0f ? (comboStep_ + 1) % 3 : 0, world);
        break;

    case Action::Attack: {
        const WeaponTypeInfo& wt = GetWeaponTypeInfo(CurrentWeapon());
        const float dur = wt.attackDuration * (comboStep_ == 2 ? 1.25f : 1.0f);
        HandleMovement(dt, cam, world, inputEnabled, 0.3f);
        if (attackPressed && actionTimer_ > dur * 0.35f) comboQueued_ = true;
        if (!hitDone_ && actionTimer_ >= dur * wt.hitTiming) {
            hitDone_ = true;
            const int hits = world.PlayerAttack(*this, wt.range, DegToRad(wt.arcDeg * 0.5f), wt.comboMultiplier[comboStep_], 0.0f, events);
            if (hits > 0) WearEquipment(EquipSlot::Weapon, 1);
        }
        if (actionTimer_ >= dur) {
            comboResetTimer_ = kComboWindow;
            if (comboQueued_ && comboStep_ < 2) StartAttack(comboStep_ + 1, world);
            else {
                action_ = Action::Free;
                if (comboStep_ >= 2) comboResetTimer_ = 0.0f;
            }
        }
        break;
    }

    case Action::Skill: {
        const SkillDef* s = activeSkill_;
        if (!s) { action_ = Action::Free; break; }
        const float t = actionTimer_ / s->duration;
        // モーション固定: 入力を無視して前進
        if (s->lunge > 0.0f && t < 0.7f) {
            pos = VAdd(pos, VScale(DirFromYaw(yaw), s->lunge * dt / (s->duration * 0.7f)));
            world.ResolveCollision(pos, radius);
        }
        if (s->arcDeg >= 359.0f && !look.thrustStyle) skillSpin_ = t * kTwoPi;
        while (skillHitIndex_ < s->hits.size() && t >= s->hits[skillHitIndex_].timing) {
            const int hits = world.PlayerAttack(*this, s->range, DegToRad(s->arcDeg * 0.5f), s->hits[skillHitIndex_].multiplier, s->extraCritRate, events);
            if (hits > 0) WearEquipment(EquipSlot::Weapon, 1);
            ++skillHitIndex_;
        }
        if (actionTimer_ >= s->duration) {
            action_ = Action::Free;
            activeSkill_ = nullptr;
            skillSpin_ = 0.0f;
        }
        break;
    }

    case Action::Dodge: {
        const float k = 1.0f - actionTimer_ / kDodgeDuration;
        pos = VAdd(pos, VScale(dodgeDir_, kDodgeDistance / kDodgeDuration * 1.6f * std::max(0.0f, k) * dt));
        world.ResolveCollision(pos, radius);
        yaw = ApproachAngle(yaw, YawFromDir(dodgeDir_), kTurnSpeed * dt);
        if (actionTimer_ >= kDodgeDuration) action_ = Action::Free;
        break;
    }

    case Action::Stagger:
        if (actionTimer_ >= 0.3f) action_ = Action::Free;
        break;

    case Action::Dead:
        break;
    }
}

void Player::Draw() const
{
    // 回避中は半透明で無敵を表現
    const float opacity = invulnTimer_ > 0.0f ? 0.55f : 1.0f;
    const unsigned int trail = (action_ == Action::Skill && activeSkill_) ? activeSkill_->effectColor : 0;
    DrawBody(opacity, skillSpin_, trail);
}

} // namespace ag
