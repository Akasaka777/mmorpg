#include "Camera.h"
#include "Input.h"
#include "Config.h"
#include "MathUtil.h"
#include "Random.h"

namespace ag {

namespace {
constexpr float kTargetHeight = 140.0f;
constexpr float kMinPitch = -1.25f;
constexpr float kMaxPitch = 0.30f;
constexpr float kMinDist = 300.0f;
constexpr float kMaxDist = 1400.0f;
}

void Camera::Reset(const VECTOR& target, float yaw)
{
    yaw_ = yaw;
    pitch_ = -0.35f;
    distance_ = targetDistance_ = Settings().cameraDistance;
    target_ = VAdd(target, VGet(0, kTargetHeight, 0));
    Update(target, 0.0f);
}

void Camera::HandleInput(bool allowRotate)
{
    Input& in = Input::Get();
    const bool rotating = allowRotate && in.MouseDown(MouseButton::Right);
    in.SetMouseCaptured(rotating);
    if (rotating) {
        const float sens = 0.0035f * Settings().mouseSensitivity;
        yaw_ = WrapAngle(yaw_ + in.MouseDX() * sens);
        const float dy = in.MouseDY() * sens * (Settings().invertY ? 1.0f : -1.0f);
        pitch_ = Clamp(pitch_ + dy, kMinPitch, kMaxPitch);
    }
    if (allowRotate && in.Wheel() != 0) {
        targetDistance_ = Clamp(targetDistance_ - in.Wheel() * 60.0f, kMinDist, kMaxDist);
        Settings().cameraDistance = targetDistance_;
    }
}

void Camera::Update(const VECTOR& followTarget, float dt)
{
    const VECTOR want = VAdd(followTarget, VGet(0, kTargetHeight, 0));
    const float follow = dt > 0.0f ? Clamp(dt * 12.0f, 0.0f, 1.0f) : 1.0f;
    target_ = VAdd(target_, VScale(VSub(want, target_), follow));
    distance_ = Lerp(distance_, targetDistance_, dt > 0.0f ? Clamp(dt * 8.0f, 0.0f, 1.0f) : 1.0f);

    const VECTOR dir = VGet(std::sin(yaw_) * std::cos(pitch_), std::sin(pitch_), std::cos(yaw_) * std::cos(pitch_));
    position_ = VSub(target_, VScale(dir, distance_));
    if (position_.y < 30.0f) position_.y = 30.0f;

    if (shakeTime_ > 0.0f) {
        shakeTime_ -= dt;
        const float s = shakeStrength_ * Clamp(shakeTime_ * 4.0f, 0.0f, 1.0f);
        position_ = VAdd(position_, VGet(Random::Float(-s, s), Random::Float(-s, s), Random::Float(-s, s)));
    }
}

void Camera::Apply() const
{
    SetCameraNearFar(10.0f, 30000.0f);
    SetupCamera_Perspective(DegToRad(60.0f));
    SetCameraPositionAndTarget_UpVecY(position_, target_);
}

VECTOR Camera::ForwardXZ() const { return DirFromYaw(yaw_); }
VECTOR Camera::RightXZ() const { return VGet(std::cos(yaw_), 0.0f, -std::sin(yaw_)); }

void Camera::AddShake(float strength, float duration)
{
    shakeStrength_ = strength;
    shakeTime_ = duration;
}

} // namespace ag
