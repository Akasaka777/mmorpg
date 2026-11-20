#include "Field.h"
#include "../Core/MathUtil.h"
#include "../Core/AssetManager.h"
#include "../Core/FontManager.h"
#include "../Entity/PrimitiveRenderer.h"
#include <cmath>
#include <random>

namespace ag {

namespace {
constexpr int kGrid = 64;

COLOR_U8 ToU8(unsigned int rgb, float shade)
{
    auto c = [&](int shift) { return static_cast<int>(Clamp(((rgb >> shift) & 0xFF) * shade, 0.0f, 255.0f)); };
    return GetColorU8(c(16), c(8), c(0), 255);
}

// 簡易ノイズ（格子ハッシュ）
float Hash(int x, int z, unsigned int seed)
{
    unsigned int h = static_cast<unsigned int>(x) * 374761393u + static_cast<unsigned int>(z) * 668265263u + seed * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return static_cast<float>(h & 0xFFFF) / 65535.0f;
}
}

Field::~Field() { Release(); }

void Field::Release()
{
    if (fieldModel_ != -1) MV1DeleteModel(fieldModel_);
    fieldModel_ = -1;
    vertices_.clear();
    indices_.clear();
    props_.clear();
    obstacles_.clear();
}

float Field::PathX(float z) const
{
    return std::sin(z * 0.0006f) * 500.0f;
}

void Field::Build(const FloorDef& def)
{
    Release();
    def_ = &def;
    groundTexture_ = AssetManager::Get().GetImage("Assets/Textures/ground.png");
    fieldModel_ = AssetManager::Get().CreateModelInstance(Format("field_floor%d", def.number));

    // ---- 地面メッシュ ----
    const float size = def.halfSize * 2.0f;
    const float cell = size / kGrid;
    vertices_.resize((kGrid + 1) * (kGrid + 1));
    for (int z = 0; z <= kGrid; ++z) {
        for (int x = 0; x <= kGrid; ++x) {
            VERTEX3D& v = vertices_[z * (kGrid + 1) + x];
            const float wx = -def.halfSize + x * cell;
            const float wz = -def.halfSize + z * cell;
            v.pos = VGet(wx, 0.0f, wz);
            v.norm = VGet(0.0f, 1.0f, 0.0f);
            const float n = Hash(x, z, def.seed);
            unsigned int base = (n > 0.5f) ? def.groundColorA : def.groundColorB;
            float shade = 0.9f + n * 0.2f;
            const VECTOR p = v.pos;
            if (std::fabs(wx - PathX(wz)) < 220.0f && wz > def.townCenter.z && wz < def.bossPos.z) base = def.pathColor;
            if (DistXZ(p, def.townCenter) < def.townRadius) { base = 0xB8B0A0; shade = 0.95f + n * 0.1f; }
            if (DistXZ(p, def.bossPos) < def.bossArenaRadius) { base = 0x6E6A62; shade = 0.85f + n * 0.15f; }
            if (groundTexture_ != -1) base = 0xFFFFFF;
            v.dif = ToU8(base, shade);
            v.spc = GetColorU8(0, 0, 0, 0);
            v.u = x * 2.0f;
            v.v = z * 2.0f;
            v.su = v.sv = 0.0f;
        }
    }
    indices_.clear();
    for (int z = 0; z < kGrid; ++z) {
        for (int x = 0; x < kGrid; ++x) {
            const unsigned short i0 = static_cast<unsigned short>(z * (kGrid + 1) + x);
            const unsigned short i1 = static_cast<unsigned short>(i0 + 1);
            const unsigned short i2 = static_cast<unsigned short>(i0 + (kGrid + 1));
            const unsigned short i3 = static_cast<unsigned short>(i2 + 1);
            indices_.insert(indices_.end(), { i0, i2, i1, i1, i2, i3 });
        }
    }

    // ---- 町 ----
    const VECTOR tc = def.townCenter;
    props_.push_back({ Prop::Kind::Fountain, tc, 120.0f, 0.0f, 0x9AB0C8 });
    obstacles_.push_back({ tc, 150.0f });
    props_.push_back({ Prop::Kind::Guild, VAdd(tc, VGet(-650, 0, 150)), 1.0f, 0.0f, 0x8A5A3A });
    obstacles_.push_back({ VAdd(tc, VGet(-650, 0, 150)), 300.0f });
    const VECTOR houses[] = { VGet(600, 0, 300), VGet(700, 0, -400), VGet(-500, 0, -700), VGet(250, 0, -850), VGet(-900, 0, -250) };
    const unsigned int roofColors[] = { 0xA04030, 0x305080, 0x806030, 0x407040, 0x704070 };
    for (int i = 0; i < 5; ++i) {
        const VECTOR hp = VAdd(tc, houses[i]);
        props_.push_back({ Prop::Kind::House, hp, 1.0f, 0.0f, roofColors[i] });
        obstacles_.push_back({ hp, 190.0f });
    }
    // 町の柵（外周の杭）
    for (int i = 0; i < 48; ++i) {
        const float a = kTwoPi * i / 48.0f;
        if (std::fabs(WrapAngle(a)) < 0.25f) continue; // 北側（フィールド方向）は門として開ける
        props_.push_back({ Prop::Kind::Post, VAdd(tc, VGet(std::sin(a) * def.townRadius, 0, std::cos(a) * def.townRadius)), 1.0f, 0.0f, 0x6A4A2A });
    }

    // ---- ボス部屋（石柱の円環）----
    for (int i = 0; i < 16; ++i) {
        const float a = kTwoPi * i / 16.0f;
        if (std::fabs(WrapAngle(a - kPi)) < 0.25f) continue; // 南側が入口
        const VECTOR pp = VAdd(def.bossPos, VGet(std::sin(a) * def.bossArenaRadius, 0, std::cos(a) * def.bossArenaRadius));
        props_.push_back({ Prop::Kind::Pillar, pp, 1.0f, 0.0f, 0x7C7870 });
        obstacles_.push_back({ pp, 70.0f });
    }

    // ---- 木と岩（町・ボス部屋・道を避けて配置）----
    std::mt19937 rng(def.seed);
    std::uniform_real_distribution<float> coord(-def.halfSize + 300.0f, def.halfSize - 300.0f);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    auto freeSpot = [&](const VECTOR& p) {
        if (DistXZ(p, def.townCenter) < def.townRadius + 300.0f) return false;
        if (DistXZ(p, def.bossPos) < def.bossArenaRadius + 300.0f) return false;
        if (std::fabs(p.x - PathX(p.z)) < 450.0f) return false;
        for (const auto& z : def.zones) if (DistXZ(p, z.center) < 350.0f) return false;
        return true;
    };
    for (int i = 0, tries = 0; i < def.treeCount && tries < def.treeCount * 20; ++tries) {
        const VECTOR p = VGet(coord(rng), 0.0f, coord(rng));
        if (!freeSpot(p)) continue;
        const float s = 0.8f + unit(rng) * 0.7f;
        const unsigned int c = unit(rng) > 0.5f ? 0x2F6B2A : 0x3C7F33;
        props_.push_back({ Prop::Kind::Tree, p, s, 0.0f, c });
        obstacles_.push_back({ p, 40.0f * s });
        ++i;
    }
    for (int i = 0, tries = 0; i < def.rockCount && tries < def.rockCount * 20; ++tries) {
        const VECTOR p = VGet(coord(rng), 0.0f, coord(rng));
        if (!freeSpot(p)) continue;
        const float s = 0.6f + unit(rng) * 1.2f;
        props_.push_back({ Prop::Kind::Rock, p, s, unit(rng) * kTwoPi, 0x8A8A86 });
        obstacles_.push_back({ p, 60.0f * s });
        ++i;
    }
}

void Field::Draw(const VECTOR& cameraPos) const
{
    if (fieldModel_ != -1) {
        MV1DrawModel(fieldModel_);
    } else {
        SetUseLighting(FALSE);
        DrawPolygonIndexed3D(vertices_.data(), static_cast<int>(vertices_.size()), indices_.data(),
                             static_cast<int>(indices_.size() / 3), groundTexture_ != -1 ? groundTexture_ : DX_NONE_GRAPH, FALSE);
        SetUseLighting(TRUE);
    }

    // 町とボス部屋の境界表示
    PrimitiveRenderer::DrawGroundRing(def_->townCenter, def_->townRadius, 30.0f, 0x88CCFF, 90, 72);
    PrimitiveRenderer::DrawGroundRing(def_->bossPos, def_->bossArenaRadius - 80.0f, 40.0f, 0xAA2222, 110, 72);

    const float cullSq = 11000.0f * 11000.0f;
    for (const auto& p : props_) {
        if (DistXZSq(p.pos, cameraPos) > cullSq) continue;
        DrawProp(p);
    }
}

void Field::DrawProp(const Prop& p) const
{
    const unsigned int spc = GetColor(20, 20, 20);
    switch (p.kind) {
    case Prop::Kind::Tree: {
        const float s = p.size;
        DrawCapsule3D(VAdd(p.pos, VGet(0, 20 * s, 0)), VAdd(p.pos, VGet(0, 160 * s, 0)), 18 * s, 6, GetColor(110, 75, 45), spc, TRUE);
        const unsigned int c = PrimitiveRenderer::ToDx(p.color);
        DrawCone3D(VAdd(p.pos, VGet(0, 420 * s, 0)), VAdd(p.pos, VGet(0, 200 * s, 0)), 120 * s, 10, c, spc, TRUE);
        DrawCone3D(VAdd(p.pos, VGet(0, 330 * s, 0)), VAdd(p.pos, VGet(0, 130 * s, 0)), 150 * s, 10, c, spc, TRUE);
        break;
    }
    case Prop::Kind::Rock:
        DrawSphere3D(VAdd(p.pos, VGet(0, 20 * p.size, 0)), 60 * p.size, 7, PrimitiveRenderer::ToDx(p.color), spc, TRUE);
        DrawSphere3D(VAdd(p.pos, VGet(35 * p.size, 10 * p.size, 25 * p.size)), 35 * p.size, 6, GetColor(120, 120, 115), spc, TRUE);
        break;
    case Prop::Kind::House: {
        DrawCube3D(VAdd(p.pos, VGet(-150, 0, -150)), VAdd(p.pos, VGet(150, 230, 150)), GetColor(225, 210, 180), spc, TRUE);
        DrawCone3D(VAdd(p.pos, VGet(0, 380, 0)), VAdd(p.pos, VGet(0, 230, 0)), 230, 4, PrimitiveRenderer::ToDx(p.color), spc, TRUE);
        DrawCube3D(VAdd(p.pos, VGet(-35, 0, 148)), VAdd(p.pos, VGet(35, 120, 156)), GetColor(90, 60, 40), spc, TRUE);
        break;
    }
    case Prop::Kind::Guild: {
        DrawCube3D(VAdd(p.pos, VGet(-260, 0, -220)), VAdd(p.pos, VGet(260, 360, 220)), GetColor(200, 185, 160), spc, TRUE);
        DrawCone3D(VAdd(p.pos, VGet(0, 580, 0)), VAdd(p.pos, VGet(0, 360, 0)), 380, 4, PrimitiveRenderer::ToDx(p.color), spc, TRUE);
        DrawCube3D(VAdd(p.pos, VGet(250, 0, -60)), VAdd(p.pos, VGet(262, 180, 60)), GetColor(80, 55, 35), spc, TRUE);
        // 看板の旗
        DrawCapsule3D(VAdd(p.pos, VGet(300, 0, 200)), VAdd(p.pos, VGet(300, 450, 200)), 6, 4, GetColor(90, 70, 50), spc, TRUE);
        DrawCube3D(VAdd(p.pos, VGet(302, 330, 200)), VAdd(p.pos, VGet(306, 440, 320)), GetColor(180, 40, 40), spc, TRUE);
        break;
    }
    case Prop::Kind::Pillar:
        DrawCapsule3D(VAdd(p.pos, VGet(0, 0, 0)), VAdd(p.pos, VGet(0, 420, 0)), 45, 8, PrimitiveRenderer::ToDx(p.color), spc, TRUE);
        DrawCone3D(VAdd(p.pos, VGet(0, 520, 0)), VAdd(p.pos, VGet(0, 440, 0)), 55, 8, GetColor(150, 40, 40), spc, TRUE);
        break;
    case Prop::Kind::Fountain:
        DrawCone3D(VAdd(p.pos, VGet(0, 60, 0)), p.pos, 150, 16, GetColor(170, 170, 165), spc, TRUE);
        DrawCapsule3D(VAdd(p.pos, VGet(0, 40, 0)), VAdd(p.pos, VGet(0, 200, 0)), 22, 8, GetColor(180, 180, 175), spc, TRUE);
        DrawSphere3D(VAdd(p.pos, VGet(0, 215, 0)), 30, 10, PrimitiveRenderer::ToDx(p.color), spc, TRUE);
        break;
    case Prop::Kind::Post:
        DrawCapsule3D(p.pos, VAdd(p.pos, VGet(0, 110, 0)), 10, 4, PrimitiveRenderer::ToDx(p.color), spc, TRUE);
        break;
    }
}

void Field::DrawLabels() const
{
    struct Label { VECTOR pos; const char* text; unsigned int color; };
    const Label labels[] = {
        { VAdd(def_->townCenter, VGet(-650, 650, 150)), "冒険者ギルド（準備中）", GetColor(255, 230, 160) },
        { VAdd(def_->townCenter, VGet(0, 320, 0)), def_->townName.c_str(), GetColor(200, 230, 255) },
        { VAdd(def_->bossPos, VGet(0, 650, -def_->bossArenaRadius)), "ボスエリア", GetColor(255, 120, 100) },
    };
    for (const auto& l : labels) {
        const VECTOR s = ConvWorldPosToScreenPos(l.pos);
        if (s.z <= 0.0f || s.z >= 1.0f) continue;
        FontManager::Get().DrawCentered(static_cast<int>(s.x), static_cast<int>(s.y), l.text, l.color, FontId::Normal, GetColor(0, 0, 0));
    }
}

void Field::ResolveCollision(VECTOR& pos, float radius) const
{
    for (const auto& o : obstacles_) {
        const float minDist = o.radius + radius;
        const float dx = pos.x - o.pos.x, dz = pos.z - o.pos.z;
        const float d2 = dx * dx + dz * dz;
        if (d2 >= minDist * minDist || d2 < 0.0001f) continue;
        const float d = std::sqrt(d2);
        pos.x = o.pos.x + dx / d * minDist;
        pos.z = o.pos.z + dz / d * minDist;
    }
    const float lim = def_->halfSize - radius;
    pos.x = Clamp(pos.x, -lim, lim);
    pos.z = Clamp(pos.z, -lim, lim);
    pos.y = 0.0f;
}

bool Field::InTown(const VECTOR& p) const { return DistXZ(p, def_->townCenter) < def_->townRadius; }
bool Field::InBossArena(const VECTOR& p) const { return DistXZ(p, def_->bossPos) < def_->bossArenaRadius; }

} // namespace ag
