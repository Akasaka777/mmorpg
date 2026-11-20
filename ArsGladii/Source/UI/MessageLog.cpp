#include "MessageLog.h"
#include "UIDraw.h"
#include "../Core/Config.h"
#include "../Core/MathUtil.h"
#include "DxLib.h"

namespace ag {

void MessageLog::Add(const std::string& text, unsigned int color)
{
    lines_.push_back({ text, color, 0.0f });
    while (static_cast<int>(lines_.size()) > kMaxLines) lines_.pop_front();
}

void MessageLog::Update(float dt)
{
    for (auto& l : lines_) l.age += dt;
    while (!lines_.empty() && lines_.front().age > kLifetime) lines_.pop_front();
}

void MessageLog::Draw() const
{
    if (lines_.empty()) return;
    const FontManager& fm = FontManager::Get();
    const int lineH = 28;
    const int x = 30;
    const int bottom = kScreenHeight - 200;
    const int h = static_cast<int>(lines_.size()) * lineH + 16;
    ui::Panel(x - 10, bottom - h, 620, h + 8, 120, false);
    int y = bottom - h + 10;
    for (const auto& l : lines_) {
        const float fade = Clamp((kLifetime - l.age) / 2.0f, 0.0f, 1.0f);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * fade));
        fm.Draw(x, y, l.text, l.color, FontId::Small, GetColor(0, 0, 0));
        y += lineH;
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

} // namespace ag
