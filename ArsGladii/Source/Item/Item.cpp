#include "Item.h"
#include "ItemDatabase.h"
#include "../Core/Random.h"
#include "DxLib.h"
#include <algorithm>
#include <cmath>

namespace ag {

namespace {
std::uint32_t g_nextUid = 1;

float IvFactor(int iv) { return 0.80f + 0.40f * (iv / 100.0f); }
}

std::uint32_t NextItemUid() { return g_nextUid++; }
std::uint32_t PeekNextItemUid() { return g_nextUid; }
void SetNextItemUid(std::uint32_t uid) { g_nextUid = std::max<std::uint32_t>(uid, 1); }

const ItemDef* ItemInstance::Def() const { return ItemDatabase::Get().Find(defId); }

StatBonus ItemInstance::EffectiveBonus() const
{
    const ItemDef* d = Def();
    StatBonus b;
    if (!d || !d->IsEquipment()) return b;
    b.atk = static_cast<int>(std::lround(d->base.atk * IvFactor(iv[(int)IvStat::Atk])));
    b.def = static_cast<int>(std::lround(d->base.def * IvFactor(iv[(int)IvStat::Def])));
    b.maxHP = static_cast<int>(std::lround(d->base.maxHP * IvFactor(iv[(int)IvStat::HP])));
    b.maxMP = d->base.maxMP;
    b.critRate = d->base.critRate * IvFactor(iv[(int)IvStat::Crit]);
    b.critDamage = d->base.critDamage;
    b.moveSpeed = d->base.moveSpeed;
    if (IsBroken()) b = b.Scaled(0.5f);
    return b;
}

int ItemInstance::IvAverage() const
{
    int sum = 0;
    for (int v : iv) sum += v;
    return sum / static_cast<int>(IvStat::Count);
}

char ItemInstance::IvRank() const
{
    const int a = IvAverage();
    if (a >= 85) return 'S';
    if (a >= 65) return 'A';
    if (a >= 40) return 'B';
    return 'C';
}

EquipSlot SlotForCategory(ItemCategory c)
{
    switch (c) {
    case ItemCategory::Weapon: return EquipSlot::Weapon;
    case ItemCategory::Head:   return EquipSlot::Head;
    case ItemCategory::Body:   return EquipSlot::Body;
    case ItemCategory::Shield: return EquipSlot::Shield;
    default:                   return EquipSlot::None;
    }
}

const char* EquipSlotName(EquipSlot s)
{
    switch (s) {
    case EquipSlot::Weapon: return "武器";
    case EquipSlot::Head:   return "頭";
    case EquipSlot::Body:   return "体";
    case EquipSlot::Shield: return "盾";
    default:                return "";
    }
}

const char* CategoryName(ItemCategory c)
{
    switch (c) {
    case ItemCategory::Weapon:     return "武器";
    case ItemCategory::Head:       return "頭装備";
    case ItemCategory::Body:       return "体装備";
    case ItemCategory::Shield:     return "盾";
    case ItemCategory::Material:   return "素材";
    case ItemCategory::Consumable: return "消費アイテム";
    }
    return "";
}

unsigned int RarityColor(Rarity r)
{
    switch (r) {
    case Rarity::Common:   return GetColor(235, 235, 235);
    case Rarity::Uncommon: return GetColor(120, 230, 120);
    case Rarity::Rare:     return GetColor(110, 170, 255);
    case Rarity::Epic:     return GetColor(210, 130, 255);
    }
    return GetColor(255, 255, 255);
}

unsigned int RankColor(char rank)
{
    switch (rank) {
    case 'S': return GetColor(255, 200, 60);
    case 'A': return GetColor(230, 120, 255);
    case 'B': return GetColor(100, 180, 255);
    default:  return GetColor(200, 200, 200);
    }
}

ItemInstance CreateItem(const std::string& defId, int count, int luck)
{
    ItemInstance it;
    it.defId = defId;
    it.count = count;
    it.uid = NextItemUid();
    const ItemDef* d = ItemDatabase::Get().Find(defId);
    if (d && d->IsEquipment()) {
        it.count = 1;
        for (int& v : it.iv) {
            int roll = Random::Int(0, 100);
            for (int i = 0; i < luck; ++i) roll = std::max(roll, Random::Int(0, 100)); // 幸運: 複数回振って高い方
            v = roll;
        }
        it.maxDurability = std::max(1, static_cast<int>(std::lround(
            d->baseDurability * (0.70f + 0.60f * it.iv[(int)IvStat::Durability] / 100.0f))));
        it.durability = it.maxDurability;
    }
    return it;
}

} // namespace ag
