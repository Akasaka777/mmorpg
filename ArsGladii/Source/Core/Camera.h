#pragma once
// 三人称カメラ。右ドラッグで回転、ホイールでズーム。
#include "DxLib.h"

namespace ag {

class Camera {
public:
    void Reset(const VECTOR& target, float yaw);
    void HandleInput(bool allowRotate);
    void Update(const VECTOR& followTarget, float dt);
    void Apply() const;

    float Yaw() const { return yaw_; }
    float Pitch() const { return pitch_; }
    VECTOR Position() const { return position_; }
    VECTOR Target() const { return target_; }
    VECTOR ForwardXZ() const;
    VECTOR RightXZ() const;

    void AddShake(float strength, float duration);

private:
    float yaw_ = 0.0f;
    float pitch_ = -0.35f;
    float distance_ = 650.0f;
    float targetDistance_ = 650.0f;
    VECTOR target_ = VGet(0, 0, 0);
    VECTOR position_ = VGet(0, 0, 0);
    float shakeStrength_ = 0.0f;
    float shakeTime_ = 0.0f;
};

} // namespace ag
