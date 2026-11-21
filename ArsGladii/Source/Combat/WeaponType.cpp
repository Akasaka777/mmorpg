#include "WeaponType.h"
#include <cstring>

namespace ag {

namespace {
const WeaponTypeInfo kInfos[] = {
    //  key               name     range  arc   dur   hit   combo               crit   critDmg 2H
    { "one_hand_sword", "片手剣", 230.f, 120.f, 0.42f, 0.45f, { 1.00f, 1.05f, 1.35f }, 0.00f, 0.00f, false },
    { "one_hand_mace",  "片手棍", 210.f, 110.f, 0.55f, 0.55f, { 1.20f, 1.25f, 1.70f }, -0.02f, 0.30f, false },
    { "dagger",         "短剣",   170.f, 100.f, 0.30f, 0.40f, { 0.70f, 0.75f, 1.00f }, 0.10f, 0.00f, false },
    { "rapier",         "細剣",   270.f,  50.f, 0.36f, 0.45f, { 0.90f, 0.95f, 1.25f }, 0.05f, 0.10f, false },
    { "spear",          "槍",     330.f,  70.f, 0.52f, 0.50f, { 1.05f, 1.10f, 1.45f }, 0.00f, 0.05f, true  },
};
}

const WeaponTypeInfo& GetWeaponTypeInfo(WeaponType t)
{
    int i = static_cast<int>(t);
    if (i < 0 || i >= static_cast<int>(WeaponType::Count)) i = 0;
    return kInfos[i];
}

WeaponType WeaponTypeFromKey(const char* key)
{
    for (int i = 0; i < static_cast<int>(WeaponType::Count); ++i)
        if (std::strcmp(kInfos[i].key, key) == 0) return static_cast<WeaponType>(i);
    return WeaponType::None;
}

} // namespace ag
