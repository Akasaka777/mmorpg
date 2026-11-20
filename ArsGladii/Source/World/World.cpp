#include "World.h"
#include "../Entity/Player.h"
#include "../Core/MathUtil.h"
#include "../Core/Random.h"
#include "../Core/FontManager.h"
#include "../Core/AssetManager.h"
#include <algorithm>

namespace ag {

void World::Load(int floorNumber)
{
    const FloorDef* def = FloorDatabase::Get().Find(floorNumber);
    if (!def) def = FloorDatabase::Get().Find(1);
    field_.Build(*def);
    enemies_.clear();
    spawners_.clear();
    spawners_.resize(def->zones.size());
    for (size_t i = 0; i < def->zones.size(); ++i) {
        spawners_[i].zone = &def->zones[i];
        for (int n = 0; n < def->zones[i].count; ++n) SpawnOne(static_cast<int>(i));
    }
    SpawnBoss();
}

void World::SpawnOne(int spawnerIndex)
{
    Spawner& sp = spawners_[spawnerIndex];
    const EnemyDef* def = EnemyDatabase::Get().Find(sp.zone->enemyId);
    if (!def) return;
    const float a = Random::Float(-kPi, kPi);
    const float r = Random::Float(0.0f, sp.zone->radius);
    VECTOR p = VAdd(sp.zone->center, VGet(std::sin(a) * r, 0.0f, std::cos(a) * r));
    field_.ResolveCollision(p, def->radius);
    enemies_.push_back(std::make_unique<Enemy>(*def, p, spawnerIndex));
    ++sp.alive;
}

void World::SpawnBoss()
{
    const FloorDef& f = field_.Def();
    const EnemyDef* def = EnemyDatabase::Get().Find(f.bossId);
    if (!def) return;
    enemies_.push_back(std::make_unique<Enemy>(*def, f.bossPos, -1));
    enemies_.back()->yaw = kPi; // 入口（南）を向く
    bossAlive_ = true;
}

void World::Update(float dt, Player& player, EventSink& events)
{
    player.inTown = field_.InTown(player.pos);
    const bool targetable = !player.IsDead() && !player.inTown;

    for (auto& e : enemies_) {
        e->Update(dt, player, targetable, events);
        if (!e->IsDead()) {
            // 町には入れない
            if (field_.InTown(e->pos)) {
                const VECTOR tc = field_.Def().townCenter;
                VECTOR d = VSub(e->pos, tc);
                d.y = 0.0f;
                const float len = VSize(d);
                if (len > 0.01f) e->pos = VAdd(tc, VScale(d, (field_.Def().townRadius + 1.0f) / len));
            }
            field_.ResolveCollision(e->pos, e->radius);
        }
    }
    SeparateCharacters(player);

    // 死体の除去と再出現
    for (auto it = enemies_.begin(); it != enemies_.end();) {
        if ((*it)->ReadyToRemove()) it = enemies_.erase(it);
        else ++it;
    }
    for (size_t i = 0; i < spawners_.size(); ++i) {
        auto& timers = spawners_[i].timers;
        for (auto t = timers.begin(); t != timers.end();) {
            *t -= dt;
            if (*t <= 0.0f) {
                t = timers.erase(t);
                SpawnOne(static_cast<int>(i));
            } else {
                ++t;
            }
        }
    }
    if (!bossAlive_) {
        bossRespawnTimer_ -= dt;
        if (bossRespawnTimer_ <= 0.0f && !field_.InBossArena(player.pos)) {
            SpawnBoss();
            events.Info(field_.Def().name + " のボスが再び姿を現した");
        }
    }
}

void World::SeparateCharacters(Player& player)
{
    // 敵同士・敵とプレイヤーが重ならないよう押し出す
    for (size_t i = 0; i < enemies_.size(); ++i) {
        Enemy& a = *enemies_[i];
        if (a.IsDead()) continue;
        for (size_t j = i + 1; j < enemies_.size(); ++j) {
            Enemy& b = *enemies_[j];
            if (b.IsDead()) continue;
            const float minD = a.radius + b.radius;
            const float d = DistXZ(a.pos, b.pos);
            if (d >= minD || d < 0.01f) continue;
            const VECTOR push = VScale(VSub(a.pos, b.pos), (minD - d) * 0.5f / d);
            a.pos = VAdd(a.pos, VGet(push.x, 0, push.z));
            b.pos = VSub(b.pos, VGet(push.x, 0, push.z));
        }
        if (!player.IsDead()) {
            const float minD = a.radius + player.radius;
            const float d = DistXZ(a.pos, player.pos);
            if (d < minD && d > 0.01f) {
                const VECTOR push = VScale(VSub(player.pos, a.pos), (minD - d) / d);
                player.pos = VAdd(player.pos, VGet(push.x, 0, push.z));
                field_.ResolveCollision(player.pos, player.radius);
            }
        }
    }
}

int World::PlayerAttack(Player& player, float range, float halfArc, float multiplier, float extraCrit, EventSink& events)
{
    int hits = 0;
    for (auto& e : enemies_) {
        if (e->IsDead()) continue;
        if (!InArc(player.pos, player.yaw, e->pos, range, halfArc, e->radius)) continue;
        const DamageResult dmg = CalcDamage(player.stats, e->stats, multiplier, extraCrit);
        const DamageResult applied = e->TakeDamage(dmg);
        if (applied.amount <= 0) continue;
        ++hits;
        GameEvent ev;
        ev.type = GameEvent::Type::DamageToEnemy;
        ev.pos = VAdd(e->pos, VGet(0, e->Height() * 0.9f, 0));
        ev.amount = applied.amount;
        ev.critical = applied.critical;
        events.Push(ev);
        SoundManager::Get().PlaySE(applied.critical ? "hit_critical" : "hit");
        if (e->IsDead()) OnEnemyKilled(*e, events);
    }
    return hits;
}

void World::OnEnemyKilled(Enemy& e, EventSink& events)
{
    if (e.RewardGiven()) return;
    e.MarkRewardGiven();
    const EnemyDef& d = e.Def();

    GameEvent ev;
    ev.type = d.isBoss ? GameEvent::Type::BossKilled : GameEvent::Type::EnemyKilled;
    ev.enemy = &d;
    ev.pos = e.pos;
    ev.drops = d.drops.Roll();
    ev.exp = d.exp;
    ev.gold = Random::Int(d.goldMin, d.goldMax);
    ev.fightTime = e.EngageTime();
    events.Push(ev);

    if (d.isBoss) {
        bossAlive_ = false;
        bossRespawnTimer_ = field_.Def().bossRespawnTime;
        SoundManager::Get().PlaySE("boss_defeat");
    } else if (e.SpawnerIndex() >= 0) {
        Spawner& sp = spawners_[e.SpawnerIndex()];
        --sp.alive;
        sp.timers.push_back(sp.zone->respawnTime);
    }
}

Enemy* World::NearestEnemy(const VECTOR& from, float maxDist) const
{
    Enemy* best = nullptr;
    float bestD = maxDist;
    for (const auto& e : enemies_) {
        if (e->IsDead()) continue;
        const float d = DistXZ(from, e->pos) - e->radius;
        if (d < bestD) {
            bestD = d;
            best = e.get();
        }
    }
    return best;
}

const Enemy* World::ActiveBoss() const
{
    for (const auto& e : enemies_) if (e->IsBoss() && !e->IsDead() && e->IsEngaged()) return e.get();
    return nullptr;
}

void World::Draw(const VECTOR& cameraPos) const
{
    field_.Draw(cameraPos);
    for (const auto& e : enemies_) e->DrawTelegraph();
    const float cullSq = 9000.0f * 9000.0f;
    for (const auto& e : enemies_) {
        if (DistXZSq(e->pos, cameraPos) > cullSq) continue;
        e->Draw();
    }
}

void World::DrawOverlays(const VECTOR& cameraPos) const
{
    field_.DrawLabels();
    const FontManager& fm = FontManager::Get();
    for (const auto& e : enemies_) {
        if (e->IsDead() || e->IsBoss()) continue;
        if (DistXZSq(e->pos, cameraPos) > 2600.0f * 2600.0f) continue;
        const VECTOR s = ConvWorldPosToScreenPos(VAdd(e->pos, VGet(0, e->Height() + 40.0f, 0)));
        if (s.z <= 0.0f || s.z >= 1.0f) continue;
        const int x = static_cast<int>(s.x), y = static_cast<int>(s.y);
        const unsigned int nameColor = e->IsEngaged() ? GetColor(255, 140, 120) : GetColor(240, 240, 240);
        fm.DrawCentered(x, y - 26, Format("Lv%d %s", e->Def().level, e->Def().name.c_str()), nameColor, FontId::Small, GetColor(0, 0, 0));
        const int w = 110, h = 8;
        DrawBox(x - w / 2 - 1, y - 1, x + w / 2 + 1, y + h + 1, GetColor(0, 0, 0), TRUE);
        DrawBox(x - w / 2, y, x - w / 2 + static_cast<int>(w * e->HpRate()), y + h, GetColor(220, 60, 50), TRUE);
    }
}

} // namespace ag
