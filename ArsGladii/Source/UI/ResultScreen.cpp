#include "ResultScreen.h"
#include "UIDraw.h"
#include "../Core/Config.h"
#include "../Core/Input.h"
#include "../Core/MathUtil.h"
#include "../Item/ItemDatabase.h"
#include "DxLib.h"
#include <cmath>

namespace ag {

std::string ItemStatSummary(const ItemInstance& item)
{
    const ItemDef* d = item.Def();
    if (!d) return "";
    if (!d->IsEquipment()) return d->description;
    const StatBonus b = item.EffectiveBonus();
    std::string s;
    if (b.atk) s += Format("ATK %d  ", b.atk);
    if (b.def) s += Format("DEF %d  ", b.def);
    if (b.maxHP) s += Format("HP+%d  ", b.maxHP);
    if (b.critRate > 0.0f) s += Format("CRI %.1f%%  ", b.critRate * 100.0f);
    if (b.critDamage > 0.0f) s += Format("CRI倍率+%.0f%%  ", b.critDamage * 100.0f);
    if (b.moveSpeed != 0.0f) s += Format("移動%+.0f  ", b.moveSpeed);
    s += Format("耐久 %d/%d", item.durability, item.maxDurability);
    return s;
}

void ResultScreen::Open(const std::string& bossName, float fightTime, int exp, int gold, int levelsGained,
                        const std::vector<ItemInstance>& drops)
{
    open_ = true;
    time_ = 0.0f;
    bossName_ = bossName;
    fightTime_ = fightTime;
    exp_ = exp;
    gold_ = gold;
    levels_ = levelsGained;
    drops_ = drops;
    closeRequested_ = false;
}

void ResultScreen::Update(float dt)
{
    if (!open_) return;
    time_ += dt;
    Input& in = Input::Get();
    if (time_ > 0.8f && (in.ActionPressed(Action::Confirm) || in.ActionPressed(Action::Menu) || closeRequested_)) {
        open_ = false;
        in.ConsumeMouse();
    }
}

void ResultScreen::Draw() const
{
    if (!open_) return;
    const FontManager& fm = FontManager::Get();
    const float appear = Clamp(time_ / 0.4f, 0.0f, 1.0f);
    ui::DimScreen(static_cast<int>(150 * appear));

    const int w = 980;
    const int rows = static_cast<int>(drops_.size());
    const int h = 360 + rows * 52;
    const int x = (kScreenWidth - w) / 2;
    const int y = (kScreenHeight - h) / 2 + static_cast<int>((1.0f - appear) * 40);
    ui::Panel(x, y, w, h, 235);

    // 見出し
    const float glow = 0.7f + 0.3f * std::sin(time_ * 4.0f);
    fm.DrawCentered(kScreenWidth / 2, y + 22, "BOSS DEFEATED", GetColor(255, static_cast<int>(200 * glow), 80), FontId::Large, GetColor(40, 20, 0));
    fm.DrawCentered(kScreenWidth / 2, y + 76, bossName_ + " を討伐した！", ui::TextColor(), FontId::Medium, GetColor(0, 0, 0));
    DrawLine(x + 40, y + 122, x + w - 40, y + 122, ui::GoldDark(), 1);

    const int m = static_cast<int>(fightTime_) / 60, s = static_cast<int>(fightTime_) % 60;
    fm.Draw(x + 60, y + 138, Format("討伐時間  %02d:%02d", m, s), ui::TextColor());
    fm.Draw(x + 360, y + 138, Format("EXP  +%d", exp_), GetColor(160, 230, 255));
    fm.Draw(x + 620, y + 138, Format("%d G", gold_), GetColor(255, 220, 120));
    if (levels_ > 0) fm.DrawCentered(kScreenWidth / 2, y + 176, Format("レベルアップ！ (+%d)", levels_), GetColor(255, 240, 120), FontId::Medium, GetColor(0, 0, 0));

    fm.Draw(x + 60, y + 220, "獲得アイテム", ui::Gold(), FontId::Medium);
    for (int i = 0; i < rows; ++i) {
        const float t = Clamp((time_ - 0.5f - i * 0.25f) / 0.3f, 0.0f, 1.0f);
        if (t <= 0.0f) continue;
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * t));
        const ItemInstance& it = drops_[i];
        const ItemDef* d = it.Def();
        const int ry = y + 264 + i * 52;
        DrawBox(x + 50, ry - 4, x + w - 50, ry + 44, GetColor(20, 28, 50), TRUE);
        if (d) {
            std::string name = d->name;
            if (it.count > 1) name += Format(" x%d", it.count);
            fm.Draw(x + 64, ry, name, RarityColor(d->rarity));
            if (d->IsEquipment()) {
                fm.Draw(x + 64 + 300, ry, Format("個体値 %c (%d)", it.IvRank(), it.IvAverage()), RankColor(it.IvRank()));
                fm.Draw(x + 64 + 300, ry + 24, ItemStatSummary(it), ui::SubTextColor(), FontId::Small);
            } else {
                fm.Draw(x + 64 + 300, ry + 4, CategoryName(d->category), ui::SubTextColor(), FontId::Small);
            }
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
    if (time_ > 0.8f) {
        if (ui::Button(kScreenWidth / 2 - 120, y + h - 70, 240, 48, "閉じる (Enter)")) closeRequested_ = true;
    }
}

} // namespace ag
