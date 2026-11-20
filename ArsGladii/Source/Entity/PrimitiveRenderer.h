#pragma once
// 外部モデルが無いときのサンプル描画（球・カプセル・円錐の組み合わせ）。
// 待機・移動・攻撃・スキル・被弾・死亡の各状態を手続き的にアニメーションさせる。
#include "AnimState.h"
#include "../Combat/WeaponType.h"
#include "DxLib.h"

namespace ag {

enum class BodyShape { Humanoid, Beast };

struct PrimitiveLook {
    BodyShape shape = BodyShape::Humanoid;
    unsigned int mainColor = 0x3355AA;  // 0xRRGGBB
    unsigned int subColor = 0x222233;
    unsigned int skinColor = 0xF0C8A0;
    unsigned int eyeColor = 0x202020;
    float scale = 1.0f;
    WeaponType weapon = WeaponType::None;
    bool shield = false;
    bool horns = false;
    bool thrustStyle = false; // スキル・攻撃を突きモーションにする
};

struct PrimitivePose {
    AnimState state = AnimState::Idle;
    float time = 0.0f;       // 状態の経過時間
    float progress = 0.0f;   // 0～1
    float flash = 0.0f;      // 被弾フラッシュ 0～1
    float opacity = 1.0f;
    float spin = 0.0f;       // 全周スキル用の追加回転
    unsigned int trailColor = 0; // 0 以外ならスキルの軌跡色
};

class PrimitiveRenderer {
public:
    static void Draw(const PrimitiveLook& look, const PrimitivePose& pose, const VECTOR& pos, float yaw);
    static void DrawShadow(const VECTOR& pos, float radius);
    // 地面に半透明の円を描く（テレグラフ・エリア表示用）
    static void DrawGroundCircle(const VECTOR& center, float radius, unsigned int color, int alpha, int segments = 40);
    static void DrawGroundRing(const VECTOR& center, float radius, float width, unsigned int color, int alpha, int segments = 64);
    static void DrawGroundSector(const VECTOR& center, float yaw, float radius, float halfArc, unsigned int color, int alpha, int segments = 24);

    static unsigned int ToDx(unsigned int rgb);
    static unsigned int Mix(unsigned int rgbA, unsigned int rgbB, float t);

private:
    static void DrawHumanoid(const PrimitiveLook& look, const PrimitivePose& pose, const MATRIX& world);
    static void DrawBeast(const PrimitiveLook& look, const PrimitivePose& pose, const MATRIX& world);
    static void DrawWeapon(WeaponType w, const VECTOR& hand, const VECTOR& dir, const MATRIX& world, unsigned int tint, float flash);
};

} // namespace ag
