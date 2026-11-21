#include "Animator.h"
#include "../Core/AssetManager.h"
#include "../Core/MathUtil.h"

namespace ag {

namespace {
constexpr float kBlendTime = 0.15f;

// プリミティブ描画時に使う各状態の標準秒数
float DefaultDuration(AnimState s)
{
    switch (s) {
    case AnimState::Idle:   return 2.0f;
    case AnimState::Move:   return 0.7f;
    case AnimState::Attack: return 0.5f;
    case AnimState::Skill:  return 0.8f;
    case AnimState::Hit:    return 0.35f;
    case AnimState::Death:  return 1.2f;
    case AnimState::Guard:  return 1.0f;
    default:                return 1.0f;
    }
}
}

Animator::~Animator() { Release(); }

Animator::Animator(Animator&& other) noexcept { *this = std::move(other); }

Animator& Animator::operator=(Animator&& other) noexcept
{
    if (this == &other) return *this;
    Release();
    def_ = other.def_;
    model_ = other.model_;
    state_ = other.state_;
    time_ = other.time_;
    duration_ = other.duration_;
    attach_ = other.attach_;
    attachTotal_ = other.attachTotal_;
    playRate_ = other.playRate_;
    animTime_ = other.animTime_;
    prevAttach_ = other.prevAttach_;
    blend_ = other.blend_;
    other.model_ = -1;
    other.def_ = nullptr;
    other.attach_ = other.prevAttach_ = -1;
    return *this;
}

void Animator::Init(const std::string& modelKey)
{
    Release();
    def_ = AssetManager::Get().GetModelDef(modelKey);
    if (def_) model_ = AssetManager::Get().CreateModelInstance(modelKey);
    if (model_ == -1) def_ = nullptr;
    Play(AnimState::Idle, 0.0f, true);
}

void Animator::Release()
{
    if (model_ != -1) MV1DeleteModel(model_);
    model_ = -1;
    attach_ = prevAttach_ = -1;
}

int Animator::AttachClip(AnimState state, float& outTotal)
{
    outTotal = 0.0f;
    if (!def_) return -1;
    const AnimClipRef* clip = &def_->clips[static_cast<int>(state)];
    if (!clip->Valid() && state == AnimState::Skill) clip = &def_->clips[static_cast<int>(AnimState::Attack)];
    if (!clip->Valid() && state == AnimState::Guard) clip = &def_->clips[static_cast<int>(AnimState::Idle)];
    if (!clip->Valid()) clip = &def_->clips[static_cast<int>(AnimState::Idle)];
    if (!clip->Valid()) return -1;
    int a = MV1AttachAnim(model_, clip->index, clip->srcHandle, FALSE);
    if (a != -1) outTotal = MV1GetAttachAnimTotalTime(model_, a);
    return a;
}

void Animator::Play(AnimState state, float durationHint, bool restart)
{
    if (state == state_ && !restart && (attach_ != -1 || model_ == -1)) return;
    state_ = state;
    time_ = 0.0f;
    duration_ = durationHint > 0.0f ? durationHint : DefaultDuration(state);

    if (model_ == -1) return;

    if (prevAttach_ != -1) MV1DetachAnim(model_, prevAttach_);
    prevAttach_ = attach_;
    blend_ = (prevAttach_ != -1) ? 0.0f : 1.0f;

    attach_ = AttachClip(state, attachTotal_);
    animTime_ = 0.0f;
    const AnimClipRef& clip = def_->clips[static_cast<int>(state)];
    const float speed = clip.Valid() ? clip.speed : 1.0f;
    if (durationHint > 0.0f && attachTotal_ > 0.0f && !AnimStateLoops(state)) {
        playRate_ = attachTotal_ / durationHint;
    } else {
        playRate_ = def_->animFps * speed;
        if (attachTotal_ > 0.0f && durationHint <= 0.0f && !AnimStateLoops(state))
            duration_ = attachTotal_ / playRate_;
    }
}

void Animator::Update(float dt)
{
    time_ += dt;
    if (model_ == -1) return;

    if (attach_ != -1) {
        animTime_ += dt * playRate_;
        if (AnimStateLoops(state_)) {
            if (attachTotal_ > 0.0f) while (animTime_ >= attachTotal_) animTime_ -= attachTotal_;
        } else if (animTime_ > attachTotal_) {
            animTime_ = attachTotal_;
        }
        MV1SetAttachAnimTime(model_, attach_, animTime_);
    }

    if (prevAttach_ != -1) {
        blend_ += dt / kBlendTime;
        if (blend_ >= 1.0f) {
            blend_ = 1.0f;
            MV1DetachAnim(model_, prevAttach_);
            prevAttach_ = -1;
        } else {
            MV1SetAttachAnimBlendRate(model_, prevAttach_, 1.0f - blend_);
        }
    }
    if (attach_ != -1) MV1SetAttachAnimBlendRate(model_, attach_, blend_);
}

float Animator::Progress() const
{
    if (duration_ <= 0.0f) return 1.0f;
    if (AnimStateLoops(state_)) {
        float t = time_ / duration_;
        return t - static_cast<float>(static_cast<int>(t));
    }
    return Clamp(time_ / duration_, 0.0f, 1.0f);
}

void Animator::DrawModel(const VECTOR& pos, float yaw, float scaleMul, const COLOR_F& colorScale, float opacity) const
{
    if (model_ == -1 || !def_) return;
    const float s = def_->scale * scaleMul;
    MV1SetScale(model_, VGet(s, s, s));
    MV1SetRotationXYZ(model_, VGet(0.0f, yaw + def_->rotYOffset, 0.0f));
    MV1SetPosition(model_, VAdd(pos, VGet(0.0f, def_->yOffset, 0.0f)));
    MV1SetDifColorScale(model_, colorScale);
    MV1SetOpacityRate(model_, opacity);
    MV1DrawModel(model_);
}

} // namespace ag
