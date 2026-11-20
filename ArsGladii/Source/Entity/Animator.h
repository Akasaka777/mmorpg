#pragma once
// アニメーション状態管理。
// 外部モデルがあれば MV1 アニメを再生（クロスフェード付き）、
// 無ければ状態と経過時間だけを管理し、描画は PrimitiveRenderer が行う。
#include "AnimState.h"
#include "DxLib.h"
#include <string>

namespace ag {

struct ModelDef;

class Animator {
public:
    Animator() = default;
    ~Animator();
    Animator(const Animator&) = delete;
    Animator& operator=(const Animator&) = delete;
    Animator(Animator&& other) noexcept;
    Animator& operator=(Animator&& other) noexcept;

    void Init(const std::string& modelKey);
    void Release();

    // durationHint > 0 なら、非ループのアニメをその秒数で再生しきるよう速度を合わせる
    void Play(AnimState state, float durationHint = 0.0f, bool restart = false);
    void Update(float dt);

    AnimState State() const { return state_; }
    float Time() const { return time_; }
    float Duration() const { return duration_; }
    float Progress() const; // 0～1（ループは周期内の位置）

    bool HasModel() const { return model_ != -1; }
    void DrawModel(const VECTOR& pos, float yaw, float scaleMul, const COLOR_F& colorScale, float opacity) const;

private:
    int AttachClip(AnimState state, float& outTotal);

    const ModelDef* def_ = nullptr;
    int model_ = -1;
    AnimState state_ = AnimState::Idle;
    float time_ = 0.0f;
    float duration_ = 1.0f;

    int attach_ = -1;
    float attachTotal_ = 0.0f;
    float playRate_ = 30.0f;    // アニメ時間単位 / 秒
    float animTime_ = 0.0f;

    int prevAttach_ = -1;
    float blend_ = 1.0f;
};

} // namespace ag
