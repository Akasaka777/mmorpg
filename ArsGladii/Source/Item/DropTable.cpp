#include "DropTable.h"
#include "../Core/Random.h"

namespace ag {

namespace {
const std::string* PickWeighted(const std::vector<WeightedDrop>& pool)
{
    int total = 0;
    for (const auto& w : pool) total += w.weight;
    if (total <= 0) return nullptr;
    int r = Random::Int(0, total - 1);
    for (const auto& w : pool) {
        if (r < w.weight) return &w.itemId;
        r -= w.weight;
    }
    return nullptr;
}
}

std::vector<ItemInstance> DropTable::Roll() const
{
    std::vector<ItemInstance> out;
    for (const auto& e : entries) {
        if (!Random::Chance(e.chance)) continue;
        out.push_back(CreateItem(e.itemId, Random::Int(e.minCount, e.maxCount), luck));
    }
    int rolls = poolRolls + (Random::Chance(bonusRollChance) ? 1 : 0);
    for (int i = 0; i < rolls; ++i) {
        if (const std::string* id = PickWeighted(equipmentPool)) out.push_back(CreateItem(*id, 1, luck));
    }
    return out;
}

} // namespace ag
