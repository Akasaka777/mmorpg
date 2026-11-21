#pragma once
// キャラクターのアニメーション状態。
// 必須: 待機・移動・攻撃・被弾・死亡。Skill は未設定なら Attack を流用する。

namespace ag {

enum class AnimState {
    Idle,
    Move,
    Attack,
    Skill,
    Hit,
    Death,
    Guard,   // 任意（未設定なら Idle を流用）
    Count
};

inline const char* AnimStateKey(AnimState s)
{
    switch (s) {
    case AnimState::Idle:   return "idle";
    case AnimState::Move:   return "move";
    case AnimState::Attack: return "attack";
    case AnimState::Skill:  return "skill";
    case AnimState::Hit:    return "hit";
    case AnimState::Death:  return "death";
    case AnimState::Guard:  return "guard";
    default:                return "";
    }
}

inline bool AnimStateLoops(AnimState s) { return s == AnimState::Idle || s == AnimState::Move || s == AnimState::Guard; }

} // namespace ag
