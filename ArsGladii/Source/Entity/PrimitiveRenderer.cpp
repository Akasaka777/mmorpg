#include "PrimitiveRenderer.h"
#include "../Core/MathUtil.h"
#include <cmath>

namespace ag {

namespace {

float g_scale = 1.0f;     // 描画中キャラのスケール（半径に掛ける）
float g_flash = 0.0f;     // 被弾フラッシュ

float EaseOut(float t) { t = Clamp(t, 0.0f, 1.0f); return 1.0f - (1.0f - t) * (1.0f - t); }
float Seg(float p, float a, float b) { return Clamp((p - a) / (b - a), 0.0f, 1.0f); }

VECTOR ArmDir(float pitch, float side)
{
    return VGet(std::sin(pitch) * std::sin(side), -std::cos(pitch), std::sin(pitch) * std::cos(side));
}

unsigned int Col(unsigned int rgb) { return PrimitiveRenderer::ToDx(PrimitiveRenderer::Mix(rgb, 0xFFFFFF, g_flash)); }

void Capsule(const MATRIX& m, const VECTOR& a, const VECTOR& b, float r, unsigned int rgb)
{
    DrawCapsule3D(VTransform(a, m), VTransform(b, m), r * g_scale, 8, Col(rgb), GetColor(40, 40, 40), TRUE);
}

void Sphere(const MATRIX& m, const VECTOR& c, float r, unsigned int rgb)
{
    DrawSphere3D(VTransform(c, m), r * g_scale, 12, Col(rgb), GetColor(40, 40, 40), TRUE);
}

void Cone(const MATRIX& m, const VECTOR& top, const VECTOR& bottom, float r, unsigned int rgb)
{
    DrawCone3D(VTransform(top, m), VTransform(bottom, m), r * g_scale, 12, Col(rgb), GetColor(40, 40, 40), TRUE);
}

struct BlendScope {
    explicit BlendScope(int alpha)
    {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
        SetUseLighting(FALSE);
        SetWriteZBuffer3D(FALSE);
    }
    ~BlendScope()
    {
        SetWriteZBuffer3D(TRUE);
        SetUseLighting(TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
};

} // namespace

unsigned int PrimitiveRenderer::ToDx(unsigned int rgb)
{
    return GetColor((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
}

unsigned int PrimitiveRenderer::Mix(unsigned int a, unsigned int b, float t)
{
    t = Clamp(t, 0.0f, 1.0f);
    auto ch = [&](int shift) {
        const float ca = static_cast<float>((a >> shift) & 0xFF);
        const float cb = static_cast<float>((b >> shift) & 0xFF);
        return static_cast<unsigned int>(ca + (cb - ca) * t) & 0xFF;
    };
    return (ch(16) << 16) | (ch(8) << 8) | ch(0);
}

void PrimitiveRenderer::Draw(const PrimitiveLook& look, const PrimitivePose& pose, const VECTOR& pos, float yaw)
{
    g_scale = look.scale;
    g_flash = pose.flash;

    MATRIX fall = MGetIdent();
    if (pose.state == AnimState::Death) {
        const float k = EaseOut(pose.progress * 1.4f);
        fall = (look.shape == BodyShape::Beast) ? MGetRotZ(k * kPi * 0.5f) : MGetRotX(-k * kPi * 0.5f);
    }
    MATRIX world = MMult(MGetScale(VGet(look.scale, look.scale, look.scale)), fall);
    world = MMult(world, MGetRotY(yaw + pose.spin));
    world = MMult(world, MGetTranslate(pos));

    const bool translucent = pose.opacity < 0.999f;
    if (translucent) SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(pose.opacity * 255));
    if (look.shape == BodyShape::Beast) DrawBeast(look, pose, world);
    else DrawHumanoid(look, pose, world);
    if (translucent) SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void PrimitiveRenderer::DrawHumanoid(const PrimitiveLook& look, const PrimitivePose& pose, const MATRIX& m)
{
    const float t = pose.time;
    const float p = pose.progress;
    float bob = 0.0f, lean = 0.0f, legSwing = 0.0f;
    float rPitch = 0.25f, rSide = 0.15f, lPitch = 0.25f, lSide = -0.15f;
    float rArmLen = 55.0f;
    bool thrustWeapon = look.thrustStyle;
    VECTOR weaponDir = VGet(0, 0, 0);
    bool useWeaponDir = false;

    switch (pose.state) {
    case AnimState::Idle:
        bob = std::sin(t * 2.0f) * 2.0f;
        rPitch = 0.30f + std::sin(t * 2.0f) * 0.03f;
        break;
    case AnimState::Move: {
        const float ph = p * kTwoPi;
        legSwing = std::sin(ph) * 0.7f;
        rPitch = 0.35f - std::sin(ph) * 0.5f;
        lPitch = 0.35f + std::sin(ph) * 0.5f;
        bob = std::fabs(std::sin(ph)) * 5.0f;
        lean = 8.0f;
        break;
    }
    case AnimState::Attack:
        if (thrustWeapon) {
            const float back = Seg(p, 0.0f, 0.35f), fwd = EaseOut(Seg(p, 0.35f, 0.6f)), ret = Seg(p, 0.75f, 1.0f);
            rPitch = 1.45f;
            rSide = 0.1f;
            rArmLen = 55.0f - back * 20.0f + fwd * 45.0f - ret * 25.0f;
            lean = fwd * 14.0f;
            weaponDir = VGet(0.0f, 0.02f, 1.0f);
            useWeaponDir = true;
        } else if (p < 0.3f) {
            const float k = Seg(p, 0.0f, 0.3f);
            rPitch = Lerp(0.3f, 2.9f, k);
            rSide = Lerp(0.15f, -0.3f, k);
        } else {
            const float k = EaseOut(Seg(p, 0.3f, 0.7f));
            rPitch = Lerp(2.9f, 0.8f, k);
            rSide = Lerp(-0.3f, 0.7f, k);
            lean = 12.0f * k;
        }
        lPitch = 0.5f;
        break;
    case AnimState::Skill:
        if (thrustWeapon) {
            const float pulse = std::fabs(std::sin(p * kPi * 3.0f));
            rPitch = 1.5f;
            rSide = 0.05f;
            rArmLen = 45.0f + pulse * 55.0f;
            lean = 10.0f + pulse * 8.0f;
            weaponDir = VGet(0.0f, 0.02f, 1.0f);
            useWeaponDir = true;
        } else {
            const float wind = Seg(p, 0.0f, 0.2f), sweep = EaseOut(Seg(p, 0.2f, 0.75f));
            rPitch = 1.55f;
            rSide = Lerp(Lerp(0.15f, -1.9f, wind), 1.9f, sweep);
            lean = 10.0f;
            weaponDir = ArmDir(1.6f, rSide + 0.5f);
            useWeaponDir = true;
        }
        lPitch = 0.9f;
        lSide = -0.6f;
        break;
    case AnimState::Hit:
        lean = -14.0f * (1.0f - p);
        rPitch = 0.9f;
        lPitch = 0.9f;
        bob = -3.0f;
        break;
    case AnimState::Death:
        rPitch = 0.6f + p;
        lPitch = 0.6f + p;
        rSide = 0.8f;
        lSide = -0.8f;
        break;
    default:
        break;
    }

    const unsigned int main = look.mainColor, sub = look.subColor, skin = look.skinColor;

    // 脚
    for (int side = -1; side <= 1; side += 2) {
        const float sw = legSwing * side;
        const VECTOR hip = VGet(11.0f * side, 90.0f + bob, 0.0f);
        const VECTOR foot = VGet(11.0f * side, 90.0f + bob - 86.0f * std::cos(sw), 86.0f * std::sin(sw));
        Capsule(m, hip, foot, 9.0f, sub);
    }
    // 胴体
    const VECTOR torsoLow = VGet(0.0f, 96.0f + bob, 0.0f);
    const VECTOR torsoHigh = VGet(0.0f, 138.0f + bob, lean);
    Capsule(m, torsoLow, torsoHigh, 22.0f, main);
    // 頭
    const VECTOR head = VGet(0.0f, 163.0f + bob, lean * 1.2f);
    Sphere(m, head, 16.0f, skin);
    Sphere(m, VAdd(head, VGet(-6.0f, 2.0f, 14.0f)), 2.6f, look.eyeColor);
    Sphere(m, VAdd(head, VGet(6.0f, 2.0f, 14.0f)), 2.6f, look.eyeColor);
    if (look.horns) {
        Cone(m, VAdd(head, VGet(-22.0f, 26.0f, -2.0f)), VAdd(head, VGet(-10.0f, 8.0f, 0.0f)), 5.0f, 0xE8E0C8);
        Cone(m, VAdd(head, VGet(22.0f, 26.0f, -2.0f)), VAdd(head, VGet(10.0f, 8.0f, 0.0f)), 5.0f, 0xE8E0C8);
    }

    // 腕
    const VECTOR rShoulder = VGet(27.0f, 134.0f + bob, lean * 0.9f);
    const VECTOR lShoulder = VGet(-27.0f, 134.0f + bob, lean * 0.9f);
    const VECTOR rDir = ArmDir(rPitch, rSide);
    const VECTOR lDir = ArmDir(lPitch, lSide);
    const VECTOR rHand = VAdd(rShoulder, VScale(rDir, rArmLen));
    const VECTOR lHand = VAdd(lShoulder, VScale(lDir, 52.0f));
    Capsule(m, rShoulder, rHand, 7.0f, main);
    Capsule(m, lShoulder, lHand, 7.0f, main);
    Sphere(m, rHand, 7.5f, skin);
    Sphere(m, lHand, 7.5f, skin);

    if (look.shield) {
        Cone(m, VAdd(lHand, VGet(-4.0f, 0.0f, 12.0f)), VAdd(lHand, VGet(-2.0f, 0.0f, 3.0f)), 26.0f, sub);
    }
    if (look.weapon != WeaponType::None) {
        VECTOR wdir = useWeaponDir ? VNorm(weaponDir) : ArmDir(rPitch + 1.1f, rSide);
        DrawWeapon(look.weapon, rHand, wdir, m, 0xC8CCD8, pose.flash);

        // スキル軌跡（剣先の残光）
        if (pose.trailColor != 0 && (pose.state == AnimState::Skill || pose.state == AnimState::Attack)) {
            SetDrawBlendMode(DX_BLENDMODE_ADD, 180);
            const VECTOR tip = VAdd(rHand, VScale(wdir, 90.0f));
            DrawSphere3D(VTransform(tip, m), 14.0f * g_scale, 8, ToDx(pose.trailColor), GetColor(0, 0, 0), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }
    }
}

void PrimitiveRenderer::DrawBeast(const PrimitiveLook& look, const PrimitivePose& pose, const MATRIX& m)
{
    const float t = pose.time;
    const float p = pose.progress;
    float legSwing = 0.0f, dz = 0.0f, headDy = 0.0f, bob = 0.0f, tailWag = std::sin(t * 6.0f) * 10.0f;

    switch (pose.state) {
    case AnimState::Idle:
        bob = std::sin(t * 2.5f) * 1.5f;
        break;
    case AnimState::Move:
        legSwing = std::sin(p * kTwoPi) * 0.6f;
        bob = std::fabs(std::sin(p * kTwoPi)) * 4.0f;
        tailWag = std::sin(t * 14.0f) * 14.0f;
        break;
    case AnimState::Attack:
    case AnimState::Skill: {
        const float back = Seg(p, 0.0f, 0.4f), fwd = EaseOut(Seg(p, 0.4f, 0.65f)), ret = Seg(p, 0.7f, 1.0f);
        dz = -back * 18.0f + fwd * 55.0f - ret * 37.0f;
        headDy = -fwd * 10.0f + ret * 10.0f;
        break;
    }
    case AnimState::Hit:
        dz = -12.0f * (1.0f - p);
        bob = std::sin(t * 60.0f) * 2.0f;
        break;
    default:
        break;
    }

    const unsigned int main = look.mainColor, sub = look.subColor;
    Capsule(m, VGet(0, 55 + bob, -38 + dz), VGet(0, 62 + bob, 36 + dz), 26.0f, main);
    const VECTOR head = VGet(0, 74 + bob + headDy, 72 + dz);
    Sphere(m, head, 20.0f, main);
    Cone(m, VAdd(head, VGet(0, -6, 36)), VAdd(head, VGet(0, -4, 12)), 11.0f, sub);
    Cone(m, VAdd(head, VGet(-10, 26, -6)), VAdd(head, VGet(-9, 12, -2)), 6.0f, sub);
    Cone(m, VAdd(head, VGet(10, 26, -6)), VAdd(head, VGet(9, 12, -2)), 6.0f, sub);
    Sphere(m, VAdd(head, VGet(-8, 5, 16)), 3.0f, look.eyeColor);
    Sphere(m, VAdd(head, VGet(8, 5, 16)), 3.0f, look.eyeColor);
    if (look.horns) {
        Cone(m, VAdd(head, VGet(-12, 18, 14)), VAdd(head, VGet(-8, 8, 6)), 5.0f, 0xEEE6D0);
        Cone(m, VAdd(head, VGet(12, 18, 14)), VAdd(head, VGet(8, 8, 6)), 5.0f, 0xEEE6D0);
    }

    const float legLen = 48.0f;
    const float hipZ[2] = { 28.0f, -32.0f };
    for (int fb = 0; fb < 2; ++fb) {
        for (int side = -1; side <= 1; side += 2) {
            const float sw = legSwing * side * (fb == 0 ? 1.0f : -1.0f);
            const VECTOR hip = VGet(15.0f * side, 50.0f + bob, hipZ[fb] + dz);
            const VECTOR foot = VAdd(hip, VGet(0, -legLen * std::cos(sw), legLen * std::sin(sw)));
            Capsule(m, hip, foot, 7.0f, sub);
        }
    }
    Capsule(m, VGet(0, 64 + bob, -58 + dz), VGet(tailWag, 78 + bob, -86 + dz), 5.0f, sub);
}

void PrimitiveRenderer::DrawWeapon(WeaponType w, const VECTOR& hand, const VECTOR& dir, const MATRIX& m, unsigned int tint, float /*flash*/)
{
    const unsigned int grip = 0x5A3A22;
    switch (w) {
    case WeaponType::OneHandSword:
        Capsule(m, VAdd(hand, VScale(dir, -10.0f)), VAdd(hand, VScale(dir, 8.0f)), 3.0f, grip);
        Capsule(m, VAdd(hand, VScale(dir, 10.0f)), VAdd(hand, VScale(dir, 95.0f)), 4.0f, tint);
        break;
    case WeaponType::OneHandMace:
        Capsule(m, VAdd(hand, VScale(dir, -8.0f)), VAdd(hand, VScale(dir, 55.0f)), 3.5f, grip);
        Sphere(m, VAdd(hand, VScale(dir, 62.0f)), 13.0f, 0x8A8A96);
        break;
    case WeaponType::Dagger:
        Capsule(m, VAdd(hand, VScale(dir, -6.0f)), VAdd(hand, VScale(dir, 6.0f)), 3.0f, grip);
        Capsule(m, VAdd(hand, VScale(dir, 8.0f)), VAdd(hand, VScale(dir, 45.0f)), 3.5f, tint);
        break;
    case WeaponType::Rapier:
        Sphere(m, VAdd(hand, VScale(dir, 6.0f)), 6.0f, 0xD4AF37);
        Capsule(m, VAdd(hand, VScale(dir, 8.0f)), VAdd(hand, VScale(dir, 110.0f)), 1.8f, tint);
        break;
    case WeaponType::Spear:
        Capsule(m, VAdd(hand, VScale(dir, -70.0f)), VAdd(hand, VScale(dir, 140.0f)), 3.2f, grip);
        Cone(m, VAdd(hand, VScale(dir, 172.0f)), VAdd(hand, VScale(dir, 140.0f)), 7.0f, tint);
        break;
    default:
        break;
    }
}

void PrimitiveRenderer::DrawShadow(const VECTOR& pos, float radius)
{
    DrawGroundCircle(pos, radius, 0x000000, 90, 20);
}

void PrimitiveRenderer::DrawGroundCircle(const VECTOR& center, float radius, unsigned int color, int alpha, int segments)
{
    BlendScope scope(alpha);
    const unsigned int c = ToDx(color);
    const VECTOR cen = VGet(center.x, 2.0f, center.z);
    for (int i = 0; i < segments; ++i) {
        const float a0 = kTwoPi * i / segments, a1 = kTwoPi * (i + 1) / segments;
        const VECTOR p0 = VGet(center.x + std::sin(a0) * radius, 2.0f, center.z + std::cos(a0) * radius);
        const VECTOR p1 = VGet(center.x + std::sin(a1) * radius, 2.0f, center.z + std::cos(a1) * radius);
        DrawTriangle3D(cen, p0, p1, c, TRUE);
    }
}

void PrimitiveRenderer::DrawGroundRing(const VECTOR& center, float radius, float width, unsigned int color, int alpha, int segments)
{
    BlendScope scope(alpha);
    const unsigned int c = ToDx(color);
    const float r0 = radius - width;
    for (int i = 0; i < segments; ++i) {
        const float a0 = kTwoPi * i / segments, a1 = kTwoPi * (i + 1) / segments;
        const VECTOR o0 = VGet(center.x + std::sin(a0) * radius, 2.5f, center.z + std::cos(a0) * radius);
        const VECTOR o1 = VGet(center.x + std::sin(a1) * radius, 2.5f, center.z + std::cos(a1) * radius);
        const VECTOR i0 = VGet(center.x + std::sin(a0) * r0, 2.5f, center.z + std::cos(a0) * r0);
        const VECTOR i1 = VGet(center.x + std::sin(a1) * r0, 2.5f, center.z + std::cos(a1) * r0);
        DrawTriangle3D(i0, o0, o1, c, TRUE);
        DrawTriangle3D(i0, o1, i1, c, TRUE);
    }
}

void PrimitiveRenderer::DrawGroundSector(const VECTOR& center, float yaw, float radius, float halfArc, unsigned int color, int alpha, int segments)
{
    BlendScope scope(alpha);
    const unsigned int c = ToDx(color);
    const VECTOR cen = VGet(center.x, 2.0f, center.z);
    for (int i = 0; i < segments; ++i) {
        const float a0 = yaw - halfArc + 2.0f * halfArc * i / segments;
        const float a1 = yaw - halfArc + 2.0f * halfArc * (i + 1) / segments;
        const VECTOR p0 = VGet(center.x + std::sin(a0) * radius, 2.0f, center.z + std::cos(a0) * radius);
        const VECTOR p1 = VGet(center.x + std::sin(a1) * radius, 2.0f, center.z + std::cos(a1) * radius);
        DrawTriangle3D(cen, p0, p1, c, TRUE);
    }
}

} // namespace ag
