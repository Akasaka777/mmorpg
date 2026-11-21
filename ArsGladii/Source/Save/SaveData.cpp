#include "SaveData.h"
#include "../Entity/Player.h"
#include "../Core/KeyValueFile.h"
#include "../Item/ItemDatabase.h"
#include <ctime>
#include <algorithm>
#include <sstream>
#include <cstdlib>

namespace ag {

namespace {
constexpr int kSaveVersion = 1;

// 1アイテム = "defId,count,uid,iv0,iv1,iv2,iv3,iv4,durability,maxDurability"
std::string SerializeItem(const ItemInstance& it)
{
    std::ostringstream ss;
    ss << it.defId << ',' << it.count << ',' << it.uid;
    for (int v : it.iv) ss << ',' << v;
    ss << ',' << it.durability << ',' << it.maxDurability;
    return ss.str();
}

bool DeserializeItem(const std::string& s, ItemInstance& out)
{
    const auto f = KeyValueFile::Split(s, ',');
    if (f.size() < 10) return false;
    out.defId = f[0];
    if (!ItemDatabase::Get().Find(out.defId)) return false; // 削除されたアイテムは読み飛ばす
    out.count = std::atoi(f[1].c_str());
    out.uid = static_cast<std::uint32_t>(std::strtoul(f[2].c_str(), nullptr, 10));
    for (int i = 0; i < static_cast<int>(IvStat::Count); ++i) out.iv[i] = std::atoi(f[3 + i].c_str());
    out.durability = std::atoi(f[8].c_str());
    out.maxDurability = std::atoi(f[9].c_str());
    return out.count > 0;
}

std::string NowString()
{
    std::time_t t = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y/%m/%d %H:%M", &tm);
    return buf;
}
} // namespace

bool SaveSystem::Save(const std::string& path, const Player& p, int floor)
{
    KeyValueFile kv;
    kv.SetInt("version", kSaveVersion);
    kv.SetString("saved_at", NowString());
    kv.SetInt("floor", floor);
    kv.SetString("name", p.name);
    kv.SetInt("level", p.level);
    kv.SetInt("exp", p.exp);
    kv.SetInt("gold", p.gold);
    kv.SetInt("hp", p.hp);
    kv.SetInt("mp", p.mp);
    kv.SetFloat("pos_x", p.pos.x);
    kv.SetFloat("pos_z", p.pos.z);
    kv.SetFloat("yaw", p.yaw);
    kv.SetString("quick_item", p.quickItemId);
    kv.SetInt("next_uid", static_cast<int>(PeekNextItemUid()));

    for (int i = 0; i < Equipment::kSlotCount; ++i) {
        const ItemInstance& it = p.equipment.Get(static_cast<EquipSlot>(i));
        kv.SetString("equip" + std::to_string(i), it.Valid() ? SerializeItem(it) : "");
    }
    kv.SetInt("item_count", p.inventory.Size());
    for (int i = 0; i < p.inventory.Size(); ++i) kv.SetString("item" + std::to_string(i), SerializeItem(p.inventory.At(i)));

    for (int w = 0; w < static_cast<int>(WeaponType::Count); ++w) {
        std::string line;
        for (int s = 0; s < kSkillSlotCount; ++s) {
            if (s) line += ',';
            line += p.skillSlots[w][s];
        }
        kv.SetString(std::string("skills_") + GetWeaponTypeInfo(static_cast<WeaponType>(w)).key, line);
    }
    return kv.Save(path);
}

bool SaveSystem::Load(const std::string& path, Player& p, int& floor)
{
    KeyValueFile kv;
    if (!kv.Load(path)) return false;
    if (kv.GetInt("version", 0) <= 0) return false;

    floor = kv.GetInt("floor", 1);
    p.name = kv.GetString("name", p.name);
    p.level = kv.GetInt("level", 1);
    p.exp = kv.GetInt("exp", 0);
    p.gold = kv.GetInt("gold", 0);
    p.quickItemId = kv.GetString("quick_item", "potion_s");
    SetNextItemUid(static_cast<std::uint32_t>(kv.GetInt("next_uid", 1)));

    p.equipment.Clear();
    for (int i = 0; i < Equipment::kSlotCount; ++i) {
        ItemInstance it;
        if (DeserializeItem(kv.GetString("equip" + std::to_string(i)), it)) p.equipment.Set(static_cast<EquipSlot>(i), it);
    }
    p.inventory.Clear();
    const int n = kv.GetInt("item_count", 0);
    for (int i = 0; i < n; ++i) {
        ItemInstance it;
        if (DeserializeItem(kv.GetString("item" + std::to_string(i)), it)) p.inventory.Add(it);
    }
    for (auto& slots : p.skillSlots) slots.fill("");
    for (int w = 0; w < static_cast<int>(WeaponType::Count); ++w) {
        const auto ids = KeyValueFile::Split(kv.GetString(std::string("skills_") + GetWeaponTypeInfo(static_cast<WeaponType>(w)).key), ',');
        for (int s = 0; s < kSkillSlotCount && s < static_cast<int>(ids.size()); ++s)
            if (SkillDatabase::Get().Find(ids[s])) p.skillSlots[w][s] = ids[s];
    }
    p.FillDefaultSkillSlots();
    p.cooldowns.clear();
    p.atkBuffTime = p.defBuffTime = 0.0f;

    p.hp = 1;
    p.RecalcStats();
    p.Respawn(VGet(kv.GetFloat("pos_x", 0.0f), 0.0f, kv.GetFloat("pos_z", 0.0f)));
    p.hp = std::max(1, std::min(p.stats.maxHP, kv.GetInt("hp", p.stats.maxHP)));
    p.mp = std::max(0, std::min(p.stats.maxMP, kv.GetInt("mp", p.stats.maxMP)));
    p.yaw = kv.GetFloat("yaw", 0.0f);
    return true;
}

SaveHeader SaveSystem::Peek(const std::string& path)
{
    SaveHeader h;
    KeyValueFile kv;
    if (!kv.Load(path) || kv.GetInt("version", 0) <= 0) return h;
    h.exists = true;
    h.level = kv.GetInt("level", 1);
    h.floor = kv.GetInt("floor", 1);
    h.savedAt = kv.GetString("saved_at");
    return h;
}

} // namespace ag
