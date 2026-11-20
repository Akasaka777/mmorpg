#include "Input.h"
#include "Config.h"
#include "DxLib.h"

namespace ag {

Input& Input::Get()
{
    static Input instance;
    return instance;
}

Input::Input()
{
    bindings_[static_cast<int>(Action::MoveForward)] = KEY_INPUT_W;
    bindings_[static_cast<int>(Action::MoveBack)]    = KEY_INPUT_S;
    bindings_[static_cast<int>(Action::MoveLeft)]    = KEY_INPUT_A;
    bindings_[static_cast<int>(Action::MoveRight)]   = KEY_INPUT_D;
    bindings_[static_cast<int>(Action::Dash)]        = KEY_INPUT_LSHIFT;
    bindings_[static_cast<int>(Action::Dodge)]       = KEY_INPUT_SPACE;
    bindings_[static_cast<int>(Action::Skill1)]      = KEY_INPUT_1;
    bindings_[static_cast<int>(Action::Skill2)]      = KEY_INPUT_2;
    bindings_[static_cast<int>(Action::Skill3)]      = KEY_INPUT_3;
    bindings_[static_cast<int>(Action::Skill4)]      = KEY_INPUT_4;
    bindings_[static_cast<int>(Action::UseItem)]     = KEY_INPUT_R;
    bindings_[static_cast<int>(Action::Menu)]        = KEY_INPUT_ESCAPE;
    bindings_[static_cast<int>(Action::Confirm)]     = KEY_INPUT_RETURN;
    bindings_[static_cast<int>(Action::QuickSave)]   = KEY_INPUT_F5;
    bindings_[static_cast<int>(Action::QuickLoad)]   = KEY_INPUT_F9;
}

int Input::MouseMask(MouseButton b)
{
    switch (b) {
    case MouseButton::Left:   return MOUSE_INPUT_LEFT;
    case MouseButton::Right:  return MOUSE_INPUT_RIGHT;
    case MouseButton::Middle: return MOUSE_INPUT_MIDDLE;
    }
    return 0;
}

void Input::Update()
{
    prevKeys_ = keys_;
    GetHitKeyStateAll(keys_.data());

    prevMouse_ = mouse_;
    mouse_ = GetMouseInput();
    mouseConsumed_ = false;

    int x = 0, y = 0;
    GetMousePoint(&x, &y);
    if (captured_) {
        const int cx = kScreenWidth / 2, cy = kScreenHeight / 2;
        mouseDX_ = x - cx;
        mouseDY_ = y - cy;
        SetMousePoint(cx, cy);
    } else {
        mouseDX_ = x - mouseX_;
        mouseDY_ = y - mouseY_;
        mouseX_ = x;
        mouseY_ = y;
    }
    wheel_ = GetMouseWheelRotVol();
}

bool Input::KeyDown(int keyCode) const { return keys_[keyCode] != 0; }
bool Input::KeyPressed(int keyCode) const { return keys_[keyCode] != 0 && prevKeys_[keyCode] == 0; }
bool Input::KeyReleased(int keyCode) const { return keys_[keyCode] == 0 && prevKeys_[keyCode] != 0; }

bool Input::ActionDown(Action a) const { return KeyDown(ActionKey(a)); }
bool Input::ActionPressed(Action a) const { return KeyPressed(ActionKey(a)); }

bool Input::MouseDown(MouseButton b) const { return (mouse_ & MouseMask(b)) != 0; }
bool Input::MousePressed(MouseButton b) const
{
    return !mouseConsumed_ && (mouse_ & MouseMask(b)) != 0 && (prevMouse_ & MouseMask(b)) == 0;
}
bool Input::MouseReleased(MouseButton b) const
{
    return (mouse_ & MouseMask(b)) == 0 && (prevMouse_ & MouseMask(b)) != 0;
}

void Input::SetMouseCaptured(bool captured)
{
    if (captured_ == captured) return;
    captured_ = captured;
    if (captured) {
        SetMouseDispFlag(FALSE);
        SetMousePoint(kScreenWidth / 2, kScreenHeight / 2);
    } else {
        SetMousePoint(mouseX_, mouseY_);
        SetMouseDispFlag(TRUE);
    }
    mouseDX_ = mouseDY_ = 0;
}

} // namespace ag
