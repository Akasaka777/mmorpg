#include "Character.h"
#include <algorithm>

namespace ag {

DamageResult Character::TakeDamage(const DamageResult& dmg, Character* /*attacker*/)
{
    if (IsDead()) return DamageResult{};
    hp = std::max(0, hp - dmg.amount);
    hitFlash = 1.0f;
    return dmg;
}

void Character::InitVisual(const std::string& modelKey, const PrimitiveLook& primitiveLook)
{
    look = primitiveLook;
    anim.Init(modelKey);
}

void Character::UpdateCommon(float dt)
{
    anim.Update(dt);
    hitFlash = std::max(0.0f, hitFlash - dt * 6.0f);
}

void Character::DrawBody(float opacity, float spin, unsigned int trailColor) const
{
    if (opacity <= 0.0f) return;
    PrimitiveRenderer::DrawShadow(pos, radius * 1.1f * (look.shape == BodyShape::Beast ? 1.3f : 1.0f));
    if (anim.HasModel()) {
        const float f = 1.0f + hitFlash * 1.5f;
        anim.DrawModel(pos, yaw + spin, 1.0f, GetColorF(f, f, f, 1.0f), opacity);
        return;
    }
    PrimitivePose pose;
    pose.state = anim.State();
    pose.time = anim.Time();
    pose.progress = anim.Progress();
    pose.flash = hitFlash * 0.8f;
    pose.opacity = opacity;
    pose.spin = spin;
    pose.trailColor = trailColor;
    PrimitiveRenderer::Draw(look, pose, pos, yaw);
}

} // namespace ag
