#include "FloatingText.h"
#include "../Core/FontManager.h"
#include "../Core/MathUtil.h"
#include "../Core/Random.h"
#include <algorithm>

namespace ag {

namespace {
constexpr float kLifetime = 1.1f;
}

void FloatingTextSystem::Spawn(const VECTOR& worldPos, const std::string& text, unsigned int color, bool big)
{
    items_.push_back({ worldPos, text, color, big, 0.0f, Random::Float(-25.0f, 25.0f) });
}

void FloatingTextSystem::Update(float dt)
{
    for (auto& i : items_) {
        i.age += dt;
        i.pos.y += 90.0f * dt;
    }
    items_.erase(std::remove_if(items_.begin(), items_.end(), [](const Item& i) { return i.age > kLifetime; }), items_.end());
}

void FloatingTextSystem::Draw() const
{
    const FontManager& fm = FontManager::Get();
    for (const auto& i : items_) {
        const VECTOR s = ConvWorldPosToScreenPos(i.pos);
        if (s.z <= 0.0f || s.z >= 1.0f) continue;
        const float fade = Clamp((kLifetime - i.age) / 0.3f, 0.0f, 1.0f);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * fade));
        fm.DrawCentered(static_cast<int>(s.x + i.offsetX), static_cast<int>(s.y), i.text, i.color,
                        i.big ? FontId::Large : FontId::Medium, GetColor(0, 0, 0));
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

} // namespace ag
