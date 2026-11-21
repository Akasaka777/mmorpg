#include "TitleScene.h"
#include "../UI/UIDraw.h"
#include "../Core/Config.h"
#include "../Core/Input.h"
#include "../Core/MathUtil.h"
#include "../Core/AssetManager.h"
#include "DxLib.h"
#include <cmath>

namespace ag {

void TitleScene::Enter()
{
    time_ = 0.0f;
    save_ = SaveSystem::Peek(kSavePath);
    cursor_ = save_.exists ? 1 : 0;
    Input::Get().SetMouseCaptured(false);
    SoundManager::Get().PlayBGM("bgm_title");
}

void TitleScene::Update(float dt)
{
    time_ += dt;
    Input& in = Input::Get();
    if (in.KeyPressed(KEY_INPUT_UP) || in.KeyPressed(KEY_INPUT_W)) cursor_ = (cursor_ + 2) % 3;
    if (in.KeyPressed(KEY_INPUT_DOWN) || in.KeyPressed(KEY_INPUT_S)) cursor_ = (cursor_ + 1) % 3;
    if (!save_.exists && cursor_ == 1) cursor_ = in.KeyPressed(KEY_INPUT_UP) || in.KeyPressed(KEY_INPUT_W) ? 0 : 2;
    if (in.ActionPressed(Action::Confirm) || in.KeyPressed(KEY_INPUT_SPACE)) {
        if (cursor_ == 0) RequestScene(SceneId::Field, false);
        else if (cursor_ == 1 && save_.exists) RequestScene(SceneId::Field, true);
        else if (cursor_ == 2) RequestScene(SceneId::Quit);
    }
}

void TitleScene::Draw()
{
    const FontManager& fm = FontManager::Get();
    // 背景グラデーション
    const int bg = AssetManager::Get().GetImage("Assets/UI/title_bg.png");
    if (bg != -1) {
        DrawExtendGraph(0, 0, kScreenWidth, kScreenHeight, bg, FALSE);
    } else {
        for (int y = 0; y < kScreenHeight; y += 4) {
            const float t = static_cast<float>(y) / kScreenHeight;
            DrawBox(0, y, kScreenWidth, y + 4, GetColor(static_cast<int>(10 + 20 * t), static_cast<int>(14 + 30 * t), static_cast<int>(40 + 50 * t)), TRUE);
        }
        // 浮遊する光の粒
        SetDrawBlendMode(DX_BLENDMODE_ADD, 120);
        for (int i = 0; i < 60; ++i) {
            const float px = std::fmod(i * 197.0f + time_ * (10 + i % 7) * 3.0f, static_cast<float>(kScreenWidth));
            const float py = kScreenHeight - std::fmod(i * 131.0f + time_ * (20 + i % 5) * 4.0f, static_cast<float>(kScreenHeight));
            DrawCircle(static_cast<int>(px), static_cast<int>(py), 2 + i % 3, GetColor(200, 170, 90), TRUE);
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        // 中央の紋章（剣と円環）
        const int cx = kScreenWidth / 2, cy = 330;
        DrawCircle(cx, cy, 170, ui::GoldDark(), FALSE, 3);
        DrawCircle(cx, cy, 150, GetColor(70, 60, 30), FALSE, 1);
        DrawLine(cx - 120, cy + 120, cx + 120, cy - 120, GetColor(170, 175, 190), 8);
        DrawLine(cx + 120, cy + 120, cx - 120, cy - 120, GetColor(170, 175, 190), 8);
    }

    const float a = Clamp(time_ / 1.2f, 0.0f, 1.0f);
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * a));
    fm.DrawCentered(kScreenWidth / 2, 270, "Ars Gladii", ui::Gold(), FontId::Title, GetColor(30, 20, 0));
    fm.DrawCentered(kScreenWidth / 2, 390, "― アルス・グラディ ―", ui::TextColor(), FontId::Medium, GetColor(0, 0, 0));
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    const int bw = 460, bh = 64, bx = (kScreenWidth - bw) / 2;
    int by = 560;
    const std::string cont = save_.exists ? Format("つづきから（Lv%d / 第%d層）", save_.level, save_.floor) : "つづきから（データなし）";
    const char* labels[3] = { "はじめから", cont.c_str(), "ゲーム終了" };
    for (int i = 0; i < 3; ++i) {
        const bool enabled = (i != 1 || save_.exists);
        if (ui::MouseIn(bx, by, bw, bh) && enabled) cursor_ = i;
        if (ui::Button(bx, by, bw, bh, labels[i], cursor_ == i, enabled, FontId::Medium)) {
            if (i == 0) RequestScene(SceneId::Field, false);
            else if (i == 1) RequestScene(SceneId::Field, true);
            else RequestScene(SceneId::Quit);
        }
        by += bh + 20;
    }
    if (save_.exists && !save_.savedAt.empty())
        fm.DrawCentered(kScreenWidth / 2, by + 6, Format("最終セーブ: %s", save_.savedAt.c_str()), ui::SubTextColor(), FontId::Small);

    fm.DrawRight(kScreenWidth - 30, kScreenHeight - 44, Format("Ver. %s", kVersion), ui::SubTextColor(), FontId::Small);
    fm.Draw(30, kScreenHeight - 44, "W/S・↑↓: 選択   Enter: 決定   マウス操作可", ui::SubTextColor(), FontId::Small);
}

} // namespace ag
