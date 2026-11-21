#include "HUD.h"
#include "UIDraw.h"
#include "../Entity/Player.h"
#include "../World/World.h"
#include "../Core/Camera.h"
#include "../Core/Config.h"
#include "../Core/Input.h"
#include "../Core/MathUtil.h"
#include "../Core/AssetManager.h"
#include "../Item/ItemDatabase.h"
#include "DxLib.h"
#include <cmath>
#include <algorithm>

namespace ag {

void HUD::Update(float dt, const Player& player, const World& world)
{
    time_ += dt;
    const float hpRate = player.HpRate();
    hpTrail_ = (hpTrail_ > hpRate) ? std::max(hpRate, hpTrail_ - dt * 0.4f) : hpRate;
    if (const Enemy* boss = world.ActiveBoss()) {
        const float r = boss->HpRate();
        bossTrail_ = (bossTrail_ > r) ? std::max(r, bossTrail_ - dt * 0.25f) : r;
    } else {
        bossTrail_ = 1.0f;
    }
    warningTimer_ = std::max(0.0f, warningTimer_ - dt);
    bannerTimer_ = std::max(0.0f, bannerTimer_ - dt);
    if (Input::Get().KeyPressed(KEY_INPUT_F1)) ToggleHelp();
}

void HUD::ShowWarning(const std::string& text)
{
    warning_ = text;
    warningTimer_ = 1.6f;
}

void HUD::ShowBanner(const std::string& title, const std::string& sub, unsigned int color)
{
    bannerTitle_ = title;
    bannerSub_ = sub;
    bannerColor_ = color;
    bannerTimer_ = 3.0f;
}

void HUD::Draw(const Player& player, const World& world, const Camera& camera) const
{
    DrawStatus(player);
    DrawBossBar(world);
    DrawMinimap(player, world, camera);
    DrawHotbar(player);
    DrawCenterTexts();
    if (showHelp_) DrawHelp();
}

void HUD::DrawIcon(int x, int y, int size) const
{
    const int cx = x + size / 2, cy = y + size / 2, r = size / 2;
    const int img = AssetManager::Get().GetImage("Assets/UI/player_icon.png");
    DrawCircle(cx, cy, r + 4, ui::GoldDark(), TRUE);
    DrawCircle(cx, cy, r + 2, ui::Gold(), TRUE);
    if (img != -1) {
        DrawCircle(cx, cy, r, GetColor(10, 14, 28), TRUE);
        DrawExtendGraph(x + 6, y + 6, x + size - 6, y + size - 6, img, TRUE);
    } else {
        // サンプルアイコン: 交差した剣の紋章
        DrawCircle(cx, cy, r, GetColor(22, 36, 72), TRUE);
        DrawCircle(cx, cy, r - 8, GetColor(36, 58, 110), TRUE);
        const unsigned int blade = GetColor(225, 230, 240);
        DrawLine(cx - r / 2, cy + r / 2, cx + r / 2, cy - r / 2, blade, 6);
        DrawLine(cx + r / 2, cy + r / 2, cx - r / 2, cy - r / 2, blade, 6);
        DrawLine(cx - r / 2 + 6, cy + r / 4 + 2, cx - r / 4 - 2, cy + r / 2 - 6, ui::Gold(), 5);
        DrawLine(cx + r / 2 - 6, cy + r / 4 + 2, cx + r / 4 + 2, cy + r / 2 - 6, ui::Gold(), 5);
        DrawCircle(cx, cy, 8, ui::Gold(), TRUE);
    }
    DrawCircle(cx, cy, r + 4, GetColor(0, 0, 0), FALSE);
}

void HUD::DrawStatus(const Player& player) const
{
    const FontManager& fm = FontManager::Get();
    const int x = 24, y = 24;
    // 斜めカットの背景プレート
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 170);
    DrawBox(x + 60, y + 14, x + 560, y + 128, GetColor(10, 14, 30), TRUE);
    DrawTriangle(x + 560, y + 14, x + 600, y + 14, x + 560, y + 128, GetColor(10, 14, 30), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawLine(x + 60, y + 14, x + 600, y + 14, ui::Gold(), 2);
    DrawLine(x + 60, y + 128, x + 560, y + 128, ui::GoldDark(), 1);

    DrawIcon(x, y, 140);

    const int bx = x + 160;
    fm.Draw(bx, y + 22, Format("Lv %d", player.level), ui::Gold(), FontId::Medium, GetColor(0, 0, 0));
    fm.Draw(bx + 100, y + 26, player.name, ui::TextColor(), FontId::Normal, GetColor(0, 0, 0));

    const float hpRate = player.HpRate();
    unsigned int hpColor = GetColor(80, 210, 90);
    if (hpRate < 0.5f) hpColor = GetColor(230, 200, 60);
    if (hpRate < 0.25f) hpColor = GetColor(230, 70, 60);
    ui::Gauge(bx, y + 62, 360, 22, hpRate, hpColor, GetColor(40, 20, 20), hpTrail_);
    fm.Draw(bx + 6, y + 61, "HP", GetColor(255, 255, 255), FontId::Small, GetColor(0, 0, 0));
    fm.DrawRight(bx + 354, y + 61, Format("%d / %d", player.hp, player.stats.maxHP), GetColor(255, 255, 255), FontId::Small, GetColor(0, 0, 0));

    ui::Gauge(bx, y + 92, 300, 14, player.MpRate(), GetColor(70, 130, 240), GetColor(20, 20, 40));
    fm.Draw(bx + 310, y + 87, Format("MP %d/%d", player.mp, player.stats.maxMP), GetColor(170, 200, 255), FontId::Small, GetColor(0, 0, 0));

    const float expRate = static_cast<float>(player.exp) / ExpToNextLevel(player.level);
    DrawBox(bx, y + 114, bx + 360, y + 119, GetColor(30, 30, 30), TRUE);
    DrawBox(bx, y + 114, bx + static_cast<int>(360 * Clamp(expRate, 0.0f, 1.0f)), y + 119, ui::Gold(), TRUE);

    // バフ・状態
    int sx = x + 10;
    const int sy = y + 150;
    auto chip = [&](const std::string& text, unsigned int color) {
        const int w = fm.Width(text, FontId::Small) + 16;
        ui::Panel(sx, sy, w, 28, 180, false);
        DrawBox(sx, sy, sx + w, sy + 28, color, FALSE);
        fm.Draw(sx + 8, sy + 4, text, color, FontId::Small);
        sx += w + 8;
    };
    if (player.atkBuffTime > 0.0f) chip(Format("ATK↑ %.0fs", player.atkBuffTime), GetColor(255, 150, 90));
    if (player.defBuffTime > 0.0f) chip(Format("DEF↑ %.0fs", player.defBuffTime), GetColor(120, 200, 255));
    if (player.CounterTime() > 0.0f) chip(Format("カウンター x1.5 %.1fs", player.CounterTime()), GetColor(255, 230, 90));
    if (player.IsGuarding()) chip(player.IsParryWindow() ? "パリィ受付中" : "ガード中", player.IsParryWindow() ? GetColor(255, 230, 90) : GetColor(150, 190, 255));
    if (player.inTown) chip("安全地帯（回復量UP）", GetColor(140, 230, 160));
    const ItemInstance& w = player.equipment.Get(EquipSlot::Weapon);
    if (w.Valid() && w.IsBroken()) chip("武器破損", GetColor(255, 80, 80));
    else if (w.Valid() && w.maxDurability > 0 && w.durability * 5 < w.maxDurability) chip("武器の耐久値低下", GetColor(255, 200, 80));
}

void HUD::DrawBossBar(const World& world) const
{
    const Enemy* boss = world.ActiveBoss();
    if (!boss) return;
    const FontManager& fm = FontManager::Get();
    const int w = 900, h = 26;
    const int x = (kScreenWidth - w) / 2, y = 40;
    ui::Panel(x - 20, y - 8, w + 40, 90, 170);
    const std::string name = boss->Def().title.empty() ? boss->Def().name : boss->Def().title + "  " + boss->Def().name;
    fm.DrawCentered(kScreenWidth / 2, y, name, GetColor(255, 210, 190), FontId::Medium, GetColor(0, 0, 0));
    const unsigned int barColor = boss->IsEnraged() ? GetColor(230, 40, 120) : GetColor(200, 40, 40);
    ui::Gauge(x, y + 40, w, h, boss->HpRate(), barColor, GetColor(30, 10, 10), bossTrail_);
    fm.DrawRight(x + w - 6, y + 41, Format("%d / %d", boss->hp, boss->stats.maxHP), GetColor(255, 255, 255), FontId::Small, GetColor(0, 0, 0));
    fm.Draw(x + 6, y + 41, Format("Lv%d", boss->Def().level), GetColor(255, 255, 255), FontId::Small, GetColor(0, 0, 0));
    if (boss->IsEnraged()) fm.DrawCentered(kScreenWidth / 2, y + 72, "― 激昂 ―", GetColor(255, 90, 140), FontId::Small, GetColor(0, 0, 0));
}

void HUD::DrawMinimap(const Player& player, const World& world, const Camera& camera) const
{
    const FontManager& fm = FontManager::Get();
    const int size = 280;
    const int x = kScreenWidth - size - 30, y = 30;
    const int cx = x + size / 2, cy = y + size / 2;
    const float scale = size / 2.0f / 5000.0f; // 半径 50m 相当を表示

    ui::Panel(x - 6, y - 6, size + 12, size + 12, 190);
    SetDrawArea(x, y, x + size, y + size);
    auto toMap = [&](const VECTOR& p, int& mx, int& my) {
        mx = cx + static_cast<int>((p.x - player.pos.x) * scale);
        my = cy - static_cast<int>((p.z - player.pos.z) * scale);
    };
    const FloorDef& f = world.Floor();
    int mx, my;
    toMap(f.townCenter, mx, my);
    DrawCircle(mx, my, static_cast<int>(f.townRadius * scale), GetColor(70, 110, 160), TRUE);
    toMap(f.bossPos, mx, my);
    DrawCircle(mx, my, static_cast<int>(f.bossArenaRadius * scale), GetColor(110, 40, 40), TRUE);
    // フィールド境界
    int bx0, by0, bx1, by1;
    toMap(VGet(-f.halfSize, 0, f.halfSize), bx0, by0);
    toMap(VGet(f.halfSize, 0, -f.halfSize), bx1, by1);
    DrawBox(bx0, by0, bx1, by1, GetColor(160, 140, 90), FALSE);

    for (const auto& e : world.Enemies()) {
        if (e->IsDead()) continue;
        toMap(e->pos, mx, my);
        if (e->IsBoss()) DrawCircle(mx, my, 8, GetColor(220, 60, 220), TRUE);
        else DrawCircle(mx, my, 3, e->IsEngaged() ? GetColor(255, 80, 60) : GetColor(230, 150, 60), TRUE);
    }
    // 視界の向き
    const float cy0 = camera.Yaw();
    const int len = 60;
    DrawLine(cx, cy, cx + static_cast<int>(std::sin(cy0 - 0.5f) * len), cy - static_cast<int>(std::cos(cy0 - 0.5f) * len), GetColor(200, 200, 160), 1);
    DrawLine(cx, cy, cx + static_cast<int>(std::sin(cy0 + 0.5f) * len), cy - static_cast<int>(std::cos(cy0 + 0.5f) * len), GetColor(200, 200, 160), 1);
    // プレイヤー（向き付き三角形）
    const float a = player.yaw;
    const int tx = cx + static_cast<int>(std::sin(a) * 12), ty = cy - static_cast<int>(std::cos(a) * 12);
    const int lx = cx + static_cast<int>(std::sin(a + 2.5f) * 8), ly = cy - static_cast<int>(std::cos(a + 2.5f) * 8);
    const int rx = cx + static_cast<int>(std::sin(a - 2.5f) * 8), ry = cy - static_cast<int>(std::cos(a - 2.5f) * 8);
    DrawTriangle(tx, ty, lx, ly, rx, ry, GetColor(255, 255, 255), TRUE);
    SetDrawArea(0, 0, kScreenWidth, kScreenHeight);

    fm.DrawCentered(cx, y + size + 12, f.name, ui::Gold(), FontId::Small, GetColor(0, 0, 0));
    const char* area = player.inTown ? f.townName.c_str() : (world.GetField().InBossArena(player.pos) ? "ボスエリア" : "フィールド");
    fm.DrawCentered(cx, y + size + 36, area, ui::TextColor(), FontId::Small, GetColor(0, 0, 0));
    fm.DrawCentered(cx, y + size + 60, Format("%d G", player.gold), GetColor(255, 220, 120), FontId::Small, GetColor(0, 0, 0));
}

void HUD::DrawHotbar(const Player& player) const
{
    const FontManager& fm = FontManager::Get();
    const int slot = 92, gap = 12;
    const int count = kSkillSlotCount + 2; // スキル4 + 回避 + アイテム
    const int totalW = count * slot + (count - 1) * gap;
    int x = (kScreenWidth - totalW) / 2;
    const int y = kScreenHeight - slot - 36;
    auto keyName = [](Action a) { const char* n = Input::KeyName(Input::Get().ActionKey(a)); return std::string(n ? n : "?"); };
    const std::string keys[] = { keyName(Action::Skill1), keyName(Action::Skill2), keyName(Action::Skill3), keyName(Action::Skill4) };

    auto drawSlot = [&](int sx, const std::string& key, const std::string& label, const std::string& sub,
                        float cdRate, float cdRemain, bool enabled, unsigned int accent) {
        ui::Panel(sx, y, slot, slot, 200);
        DrawBox(sx + 4, y + 4, sx + slot - 4, y + slot - 4, accent, FALSE);
        fm.Draw(sx + 6, y + 4, key, ui::Gold(), FontId::Small, GetColor(0, 0, 0));
        // ラベルは2行まで（長い場合は縮小せず中央寄せ）
        fm.DrawCentered(sx + slot / 2, y + 32, label, enabled ? ui::TextColor() : GetColor(110, 110, 110), FontId::Small, GetColor(0, 0, 0));
        fm.DrawCentered(sx + slot / 2, y + 62, sub, ui::SubTextColor(), FontId::Small, GetColor(0, 0, 0));
        if (cdRate > 0.0f) {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 170);
            DrawBox(sx + 2, y + 2 + static_cast<int>((slot - 4) * (1.0f - cdRate)), sx + slot - 2, y + slot - 2, GetColor(0, 0, 0), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            if (cdRemain > 0.0f) fm.DrawCentered(sx + slot / 2, y + slot / 2 - 14, Format("%.1f", cdRemain), GetColor(255, 255, 255), FontId::Medium, GetColor(0, 0, 0));
        }
    };

    for (int i = 0; i < kSkillSlotCount; ++i) {
        const SkillDef* s = player.SlotSkill(i);
        if (!s) {
            drawSlot(x, keys[i], "（空き）", "", 0.0f, 0.0f, false, GetColor(60, 60, 70));
        } else {
            const float remain = player.CooldownRemaining(s->id);
            const bool unlocked = player.IsSkillUnlocked(*s);
            const bool mpOk = player.mp >= s->mpCost;
            drawSlot(x, keys[i], s->name, unlocked ? Format("MP %d", s->mpCost) : Format("Lv%d解放", s->requiredLevel),
                     s->cooldown > 0.0f ? remain / s->cooldown : 0.0f, remain, unlocked && mpOk,
                     PrimitiveRenderer::ToDx(s->effectColor));
        }
        x += slot + gap;
    }
    drawSlot(x, keyName(Action::Dodge), "回避", "", player.DodgeCooldownRate(), 0.0f, true, GetColor(150, 150, 170));
    x += slot + gap;
    const ItemDef* q = ItemDatabase::Get().Find(player.quickItemId);
    const int n = player.inventory.CountOf(player.quickItemId);
    drawSlot(x, keyName(Action::UseItem), q ? q->name : "-", Format("x %d", n), 0.0f, 0.0f, n > 0, GetColor(90, 200, 120));

    // 武器種表示
    fm.DrawCentered(kScreenWidth / 2, y - 30, Format("武器: %s", GetWeaponTypeInfo(player.CurrentWeapon()).name), ui::SubTextColor(), FontId::Small, GetColor(0, 0, 0));
}

void HUD::DrawCenterTexts() const
{
    const FontManager& fm = FontManager::Get();
    if (warningTimer_ > 0.0f) {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * Clamp(warningTimer_ / 0.4f, 0.0f, 1.0f)));
        fm.DrawCentered(kScreenWidth / 2, kScreenHeight / 2 + 160, warning_, GetColor(255, 220, 120), FontId::Medium, GetColor(0, 0, 0));
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
    if (bannerTimer_ > 0.0f) {
        const float a = Clamp(std::min(bannerTimer_, 3.0f - bannerTimer_) / 0.4f, 0.0f, 1.0f);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(200 * a));
        DrawBox(0, 300, kScreenWidth, 440, GetColor(0, 0, 0), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * a));
        DrawLine(0, 300, kScreenWidth, 300, bannerColor_, 2);
        DrawLine(0, 440, kScreenWidth, 440, bannerColor_, 2);
        fm.DrawCentered(kScreenWidth / 2, 320, bannerTitle_, bannerColor_, FontId::Large, GetColor(0, 0, 0));
        fm.DrawCentered(kScreenWidth / 2, 384, bannerSub_, ui::TextColor(), FontId::Normal, GetColor(0, 0, 0));
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
}

