#pragma once
// UI 描画の共通部品（パネル・ボタン・ゲージ）。Ars Gladii 独自デザイン：
// 紺色の半透明パネル＋金色の縁取り。
#include "../Core/FontManager.h"
#include <string>

namespace ag {
namespace ui {

unsigned int Gold();
unsigned int GoldDark();
unsigned int TextColor();
unsigned int SubTextColor();

bool MouseIn(int x, int y, int w, int h);
void Panel(int x, int y, int w, int h, int alpha = 200, bool goldFrame = true);
// 押されたら true（クリックを消費する）
bool Button(int x, int y, int w, int h, const std::string& label, bool selected = false, bool enabled = true, FontId font = FontId::Normal);
// 斜めカットの装飾付きゲージ
void Gauge(int x, int y, int w, int h, float rate, unsigned int color, unsigned int backColor, float trailingRate = -1.0f);
// 左右のスライダー（値は min～max、変更されたら true）
bool Slider(int x, int y, int w, const std::string& label, float& value, float minV, float maxV, const char* fmt);
bool Toggle(int x, int y, const std::string& label, bool& value);
void DimScreen(int alpha);

} // namespace ui
} // namespace ag
