#pragma once
// アイテム定義（ItemDef）と所持アイテムの個体（ItemInstance）
//
// ■ 個体値（IV）
//   武器・防具はドロップ時に 0～100 の個体値を ATK/DEF/CRIT/HP/耐久 ごとに持つ。
//   実ステータス = 基礎値 × (0.80 + 0.40 × IV/100)   … 基礎値の 80%～120%
//   最大耐久値   = 基礎耐久 × (0.70 + 0.60 × IV/100) … 基礎耐久の 70%～130%
//   平均 IV でランク表示: S(85+) / A(65+) / B(40+) / C
//   耐久値 0 の装備は「破損」となり、性能が 50% に落ちる（修理は町の鍛冶屋で今後実装）。
#include "../Combat/Stats.h"
#include "../Combat/WeaponType.h"
#include <string>
#include <cstdint>

namespace ag {

enum class ItemCategory { Weapon, Head, Body, Shield, Material, Consumable };

// 装備スロット。腕・手・足を増やす場合は Count の前に追加し、
// EquipSlotName と ItemCategory の対応（SlotForCategory）を更新する。
enum class EquipSlot { Weapon, Head, Body, Shield, Count, None = -1 };

enum class Rarity { Common, Uncommon, Rare, Epic };

enum class IvStat { Atk, Def, Crit, HP, Durability, Count };

struct ConsumableEffect {
    int   healHP = 0;
    int   healMP = 0;
    float atkBuffRate = 0.0f;  // 0.2 = ATK +20%
    float defBuffRate = 0.0f;
    float buffDuration = 0.0f; // 秒
};

struct ItemDef {
    std::string id;
    std::string name;
    std::string description;
    ItemCategory category = ItemCategory::Material;
    WeaponType weaponType = WeaponType::None;
    Rarity rarity = Rarity::Common;
    StatBonus base;
    int baseDurability = 0;
    int maxStack = 1;
    int sellPrice = 1;
    ConsumableEffect effect;

    bool IsEquipment() const { return category == ItemCategory::Weapon || category == ItemCategory::Head
                                   || category == ItemCategory::Body || category == ItemCategory::Shield; }
};

struct ItemInstance {
    std::string defId;
    int count = 1;
    std::uint32_t uid = 0;
    int iv[static_cast<int>(IvStat::Count)] = { 50, 50, 50, 50, 50 };
    int durability = 0;
    int maxDurability = 0;

    const ItemDef* Def() const;
    bool Valid() const { return !defId.empty() && Def() != nullptr; }
    bool IsBroken() const { return maxDurability > 0 && durability <= 0; }
    StatBonus EffectiveBonus() const;
    int IvAverage() const;
    char IvRank() const;
};

EquipSlot SlotForCategory(ItemCategory c);
const char* EquipSlotName(EquipSlot s);
const char* CategoryName(ItemCategory c);
unsigned int RarityColor(Rarity r);
unsigned int RankColor(char rank);

// 個体値をロールして装備品を生成（luck: 0=通常, 1以上で高IV寄り。ボスは 1）
ItemInstance CreateItem(const std::string& defId, int count = 1, int luck = 0);
std::uint32_t NextItemUid();
std::uint32_t PeekNextItemUid();
void SetNextItemUid(std::uint32_t uid);

} // namespace ag
