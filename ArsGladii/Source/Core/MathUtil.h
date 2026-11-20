#pragma once
// 数学ユーティリティ（XZ平面ベースの角度・距離計算など）
#include "DxLib.h"
#include <cmath>

namespace ag {

constexpr float kPi = 3.14159265358979f;
constexpr float kTwoPi = kPi * 2.0f;

inline float Clamp(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline int ClampI(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }
inline float DegToRad(float d) { return d * kPi / 180.0f; }

// 角度を -PI ～ PI に正規化
inline float WrapAngle(float a)
{
    while (a > kPi) a -= kTwoPi;
    while (a < -kPi) a += kTwoPi;
    return a;
}

// cur から target へ最大 maxStep だけ回転させる
inline float ApproachAngle(float cur, float target, float maxStep)
{
    float d = WrapAngle(target - cur);
    if (std::fabs(d) <= maxStep) return target;
    return WrapAngle(cur + (d > 0 ? maxStep : -maxStep));
}

// yaw=0 で +Z 方向を向く（DxLib の左手座標系、Y 上）
inline VECTOR DirFromYaw(float yaw) { return VGet(std::sin(yaw), 0.0f, std::cos(yaw)); }
inline float YawFromDir(const VECTOR& d) { return std::atan2(d.x, d.z); }

inline float DistXZSq(const VECTOR& a, const VECTOR& b)
{
    float dx = a.x - b.x, dz = a.z - b.z;
    return dx * dx + dz * dz;
}
inline float DistXZ(const VECTOR& a, const VECTOR& b) { return std::sqrt(DistXZSq(a, b)); }

// target が「origin から yaw 方向へ range 以内、かつ扇形 halfArc 以内」にあるか
inline bool InArc(const VECTOR& origin, float yaw, const VECTOR& target, float range, float halfArc, float targetRadius)
{
    float dist = DistXZ(origin, target);
    if (dist > range + targetRadius) return false;
    if (dist < targetRadius + 1.0f) return true;
    float ang = std::atan2(target.x - origin.x, target.z - origin.z);
    return std::fabs(WrapAngle(ang - yaw)) <= halfArc;
}

} // namespace ag