void HUD::DrawHelp() const
{
    const FontManager& fm = FontManager::Get();
    auto k = [](Action a) { const char* n = Input::KeyName(Input::Get().ActionKey(a)); return n ? n : "?"; };
    const std::string lines[] = {
        Format("%s%s%s%s: 移動  %s: ダッシュ  %s: 回避  %s: ジャンプ", k(Action::MoveForward), k(Action::MoveLeft), k(Action::MoveBack), k(Action::MoveRight),
               k(Action::Dash), k(Action::Dodge), k(Action::Jump)),
        Format("左クリック: 攻撃  %s～%s: ソードスキル  %s: 回復薬", k(Action::Skill1), k(Action::Skill4), k(Action::UseItem)),
        "右長押し: ガード  ガード中に攻撃の瞬間 左クリック: パリィ",
        "マウス移動: 視点  ホイール: ズーム  Alt: カーソル表示",
        Format("ESC: メニュー  %s: セーブ  %s: ロード  F1: ヘルプ切替", k(Action::QuickSave), k(Action::QuickLoad)),
    };
    const int n = 5;
    const int w = 620, h = n * 26 + 16;
    const int x = kScreenWidth - w - 24, y = kScreenHeight - h - 24;
    ui::Panel(x, y, w, h, 140, false);
    for (int i = 0; i < n; ++i) fm.Draw(x + 12, y + 8 + i * 26, lines[i], ui::SubTextColor(), FontId::Small);
}

} // namespace ag
