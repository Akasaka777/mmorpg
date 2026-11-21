#pragma once
// ドロップテーブル
//   entries        : それぞれ独立に抽選（素材・消費アイテム向け）
//   equipmentPool  : 重み付きで poolRolls 回抽選（ボスの装備ドロップ向け）
#include "Item.h"
#include <vector>
#include <string>

namespace ag {

struct DropEntry {
    std::string itemId;
    float chance = 1.0f;
    int minCount = 1;
    int maxCount = 1;
};

struct WeightedDrop {
    std::string itemId;
    int weight = 1;
};

struct DropTable {
    std::vector<DropEntry> entries;
    std::vector<WeightedDrop> equipmentPool;
    int poolRolls = 0;          // 確定抽選回数
    float bonusRollChance = 0;  // 追加でもう1回抽選する確率
    int luck = 0;               // 個体値の幸運補正

    std::vector<ItemInstance> Roll() const;
};

} // namespace ag
