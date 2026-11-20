#pragma once
// 所持品と装備
#include "Item.h"
#include <vector>
#include <array>

namespace ag {

class Inventory {
public:
    static constexpr int kCapacity = 60;

    // 追加（スタック可能なら既存に積む）。入りきらなかった個数を返す。
    int Add(const ItemInstance& item);
    bool RemoveAt(int index, int count = 1);
    bool RemoveById(const std::string& defId, int count = 1);
    int CountOf(const std::string& defId) const;
    int FindIndex(const std::string& defId) const;

    int Size() const { return static_cast<int>(items_.size()); }
    bool IsFull() const { return Size() >= kCapacity; }
    ItemInstance& At(int i) { return items_[i]; }
    const ItemInstance& At(int i) const { return items_[i]; }
    const std::vector<ItemInstance>& Items() const { return items_; }
    void Clear() { items_.clear(); }
    void SortByCategory();

private:
    std::vector<ItemInstance> items_;
};

class Equipment {
public:
    static constexpr int kSlotCount = static_cast<int>(EquipSlot::Count);

    bool Has(EquipSlot s) const { return slots_[static_cast<int>(s)].Valid(); }
    const ItemInstance& Get(EquipSlot s) const { return slots_[static_cast<int>(s)]; }
    ItemInstance& Get(EquipSlot s) { return slots_[static_cast<int>(s)]; }
    void Set(EquipSlot s, const ItemInstance& item) { slots_[static_cast<int>(s)] = item; }
    ItemInstance Take(EquipSlot s);
    void Clear() { for (auto& s : slots_) s = ItemInstance{}; }

    WeaponType CurrentWeaponType() const;
    // 槍など両手持ち武器を装備中は盾が無効
    bool IsShieldDisabled() const;
    StatBonus TotalBonus() const;

private:
    std::array<ItemInstance, kSlotCount> slots_{};
};

} // namespace ag
