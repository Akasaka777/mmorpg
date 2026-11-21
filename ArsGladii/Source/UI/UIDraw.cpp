#include "UIDraw.h"
#include "../Core/Input.h"
#include "../Core/Config.h"
#include "../Core/MathUtil.h"
#include "DxLib.h"

namespace ag {
namespace ui {

unsigned int Gold() { return GetColor(222, 186, 96); }
unsigned int GoldDark() { return GetColor(120, 96, 48); }
unsigned int TextColor() { return GetColor(240, 236, 226); }
unsigned int SubTextColor() { return GetColor(170, 170, 180); }

bool MouseIn(int x, int y, int w, int h)
{
    const Input& in = Input::Get();
    if (in.IsMouseCaptured()) return false;
    return in.MouseX() >= x && in.MouseX() < x + w && in.MouseY() >= y && in.MouseY() < y + h;
}

void Panel(int x, int y, int w, int h, int alpha, bool goldFrame)
{
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
    DrawBox(x, y, x + w, y + h, GetColor(14, 20, 38), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha / 3);
    DrawBox(x, y, x + w, y + h / 3, GetColor(60, 80, 130), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    if (goldFrame) {
        DrawBox(x, y, x + w, y + h, GoldDark(), FALSE);
        DrawBox(x + 2, y + 2, x + w - 2, y + h - 2, GetColor(60, 50, 30), FALSE);
        // 四隅の装飾
        const int c = 10;
        const unsigned int g = Gold();
        DrawLine(x, y, x + c, y, g, 2); DrawLine(x, y, x, y + c, g, 2);
        DrawLine(x + w - c, y, x + w, y, g, 2); DrawLine(x + w - 1, y, x + w - 1, y + c, g, 2);
        DrawLine(x, y + h - 1, x + c, y + h - 1, g, 2); DrawLine(x, y + h - c, x, y + h, g, 2);
        DrawLine(x + w - c, y + h - 1, x + w, y + h - 1, g, 2); DrawLine(x + w - 1, y + h - c, x + w - 1, y + h, g, 2);
    }
}

bool Button(int x, int y, int w, int h, const std::string& label, bool selected, bool enabled, FontId font)
{
    Input& in = Input::Get();
    const bool hover = enabled && MouseIn(x, y, w, h);
    unsigned int bg = selected ? GetColor(70, 60, 30) : (hover ? GetColor(45, 60, 100) : GetColor(24, 32, 56));
    if (!enabled) bg = GetColor(30, 30, 36);
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 225);
    DrawBox(x, y, x + w, y + h, bg, TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawBox(x, y, x + w, y + h, (selected || hover) ? Gold() : GoldDark(), FALSE);
    const FontManager& fm = FontManager::Get();
    const unsigned int tc = enabled ? (selected ? Gold() : TextColor()) : GetColor(110, 110, 110);
    fm.DrawCentered(x + w / 2, y + (h - fm.Size(font)) / 2, label, tc, font);
    if (hover && in.MousePressed(MouseButton::Left)) {
        in.ConsumeMouse();
        return true;
    }
    return false;
}

void Gauge(int x, int y, int w, int h, float rate, unsigned int color, unsigned int backColor, float trailingRate)
{
    rate = Clamp(rate, 0.0f, 1.0f);
    DrawBox(x - 2, y - 2, x + w + 2, y + h + 2, GetColor(10, 10, 14), TRUE);
    DrawBox(x, y, x + w, y + h, backColor, TRUE);
    if (trailingRate > rate) DrawBox(x, y, x + static_cast<int>(w * Clamp(trailingRate, 0.0f, 1.0f)), y + h, GetColor(230, 220, 200), TRUE);
    DrawBox(x, y, x + static_cast<int>(w * rate), y + h, color, TRUE);
    // ハイライト
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 70);
    DrawBox(x, y, x + static_cast<int>(w * rate), y + h / 3, GetColor(255, 255, 255), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    // 目盛り
    for (int i = 1; i < 10; ++i) {
        const int gx = x + w * i / 10;
        DrawLine(gx, y + h - h / 3, gx, y + h, GetColor(0, 0, 0), 1);
    }
    DrawBox(x - 1, y - 1, x + w + 1, y + h + 1, GoldDark(), FALSE);
}

bool Slider(int x, int y, int w, const std::string& label, float& value, float minV, float maxV, const char* fmt)
{
    Input& in = Input::Get();
    const FontManager& fm = FontManager::Get();
    fm.Draw(x, y, label, TextColor());
    const int bx = x + 260, by = y + 10, bw = w - 380;
    DrawBox(bx, by, bx + bw, by + 6, GetColor(50, 50, 60), TRUE);
    const float t = (value - minV) / (maxV - minV);
    DrawBox(bx, by, bx + static_cast<int>(bw * t), by + 6, Gold(), TRUE);
    DrawCircle(bx + static_cast<int>(bw * t), by + 3, 10, Gold(), TRUE);
    fm.Draw(bx + bw + 24, y, Format(fmt, value), TextColor());
    if (in.MouseDown(MouseButton::Left) && MouseIn(bx - 10, by - 14, bw + 20, 34)) {
        const float nt = Clamp(static_cast<float>(in.MouseX() - bx) / bw, 0.0f, 1.0f);
        const float nv = minV + (maxV - minV) * nt;
        if (nv != value) {
            value = nv;
            in.ConsumeMouse();
            return true;
        }
    }
    return false;
}

bool Toggle(int x, int y, const std::string& label, bool& value)
{
    FontManager::Get().Draw(x, y, label, TextColor());
    if (Button(x + 260, y - 4, 140, 34, value ? "ON" : "OFF", value)) {
        value = !value;
        return true;
    }
    return false;
}

void DimScreen(int alpha)
{
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
    DrawBox(0, 0, kScreenWidth, kScreenHeight, GetColor(0, 0, 8), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

} // namespace ui
} // namespace ag
