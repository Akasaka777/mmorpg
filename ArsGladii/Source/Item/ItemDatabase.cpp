#include "ItemDatabase.h"

namespace ag {

ItemDatabase& ItemDatabase::Get()
{
    static ItemDatabase instance;
    return instance;
}

namespace {

ItemDef Material(const char* id, const char* name, const char* desc, Rarity r, int price)
{
    ItemDef d;
    d.id = id; d.name = name; d.description = desc; d.category = ItemCategory::Material;
    d.rarity = r; d.maxStack = 99; d.sellPrice = price;
    return d;
}

ItemDef Consumable(const char* id, const char* name, const char* desc, Rarity r, ConsumableEffect e, int price)
{
    ItemDef d;
    d.id = id; d.name = name; d.description = desc; d.category = ItemCategory::Consumable;
    d.rarity = r; d.maxStack = 20; d.sellPrice = price; d.effect = e;
    return d;
}

ItemDef Weapon(const char* id, const char* name, const char* desc, WeaponType w, Rarity r,
               int atk, float crit, float critDmg, int dur, int price)
{
    ItemDef d;
    d.id = id; d.name = name; d.description = desc; d.category = ItemCategory::Weapon;
    d.weaponType = w; d.rarity = r; d.base.atk = atk; d.base.critRate = crit; d.base.critDamage = critDmg;
    d.baseDurability = dur; d.sellPrice = price;
    return d;
}

ItemDef Armor(const char* id, const char* name, const char* desc, ItemCategory c, Rarity r,
              int def, int hp, float speed, int dur, int price)
{
    ItemDef d;
    d.id = id; d.name = name; d.description = desc; d.category = c; d.rarity = r;
    d.base.def = def; d.base.maxHP = hp; d.base.moveSpeed = speed; d.baseDurability = dur; d.sellPrice = price;
    return d;
}

ConsumableEffect Heal(int hp, int mp) { ConsumableEffect e; e.healHP = hp; e.healMP = mp; return e; }
ConsumableEffect Buff(float atk, float def, float sec) { ConsumableEffect e; e.atkBuffRate = atk; e.defBuffRate = def; e.buffDuration = sec; return e; }

} // namespace

ItemDatabase::ItemDatabase()
{
    using W = WeaponType;
    using C = ItemCategory;

    // ---- 素材（雑魚ドロップ）----
    Add(Material("wolf_pelt",   "灰狼の毛皮", "草原ウルフの毛皮。防具の素材になる。", Rarity::Common, 5));
    Add(Material("wolf_fang",   "鋭い獣牙",   "鋭く尖った牙。武器の強化素材。", Rarity::Uncommon, 12));
    Add(Material("goblin_cloth","粗い布切れ", "ゴブリンが身に着けていた布。", Rarity::Common, 3));
    Add(Material("iron_scrap",  "鉄くず",     "錆びた鉄片。鍛冶の素材になる。", Rarity::Common, 6));
    Add(Material("boar_hide",   "硬い獣皮",   "岩角イノシシの分厚い皮。", Rarity::Common, 7));
    Add(Material("graymane_core","灰牙の魂核", "第1層の主から得られる魂の結晶。強力な装備の素材。", Rarity::Epic, 300));

    // ---- 消費アイテム ----
    Add(Consumable("potion_s",  "回復薬(小)", "HP を 150 回復する。", Rarity::Common, Heal(150, 0), 10));
    Add(Consumable("potion_m",  "回復薬(中)", "HP を 400 回復する。", Rarity::Uncommon, Heal(400, 0), 40));
    Add(Consumable("ether_s",   "魔力水",     "MP を 40 回復する。", Rarity::Common, Heal(0, 40), 15));
    Add(Consumable("pill_atk",  "闘志の丸薬", "60 秒間 ATK が 20% 上昇する。", Rarity::Uncommon, Buff(0.20f, 0.0f, 60.0f), 30));
    Add(Consumable("pill_def",  "守護の丸薬", "60 秒間 DEF が 30% 上昇する。", Rarity::Uncommon, Buff(0.0f, 0.30f, 60.0f), 30));

    // ---- 初期装備 ----
    Add(Weapon("sword_training", "練習用の片手剣", "はじまりの町で支給される剣。", W::OneHandSword, Rarity::Common, 8, 0.0f, 0.0f, 200, 5));
    Add(Armor("body_cloth", "布の服", "ありふれた旅人の服。", C::Body, Rarity::Common, 3, 10, 0.0f, 200, 3));

    // ---- 第1層ボス「灰牙の獣王 グラウゼル」ドロップ ----
    Add(Weapon("sword_graymane",  "灰牙の剣",   "獣王の牙を鍛えた片手剣。", W::OneHandSword, Rarity::Rare, 28, 0.03f, 0.0f, 320, 120));
    Add(Weapon("mace_graymane",   "灰牙の戦棍", "獣王の骨を芯にした重い棍。", W::OneHandMace, Rarity::Rare, 34, 0.0f, 0.15f, 360, 120));
    Add(Weapon("dagger_graymane", "灰牙の短刀", "獣王の爪から削り出した短刀。", W::Dagger, Rarity::Rare, 20, 0.08f, 0.0f, 280, 120));
    Add(Weapon("rapier_graymane", "灰牙の細剣", "しなやかで折れにくい細剣。", W::Rapier, Rarity::Rare, 24, 0.05f, 0.05f, 280, 120));
    Add(Weapon("spear_graymane",  "灰牙の槍",   "獣王の角を穂先にした長槍。", W::Spear, Rarity::Rare, 32, 0.02f, 0.0f, 340, 120));
    Add(Armor("head_beastking",   "獣王の兜",     "獣王の頭骨を加工した兜。", C::Head, Rarity::Rare, 10, 40, 0.0f, 260, 90));
    Add(Armor("body_beastking",   "獣王の胸当て", "獣王の毛皮で裏打ちした胸当て。", C::Body, Rarity::Rare, 18, 80, 0.0f, 300, 110));
    Add(Armor("shield_beastking", "獣王の円盾",   "獣王の甲殻を張った円盾。", C::Shield, Rarity::Rare, 14, 20, -10.0f, 300, 100));
    Add(Weapon("sword_graymane_ex", "グラウゼルファング", "獣王の魂が宿る剣。まれにしか手に入らない。", W::OneHandSword, Rarity::Epic, 40, 0.08f, 0.20f, 400, 500));
}

const ItemDef* ItemDatabase::Find(const std::string& id) const
{
    for (const auto& d : items_) if (d.id == id) return &d;
    return nullptr;
}

} // namespace ag
