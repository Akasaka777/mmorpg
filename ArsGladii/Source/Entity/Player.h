#pragma once
// プレイヤーキャラクター
//   ・WASD 移動（カメラ基準）/ Shift ダッシュ / Space 回避ステップ（無敵時間あり）
//   ・左クリック: 通常攻撃（3段コンボ。移動しながら出せるリアルタイム攻撃）
//   ・1～4: ソードスキル（モーション固定・MP消費・クールダウン）
//   ・R: クイックアイテム（回復薬）
#include "Character.h"
#include "../Combat/Skill.h"
#include "../Item/Inventory.h"
#include "../World/GameEvent.h"
#include <array>
#include <map>
#include <string>

namespace ag {

class Camera;
class World;

class Player : public Character {
public:
    enum class Action { Free, Attack, Skill, Dodge, Stagger, Dead };

    Player();

    void NewGame(const VECTOR& startPos);
    void Update(float dt, const Camera& cam, World& world, EventSink& events, bool inputEnabled);
    void Draw() const;
    DamageResult TakeDamage(const DamageResult& dmg) override;

    // ステータス再計算（装備変更・レベルアップ・バフ時）
    void RecalcStats();
    int AddExp(int amount); // 上がったレベル数を返す
    void Respawn(const VECTOR& p);

    // アイテム
    bool UseItem(int invIndex, EventSink& events);
    bool UseQuickItem(EventSink& events);
    bool Equip(int invIndex, EventSink& events);
    bool Unequip(EquipSlot slot, EventSink& events);
    void WearEquipment(EquipSlot slot, int amount);

    // スキル
    WeaponType CurrentWeapon() const { return equipment.CurrentWeaponType(); }
    const SkillDef* SlotSkill(int slot) const;
    void SetSlotSkill(int slot, const std::string& skillId);
    std::array<std::string, kSkillSlotCount>& SlotsFor(WeaponType w) { return skillSlots[static_cast<int>(w)]; }
    bool IsSkillUnlocked(const SkillDef& s) const { return level >= s.requiredLevel; }
    float CooldownRemaining(const std::string& skillId) const;
    void FillDefaultSkillSlots();

    Action CurrentAction() const { return action_; }
    bool IsInvulnerable() const { return invulnTimer_ > 0.0f; }
    float DodgeCooldownRate() const { return dodgeCd_ / 0.8f; }

    // ---- 保存対象データ ----
    std::string name = "プレイヤー";
    int level = 1;
    int exp = 0;
    int gold = 0;
    Inventory inventory;
    Equipment equipment;
    std::array<std::array<std::string, kSkillSlotCount>, static_cast<int>(WeaponType::Count)> skillSlots{};
    std::map<std::string, float> cooldowns;
    std::string quickItemId = "potion_s";
    float atkBuffRate = 0.0f, atkBuffTime = 0.0f;
    float defBuffRate = 0.0f, defBuffTime = 0.0f;

    bool inTown = false;

private:
    void HandleMovement(float dt, const Camera& cam, World& world, bool inputEnabled, float speedScale);
    void StartAttack(int step, World& world);
    bool TryStartSkill(int slot, World& world, EventSink& events);
    void FaceNearestEnemy(World& world, float range);
    void UpdateLook();

    Action action_ = Action::Free;
    float actionTimer_ = 0.0f;
    int comboStep_ = 0;
    bool comboQueued_ = false;
    bool hitDone_ = false;
    float comboResetTimer_ = 0.0f;
    const SkillDef* activeSkill_ = nullptr;
    size_t skillHitIndex_ = 0;
    float skillSpin_ = 0.0f;
    VECTOR dodgeDir_ = VGet(0, 0, 1);
    float dodgeCd_ = 0.0f;
    float invulnTimer_ = 0.0f;
    float hpRegenAcc_ = 0.0f;
    float mpRegenAcc_ = 0.0f;
};

} // namespace ag
