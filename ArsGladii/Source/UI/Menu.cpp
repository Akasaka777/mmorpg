#include "Menu.h"
#include "UIDraw.h"
#include "ResultScreen.h"
#include "../Entity/Player.h"
#include "../Core/Config.h"
#include "../Core/Input.h"
#include "../Core/MathUtil.h"
#include "../Core/AssetManager.h"
#include "../Item/ItemDatabase.h"
#include "DxLib.h"
#include <algorithm>
#include <vector>

namespace ag {

namespace {
const char* kTabNames[] = { "ステータス", "装備・所持品", "スキル", "ゲーム設定", "システム" };
const char* kIvNames[] = { "攻撃", "防御", "会心", "体力", "耐久" };

bool PassFilter(const ItemDef* d, int filter)
{
    if (!d) return false;
    switch (filter) {
    case 1: return d->IsEquipment();
    case 2: return d->category == ItemCategory::Consumable;
    case 3: return d->category == ItemCategory::Material;
    default: return true;
    }
}
}

void Menu::Open()
{
    open_ = true;
    selectedInv_ = -1;
    selectedSlot_ = EquipSlot::None;
    confirmQuit_ = false;
    SoundManager::Get().PlaySE("menu_open");
}

void Menu::Close()
{
    open_ = false;
    waitingKey_ = -1;
    if (settingsDirty_) {
        Settings().Save(kConfigPath);
        settingsDirty_ = false;
    }
}

MenuCommand Menu::Run(Player& player, EventSink& events)
{
    if (!open_) return MenuCommand::None;
    Input& in = Input::Get();
    const FontManager& fm = FontManager::Get();

    ui::DimScreen(140);
    const int w = 1560, h = 860;
    const int x = (kScreenWidth - w) / 2, y = (kScreenHeight - h) / 2;
    ui::Panel(x, y, w, h, 235);
    fm.Draw(x + 30, y + 18, "MENU", ui::Gold(), FontId::Large, GetColor(0, 0, 0));

    // タブ
    for (int i = 0; i < static_cast<int>(Tab::Count); ++i) {
        if (ui::Button(x + 200 + i * 210, y + 24, 200, 46, kTabNames[i], tab_ == static_cast<Tab>(i))) {
            tab_ = static_cast<Tab>(i);
            selectedInv_ = -1;
            selectedSlot_ = EquipSlot::None;
            scroll_ = 0;
            confirmQuit_ = false;
            waitingKey_ = -1;
        }
    }
    // Q/E でタブ切り替え（キー設定の入力待ち中は無効）
    if (waitingKey_ < 0 && in.KeyPressed(KEY_INPUT_Q)) tab_ = static_cast<Tab>((static_cast<int>(tab_) + static_cast<int>(Tab::Count) - 1) % static_cast<int>(Tab::Count));
    if (waitingKey_ < 0 && in.KeyPressed(KEY_INPUT_E)) tab_ = static_cast<Tab>((static_cast<int>(tab_) + 1) % static_cast<int>(Tab::Count));

    if (ui::Button(x + w - 140, y + 24, 110, 46, "閉じる")) return MenuCommand::Close;
    DrawLine(x + 20, y + 90, x + w - 20, y + 90, ui::GoldDark(), 1);

    const int cx = x + 30, cy = y + 110, cw = w - 60, ch = h - 140;
    MenuCommand cmd = MenuCommand::None;
    switch (tab_) {
    case Tab::Status:    DrawStatusTab(player, cx, cy, cw, ch); break;
    case Tab::Equipment: DrawEquipmentTab(player, events, cx, cy, cw, ch); break;
    case Tab::Skills:    DrawSkillTab(player, cx, cy, cw, ch); break;
    case Tab::Settings:  DrawSettingsTab(cx, cy, cw, ch); break;
    case Tab::System:    cmd = DrawSystemTab(cx, cy, cw, ch); break;
    default: break;
    }
    fm.DrawRight(x + w - 30, y + h - 30, "ESC: 閉じる   Q/E: タブ切替", ui::SubTextColor(), FontId::Small);
    // メニュー内のクリックがゲーム側に伝わらないようにする
    if (in.MousePressed(MouseButton::Left)) in.ConsumeMouse();
    return cmd;
}

// ---------------------------------------------------------------------------

void Menu::DrawStatusTab(Player& player, int x, int y, int w, int /*h*/)
{
    const FontManager& fm = FontManager::Get();
    const Stats& s = player.stats;
    ui::Panel(x, y, 700, 640, 160, false);
    int ly = y + 20;
    auto row = [&](const std::string& label, const std::string& value, unsigned int color) {
        fm.Draw(x + 30, ly, label, ui::SubTextColor());
        fm.DrawRight(x + 670, ly, value, color);
        DrawLine(x + 24, ly + 34, x + 676, ly + 34, GetColor(40, 46, 70), 1);
        ly += 46;
    };
    const unsigned int tc = ui::TextColor();
    row("名前", player.name, tc);
    row("レベル", Format("%d", player.level), ui::Gold());
    row("経験値", Format("%d / %d", player.exp, ExpToNextLevel(player.level)), tc);
    row("HP", Format("%d / %d", player.hp, s.maxHP), GetColor(130, 230, 130));
    row("MP", Format("%d / %d", player.mp, s.maxMP), GetColor(140, 180, 255));
    row("攻撃力 (ATK)", Format("%d", s.atk), tc);
    row("防御力 (DEF)", Format("%d", s.def), tc);
    row("クリティカル率", Format("%.1f %%", s.critRate * 100.0f), tc);
    row("クリティカル倍率", Format("x%.2f", s.critDamage), tc);
    row("移動速度", Format("%.0f", s.moveSpeed), tc);
    row("武器種", GetWeaponTypeInfo(player.CurrentWeapon()).name, tc);
    row("所持金", Format("%d G", player.gold), GetColor(255, 220, 120));

    const int rx = x + 740;
    ui::Panel(rx, y, w - 740, 640, 160, false);
    const char* help[] = {
        "■ ダメージ計算",
        "  ダメージ = ATK × 技倍率 × 100 / (100 + 相手DEF)",
        "  ±10% の揺らぎ、クリティカル時は ×クリティカル倍率",
        "",
        "■ 装備の個体値（IV）",
        "  ボスからドロップした装備は能力ごとに 0～100 の個体値を持つ",
        "  能力値 = 基礎値 × (80% ～ 120%)",
        "  耐久値 = 基礎耐久 × (70% ～ 130%)",
        "  平均値でランク S / A / B / C を表示",
        "",
        "■ 耐久値",
        "  武器は攻撃が命中するたび、防具は被弾するたびに減少",
        "  0 になると「破損」し性能が 50% に低下",
        "  （修理は町の鍛冶屋で今後実装予定）",
        "",
        "■ 槍は両手持ちのため、盾の効果が無効になる",
    };
    int hy = y + 20;
    for (const char* line : help) {
        fm.Draw(rx + 24, hy, line, line[0] == '\0' ? tc : (line[0] == static_cast<char>(0xE2) ? ui::Gold() : ui::SubTextColor()), FontId::Small);
        hy += 32;
    }
}

// ---------------------------------------------------------------------------

void Menu::DrawEquipmentTab(Player& player, EventSink& events, int x, int y, int w, int h)
{
    const FontManager& fm = FontManager::Get();
    Input& in = Input::Get();

    // ---- 装備スロット ----
    const int ew = 440;
    ui::Panel(x, y, ew, h, 160, false);
    fm.Draw(x + 20, y + 14, "装備中", ui::Gold(), FontId::Medium);
    for (int i = 0; i < Equipment::kSlotCount; ++i) {
        const EquipSlot slot = static_cast<EquipSlot>(i);
        const int sy = y + 64 + i * 112;
        const bool sel = selectedSlot_ == slot;
        const bool hover = ui::MouseIn(x + 14, sy, ew - 28, 100);
        DrawBox(x + 14, sy, x + ew - 14, sy + 100, sel ? GetColor(60, 52, 28) : (hover ? GetColor(36, 46, 80) : GetColor(22, 28, 48)), TRUE);
        DrawBox(x + 14, sy, x + ew - 14, sy + 100, sel ? ui::Gold() : ui::GoldDark(), FALSE);
        fm.Draw(x + 26, sy + 8, EquipSlotName(slot), ui::SubTextColor(), FontId::Small);
        const ItemInstance& it = player.equipment.Get(slot);
        if (it.Valid()) {
            const ItemDef* d = it.Def();
            fm.Draw(x + 26, sy + 32, d->name, RarityColor(d->rarity));
            fm.DrawRight(x + ew - 26, sy + 8, Format("個体値 %c", it.IvRank()), RankColor(it.IvRank()), FontId::Small);
            std::string dur = Format("耐久 %d/%d", it.durability, it.maxDurability);
            if (it.IsBroken()) dur += "（破損）";
            fm.Draw(x + 26, sy + 64, dur, it.IsBroken() ? GetColor(255, 90, 90) : ui::SubTextColor(), FontId::Small);
            if (slot == EquipSlot::Shield && player.equipment.IsShieldDisabled())
                fm.DrawRight(x + ew - 26, sy + 64, "両手武器のため無効", GetColor(255, 160, 90), FontId::Small);
        } else {
            fm.Draw(x + 26, sy + 32, "― なし ―", GetColor(110, 110, 120));
        }
        if (hover && in.MousePressed(MouseButton::Left)) {
            selectedSlot_ = slot;
            selectedInv_ = -1;
            in.ConsumeMouse();
        }
    }
    fm.Draw(x + 20, y + h - 40, Format("ATK %d  DEF %d  HP %d", player.stats.atk, player.stats.def, player.stats.maxHP), ui::TextColor(), FontId::Small);

    // ---- 所持品リスト ----
    const int lx = x + ew + 20, lw = 560;
    ui::Panel(lx, y, lw, h, 160, false);
    fm.Draw(lx + 20, y + 14, Format("所持品 %d/%d", player.inventory.Size(), Inventory::kCapacity), ui::Gold(), FontId::Medium);
    const char* filters[] = { "全て", "装備", "消費", "素材" };
    for (int i = 0; i < 4; ++i) {
        if (ui::Button(lx + 250 + i * 74, y + 14, 68, 36, filters[i], filter_ == i, true, FontId::Small)) {
            filter_ = i;
            scroll_ = 0;
            selectedInv_ = -1;
        }
    }
    std::vector<int> visible;
    for (int i = 0; i < player.inventory.Size(); ++i)
        if (PassFilter(player.inventory.At(i).Def(), filter_)) visible.push_back(i);

    const int rowH = 40, listY = y + 66;
    const int maxRows = (h - 130) / rowH;
    if (ui::MouseIn(lx, y, lw, h) && in.Wheel() != 0) scroll_ -= in.Wheel();
    scroll_ = ClampI(scroll_, 0, std::max(0, static_cast<int>(visible.size()) - maxRows));

    for (int r = 0; r < maxRows && r + scroll_ < static_cast<int>(visible.size()); ++r) {
        const int idx = visible[r + scroll_];
        const ItemInstance& it = player.inventory.At(idx);
        const ItemDef* d = it.Def();
        const int ry = listY + r * rowH;
        const bool sel = selectedInv_ == idx;
        const bool hover = ui::MouseIn(lx + 12, ry, lw - 24, rowH - 4);
        DrawBox(lx + 12, ry, lx + lw - 12, ry + rowH - 4, sel ? GetColor(60, 52, 28) : (hover ? GetColor(36, 46, 80) : GetColor(20, 26, 44)), TRUE);
        std::string name = d->name;
        if (it.count > 1) name += Format("  x%d", it.count);
        fm.Draw(lx + 24, ry + 7, name, RarityColor(d->rarity), FontId::Small);
        if (d->IsEquipment()) fm.DrawRight(lx + lw - 24, ry + 7, Format("%s  %c", CategoryName(d->category), it.IvRank()), RankColor(it.IvRank()), FontId::Small);
        else fm.DrawRight(lx + lw - 24, ry + 7, CategoryName(d->category), ui::SubTextColor(), FontId::Small);
        if (hover && in.MousePressed(MouseButton::Left)) {
            selectedInv_ = idx;
            selectedSlot_ = EquipSlot::None;
            in.ConsumeMouse();
        }
    }
    if (visible.empty()) fm.DrawCentered(lx + lw / 2, listY + 40, "アイテムがありません", ui::SubTextColor());
    if (ui::Button(lx + 20, y + h - 52, 160, 38, "並べ替え", false, true, FontId::Small)) {
        player.inventory.SortByCategory();
        selectedInv_ = -1;
    }

    // ---- 詳細 ----
    const int dx = lx + lw + 20, dw = w - (dx - x);
    ui::Panel(dx, y, dw, h, 160, false);
    if (selectedInv_ >= 0 && selectedInv_ < player.inventory.Size()) {
        DrawItemDetail(player, events, player.inventory.At(selectedInv_), selectedInv_, EquipSlot::None, dx, y, dw, h);
    } else if (selectedSlot_ != EquipSlot::None && player.equipment.Has(selectedSlot_)) {
        DrawItemDetail(player, events, player.equipment.Get(selectedSlot_), -1, selectedSlot_, dx, y, dw, h);
    } else {
        fm.DrawCentered(dx + dw / 2, y + h / 2 - 12, "アイテムを選択してください", ui::SubTextColor());
    }
}

void Menu::DrawItemDetail(Player& player, EventSink& events, const ItemInstance& itemRef, int invIndex, EquipSlot equippedSlot,
                          int x, int y, int w, int h)
{
    const FontManager& fm = FontManager::Get();
    const ItemInstance item = itemRef; // ボタン操作で元が消えても描画できるようコピー
    const ItemDef* d = item.Def();
    if (!d) return;

    int ly = y + 18;
    fm.Draw(x + 20, ly, d->name, RarityColor(d->rarity), FontId::Medium);
    ly += 42;
    fm.Draw(x + 20, ly, CategoryName(d->category), ui::SubTextColor(), FontId::Small);
    if (d->weaponType != WeaponType::None) fm.Draw(x + 140, ly, GetWeaponTypeInfo(d->weaponType).name, ui::SubTextColor(), FontId::Small);
    ly += 30;
    fm.Draw(x + 20, ly, d->description, ui::TextColor(), FontId::Small);
    ly += 40;

    if (d->IsEquipment()) {
        fm.Draw(x + 20, ly, Format("個体値ランク %c（平均 %d）", item.IvRank(), item.IvAverage()), RankColor(item.IvRank()));
        ly += 36;
        for (int i = 0; i < static_cast<int>(IvStat::Count); ++i) {
            fm.Draw(x + 30, ly, kIvNames[i], ui::SubTextColor(), FontId::Small);
            const int v = item.iv[i];
            DrawBox(x + 100, ly + 6, x + 100 + 300, ly + 18, GetColor(30, 30, 40), TRUE);
            const unsigned int c = v >= 85 ? GetColor(255, 200, 60) : v >= 65 ? GetColor(220, 120, 255) : v >= 40 ? GetColor(100, 170, 255) : GetColor(170, 170, 170);
            DrawBox(x + 100, ly + 6, x + 100 + 3 * v, ly + 18, c, TRUE);
            fm.Draw(x + 412, ly, Format("%d", v), c, FontId::Small);
            ly += 28;
        }
        ly += 8;
        fm.Draw(x + 20, ly, "性能", ui::Gold(), FontId::Small);
        ly += 28;
        fm.Draw(x + 30, ly, ItemStatSummary(item), ui::TextColor(), FontId::Small);
        ly += 34;

        // 装備中との比較
        if (invIndex >= 0) {
            const EquipSlot slot = SlotForCategory(d->category);
            const StatBonus nb = item.EffectiveBonus();
            const StatBonus ob = player.equipment.Has(slot) ? player.equipment.Get(slot).EffectiveBonus() : StatBonus{};
            auto diff = [&](const char* label, int nv, int ov) {
                if (nv == ov) return std::string();
                return Format("%s %+d  ", label, nv - ov);
            };
            std::string cmp = diff("ATK", nb.atk, ob.atk) + diff("DEF", nb.def, ob.def) + diff("HP", nb.maxHP, ob.maxHP);
            if (std::fabs(nb.critRate - ob.critRate) > 0.0005f) cmp += Format("CRI %+.1f%%", (nb.critRate - ob.critRate) * 100.0f);
            if (!cmp.empty()) {
                fm.Draw(x + 20, ly, "装備中との差", ui::Gold(), FontId::Small);
                ly += 28;
                fm.Draw(x + 30, ly, cmp, GetColor(160, 230, 160), FontId::Small);
                ly += 34;
            }
        }
    } else if (d->category == ItemCategory::Consumable) {
        if (player.quickItemId == d->id) fm.Draw(x + 20, ly, "クイックスロット(R)に登録中", GetColor(140, 230, 160), FontId::Small);
    }

    // 操作ボタン
    const int by = y + h - 64;
    if (invIndex >= 0) {
        if (d->IsEquipment()) {
            if (ui::Button(x + 20, by, 160, 44, "装備する")) {
                if (player.Equip(invIndex, events)) selectedInv_ = -1;
            }
        } else if (d->category == ItemCategory::Consumable) {
            if (ui::Button(x + 20, by, 140, 44, "使う")) {
                const int before = player.inventory.Size();
                player.UseItem(invIndex, events);
                if (player.inventory.Size() != before) selectedInv_ = -1;
            }
            if (ui::Button(x + 170, by, 160, 44, "R に登録", player.quickItemId == d->id, true, FontId::Small)) player.quickItemId = d->id;
        }
        if (ui::Button(x + w - 140, by, 120, 44, "捨てる")) {
            events.Info(d->name + " を捨てた");
            player.inventory.RemoveAt(invIndex, item.count);
            selectedInv_ = -1;
        }
    } else if (equippedSlot != EquipSlot::None) {
        if (ui::Button(x + 20, by, 160, 44, "外す")) {
            if (player.Unequip(equippedSlot, events)) selectedSlot_ = EquipSlot::None;
        }
    }
}

// ---------------------------------------------------------------------------

void Menu::DrawSkillTab(Player& player, int x, int y, int w, int h)
{
    const FontManager& fm = FontManager::Get();
    Input& in = Input::Get();
    const WeaponType wt = player.CurrentWeapon();
    const auto skills = SkillDatabase::Get().ForWeapon(wt);

    const int lw = 700;
    ui::Panel(x, y, lw, h, 160, false);
    fm.Draw(x + 20, y + 14, Format("%s のソードスキル", GetWeaponTypeInfo(wt).name), ui::Gold(), FontId::Medium);
    fm.Draw(x + 20, y + 52, "スキルを選んで、右のスロットをクリック（または 1～4 キー）で登録", ui::SubTextColor(), FontId::Small);

    for (size_t i = 0; i < skills.size(); ++i) {
        const SkillDef* s = skills[i];
        const int sy = y + 90 + static_cast<int>(i) * 120;
        const bool unlocked = player.IsSkillUnlocked(*s);
        const bool sel = selectedSkill_ == s->id;
        const bool hover = ui::MouseIn(x + 14, sy, lw - 28, 108);
        DrawBox(x + 14, sy, x + lw - 14, sy + 108, sel ? GetColor(60, 52, 28) : (hover ? GetColor(36, 46, 80) : GetColor(20, 26, 44)), TRUE);
        DrawBox(x + 14, sy, x + 22, sy + 108, PrimitiveRenderer::ToDx(s->effectColor), TRUE);
        fm.Draw(x + 36, sy + 8, s->name, unlocked ? ui::TextColor() : GetColor(120, 120, 120), FontId::Medium);
        if (!unlocked) fm.Draw(x + 260, sy + 14, Format("Lv%d で解放", s->requiredLevel), GetColor(255, 140, 100), FontId::Small);
        float total = 0.0f;
        for (const auto& hit : s->hits) total += hit.multiplier;
        fm.Draw(x + 36, sy + 46, Format("MP %d   CD %.1f秒   %d段 / 合計倍率 %.1f   射程 %.0f   %s", s->mpCost, s->cooldown,
                                         static_cast<int>(s->hits.size()), total, s->range, s->arcDeg >= 359.0f ? "全周" : "前方"),
                ui::SubTextColor(), FontId::Small);
        fm.Draw(x + 36, sy + 74, s->description, ui::TextColor(), FontId::Small);
        if (hover && in.MousePressed(MouseButton::Left)) {
            selectedSkill_ = s->id;
            in.ConsumeMouse();
        }
    }

    // スロット
    const int rx = x + lw + 30;
    ui::Panel(rx, y, w - lw - 30, h, 160, false);
    fm.Draw(rx + 20, y + 14, "スキルスロット", ui::Gold(), FontId::Medium);
    const ag::Action keys[] = { ag::Action::Skill1, ag::Action::Skill2, ag::Action::Skill3, ag::Action::Skill4 };
    for (int i = 0; i < kSkillSlotCount; ++i) {
        const int sx = rx + 30 + i * 190, sy = y + 80;
        const SkillDef* s = player.SlotSkill(i);
        const bool hover = ui::MouseIn(sx, sy, 170, 170);
        ui::Panel(sx, sy, 170, 170, 200);
        if (hover) DrawBox(sx, sy, sx + 170, sy + 170, ui::Gold(), FALSE);
        fm.Draw(sx + 10, sy + 8, Format("%d", i + 1), ui::Gold(), FontId::Medium);
        fm.DrawCentered(sx + 85, sy + 70, s ? s->name : "（空き）", s ? ui::TextColor() : GetColor(110, 110, 110));
        if (s) fm.DrawCentered(sx + 85, sy + 110, Format("MP %d / CD %.0fs", s->mpCost, s->cooldown), ui::SubTextColor(), FontId::Small);
        const bool assign = (hover && in.MousePressed(MouseButton::Left)) || in.ActionPressed(keys[i]);
        if (assign && !selectedSkill_.empty()) {
            const SkillDef* chosen = SkillDatabase::Get().Find(selectedSkill_);
            if (chosen && chosen->weapon == wt) player.SetSlotSkill(i, selectedSkill_);
            in.ConsumeMouse();
        } else if (hover && in.MousePressed(MouseButton::Right) && s) {
            player.SlotsFor(wt)[i].clear();
        }
    }
    fm.Draw(rx + 30, y + 280, "右クリックでスロットを空にする", ui::SubTextColor(), FontId::Small);
    fm.Draw(rx + 30, y + 320, "スロットは武器種ごとに保存されます。", ui::SubTextColor(), FontId::Small);
    fm.Draw(rx + 30, y + 350, "武器を持ち替えると、その武器種のスロットに切り替わります。", ui::SubTextColor(), FontId::Small);
    fm.Draw(rx + 30, y + 410, "■ ソードスキル", ui::Gold(), FontId::Small);
    fm.Draw(rx + 30, y + 440, "発動中はモーションが固定され、移動できません（被弾で怯まない）。", ui::SubTextColor(), FontId::Small);
    fm.Draw(rx + 30, y + 470, "MP を消費し、使用後はクールダウンが発生します。", ui::SubTextColor(), FontId::Small);
    fm.Draw(rx + 30, y + 500, "通常攻撃中にスキルを入力すると、攻撃をキャンセルして発動します。", ui::SubTextColor(), FontId::Small);
}

// ---------------------------------------------------------------------------

void Menu::DrawSettingsTab(int x, int y, int w, int h)
{
    GameSettings& s = Settings();
    const int lw = 860;
    ui::Panel(x, y, lw, h, 160, false);
    FontManager::Get().Draw(x + 20, y + 14, "ゲーム設定", ui::Gold(), FontId::Medium);
    int ly = y + 70;
    const int sx = x + 30, sw = lw - 60;
    settingsDirty_ |= ui::Slider(sx, ly, sw, "マウス感度", s.mouseSensitivity, 0.2f, 3.0f, "%.2f"); ly += 66;
    settingsDirty_ |= ui::Slider(sx, ly, sw, "カメラ距離", s.cameraDistance, 300.0f, 1400.0f, "%.0f"); ly += 66;
    float bgm = static_cast<float>(s.bgmVolume), se = static_cast<float>(s.seVolume);
    if (ui::Slider(sx, ly, sw, "BGM 音量", bgm, 0.0f, 100.0f, "%.0f")) { s.bgmVolume = static_cast<int>(bgm); SoundManager::Get().ApplyVolume(); settingsDirty_ = true; }
    ly += 66;
    if (ui::Slider(sx, ly, sw, "効果音 音量", se, 0.0f, 100.0f, "%.0f")) { s.seVolume = static_cast<int>(se); settingsDirty_ = true; }
    ly += 76;
    settingsDirty_ |= ui::Toggle(sx, ly, "カメラ上下反転", s.invertY); ly += 64;
    settingsDirty_ |= ui::Toggle(sx, ly, "ダメージ数値表示", s.showDamageNumbers); ly += 64;
    settingsDirty_ |= ui::Toggle(sx, ly, "フルスクリーン（次回起動時）", s.fullscreen); ly += 80;
    FontManager::Get().Draw(sx, ly, Format("使用フォント: %s", FontManager::Get().FaceName().c_str()), ui::SubTextColor(), FontId::Small);
    FontManager::Get().Draw(sx, ly + 30, "設定はメニューを閉じたときに save/config.ini へ保存されます。", ui::SubTextColor(), FontId::Small);

    DrawKeyConfig(x + lw + 20, y, w - lw - 20, h);
}

void Menu::DrawKeyConfig(int x, int y, int w, int h)
{
    const FontManager& fm = FontManager::Get();
    Input& in = Input::Get();
    ui::Panel(x, y, w, h, 160, false);
    fm.Draw(x + 20, y + 14, "キー配置", ui::Gold(), FontId::Medium);

    // 入力待ち: 次に押されたキーを割り当てる（ESC でキャンセル）
    if (waitingKey_ >= 0) {
        const int code = in.DetectPressedKey();
        if (code == KEY_INPUT_ESCAPE) {
            waitingKey_ = -1;
        } else if (code >= 0) {
            in.Rebind(static_cast<Action>(waitingKey_), code);
            waitingKey_ = -1;
            settingsDirty_ = true;
        }
    }

    const int rowH = 36;
    int ry = y + 60;
    for (int i = 0; i < static_cast<int>(Action::Count); ++i) {
        const Action a = static_cast<Action>(i);
        if (!Input::IsRebindable(a)) continue;
        fm.Draw(x + 30, ry + 6, Input::ActionName(a), ui::TextColor(), FontId::Small);
        const bool waiting = waitingKey_ == i;
        const char* name = Input::KeyName(in.ActionKey(a));
        const std::string label = waiting ? "キーを押してください" : (name ? name : "?");
        if (ui::Button(x + 300, ry, w - 330, rowH - 6, label, waiting, true, FontId::Small)) waitingKey_ = i;
        ry += rowH;
    }
    ry += 6;
    fm.Draw(x + 30, ry, "通常攻撃・パリィ: 左クリック（固定）", ui::SubTextColor(), FontId::Small); ry += 28;
    fm.Draw(x + 30, ry, "ガード: 右クリック長押し（固定）", ui::SubTextColor(), FontId::Small); ry += 28;
    fm.Draw(x + 30, ry, "メニュー: ESC / 決定: Enter（固定）", ui::SubTextColor(), FontId::Small);
    if (ui::Button(x + w - 250, y + 14, 220, 38, "初期設定に戻す", false, true, FontId::Small)) {
        in.ResetBindings();
        waitingKey_ = -1;
        settingsDirty_ = true;
    }
    if (waitingKey_ >= 0) fm.DrawRight(x + w - 30, y + h - 34, "割り当てるキーを押してください（ESC でキャンセル）", GetColor(255, 220, 120), FontId::Small);
    else fm.DrawRight(x + w - 30, y + h - 34, "他の操作と重なったキーは入れ替わります", ui::SubTextColor(), FontId::Small);
}

MenuCommand Menu::DrawSystemTab(int x, int y, int w, int h)
{
    const FontManager& fm = FontManager::Get();
    ui::Panel(x, y, w, h, 160, false);
    const int bx = x + w / 2 - 200;
    int by = y + 60;
    MenuCommand cmd = MenuCommand::None;
    if (ui::Button(bx, by, 400, 64, "セーブ (F5)", false, true, FontId::Medium)) cmd = MenuCommand::Save;
    by += 90;
    if (ui::Button(bx, by, 400, 64, "ロード (F9)", false, true, FontId::Medium)) cmd = MenuCommand::Load;
    by += 90;
    if (ui::Button(bx, by, 400, 64, "タイトルへ戻る", false, true, FontId::Medium)) cmd = MenuCommand::ToTitle;
    by += 90;
    if (!confirmQuit_) {
        if (ui::Button(bx, by, 400, 64, "ゲーム終了", false, true, FontId::Medium)) confirmQuit_ = true;
    } else {
        fm.DrawCentered(x + w / 2, by - 4, "セーブしていない進行状況は失われます。終了しますか？", GetColor(255, 200, 120));
        if (ui::Button(bx, by + 30, 190, 56, "終了する")) cmd = MenuCommand::Quit;
        if (ui::Button(bx + 210, by + 30, 190, 56, "キャンセル")) confirmQuit_ = false;
    }
    fm.DrawCentered(x + w / 2, y + h - 80, Format("%s  Ver. %s", kGameTitle, kVersion), ui::SubTextColor(), FontId::Small);
    fm.DrawCentered(x + w / 2, y + h - 50, Format("セーブファイル: %s", kSavePath), ui::SubTextColor(), FontId::Small);
    return cmd;
}

} // namespace ag
