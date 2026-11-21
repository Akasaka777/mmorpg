#include "Inventory.h"
#include <algorithm>

namespace ag {

int Inventory::Add(const ItemInstance& item)
{
    const ItemDef* d = item.Def();
    if (!d) return item.count;
    int remaining = item.count;
    if (d->maxStack > 1) {
        for (auto& it : items_) {
            if (it.defId != item.defId || it.count >= d->maxStack) continue;
            const int add = std::min(remaining, d->maxStack - it.count);
            it.count += add;
            remaining -= add;
            if (remaining <= 0) return 0;
        }
    }
    while (remaining > 0 && !IsFull()) {
        ItemInstance copy = item;
        copy.count = std::min(remaining, std::max(1, d->maxStack));
        if (copy.uid == 0 || copy.count != item.count) copy.uid = NextItemUid();
        remaining -= copy.count;
        items_.push_back(copy);
    }
    return remaining;
}

bool Inventory::RemoveAt(int index, int count)
{
    if (index < 0 || index >= Size()) return false;
    ItemInstance& it = items_[index];
    if (it.count < count) return false;
    it.count -= count;
    if (it.count <= 0) items_.erase(items_.begin() + index);
    return true;
}

bool Inventory::RemoveById(const std::string& defId, int count)
{
    if (CountOf(defId) < count) return false;
    for (int i = Size() - 1; i >= 0 && count > 0; --i) {
        if (items_[i].defId != defId) continue;
        const int take = std::min(count, items_[i].count);
        RemoveAt(i, take);
        count -= take;
    }
    return true;
}

int Inventory::CountOf(const std::string& defId) const
{
    int n = 0;
    for (const auto& it : items_) if (it.defId == defId) n += it.count;
    return n;
}

int Inventory::FindIndex(const std::string& defId) const
{
    for (int i = 0; i < Size(); ++i) if (items_[i].defId == defId) return i;
    return -1;
}

void Inventory::SortByCategory()
{
    std::stable_sort(items_.begin(), items_.end(), [](const ItemInstance& a, const ItemInstance& b) {
        const ItemDef* da = a.Def();
        const ItemDef* db = b.Def();
        if (!da || !db) return da != nullptr;
        if (da->category != db->category) return static_cast<int>(da->category) < static_cast<int>(db->category);
        if (da->rarity != db->rarity) return static_cast<int>(da->rarity) > static_cast<int>(db->rarity);
        return a.IvAverage() > b.IvAverage();
    });
}

// ---------------------------------------------------------------------------

ItemInstance Equipment::Take(EquipSlot s)
{
    ItemInstance out = slots_[static_cast<int>(s)];
    slots_[static_cast<int>(s)] = ItemInstance{};
    return out;
}

WeaponType Equipment::CurrentWeaponType() const
{
    const ItemInstance& w = Get(EquipSlot::Weapon);
    if (!w.Valid()) return WeaponType::OneHandSword; // 素手は片手剣扱い（プロトタイプ）
    return w.Def()->weaponType;
}

bool Equipment::IsShieldDisabled() const
{
    return Has(EquipSlot::Weapon) && GetWeaponTypeInfo(CurrentWeaponType()).twoHanded;
}

StatBonus Equipment::TotalBonus() const
{
    StatBonus total;
    for (int i = 0; i < kSlotCount; ++i) {
        if (!slots_[i].Valid()) continue;
        if (static_cast<EquipSlot>(i) == EquipSlot::Shield && IsShieldDisabled()) continue;
        total += slots_[i].EffectiveBonus();
    }
    return total;
}

} // namespace ag
