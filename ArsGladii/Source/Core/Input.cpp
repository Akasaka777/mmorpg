#include "Input.h"
#include "Config.h"
#include "KeyValueFile.h"
#include "DxLib.h"
#include <string>

namespace ag {

Input& Input::Get()
{
    static Input instance;
    return instance;
}

Input::Input() { ResetBindings(); }

void Input::ResetBindings()
{
    bindings_[static_cast<int>(Action::MoveForward)] = KEY_INPUT_W;
    bindings_[static_cast<int>(Action::MoveBack)]    = KEY_INPUT_S;
    bindings_[static_cast<int>(Action::MoveLeft)]    = KEY_INPUT_A;
    bindings_[static_cast<int>(Action::MoveRight)]   = KEY_INPUT_D;
    bindings_[static_cast<int>(Action::Dash)]        = KEY_INPUT_LCONTROL;
    bindings_[static_cast<int>(Action::Dodge)]       = KEY_INPUT_LSHIFT;
    bindings_[static_cast<int>(Action::Jump)]        = KEY_INPUT_SPACE;
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

void Input::Rebind(Action a, int keyCode)
{
    if (!IsRebindable(a)) return;
    const int old = ActionKey(a);
    for (int i = 0; i < static_cast<int>(Action::Count); ++i) {
        if (static_cast<Action>(i) != a && bindings_[i] == keyCode && IsRebindable(static_cast<Action>(i))) bindings_[i] = old;
    }
    Bind(a, keyCode);
}

void Input::LoadBindings(const KeyValueFile& kv)
{
    for (int i = 0; i < static_cast<int>(Action::Count); ++i) {
        const Action a = static_cast<Action>(i);
        if (!IsRebindable(a)) continue;
        const int code = kv.GetInt(std::string("key_") + ActionId(a), -1);
        if (code >= 0 && code < 256 && KeyName(code)) bindings_[i] = code;
    }
}

void Input::SaveBindings(KeyValueFile& kv) const
{
    for (int i = 0; i < static_cast<int>(Action::Count); ++i) {
        const Action a = static_cast<Action>(i);
        if (IsRebindable(a)) kv.SetInt(std::string("key_") + ActionId(a), bindings_[i]);
    }
}

bool Input::IsRebindable(Action a)
{
    // ESC（メニュー）と Enter（決定）は固定にして、操作不能になるのを防ぐ
    return a != Action::Menu && a != Action::Confirm && a != Action::Count;
}

const char* Input::ActionName(Action a)
{
    switch (a) {
    case Action::MoveForward: return "前進";
    case Action::MoveBack:    return "後退";
    case Action::MoveLeft:    return "左移動";
    case Action::MoveRight:   return "右移動";
    case Action::Dash:        return "ダッシュ（長押し）";
    case Action::Dodge:       return "回避";
    case Action::Jump:        return "ジャンプ";
    case Action::Skill1:      return "スキル1";
    case Action::Skill2:      return "スキル2";
    case Action::Skill3:      return "スキル3";
    case Action::Skill4:      return "スキル4";
    case Action::UseItem:     return "クイックアイテム";
    case Action::Menu:        return "メニュー";
    case Action::Confirm:     return "決定";
    case Action::QuickSave:   return "クイックセーブ";
    case Action::QuickLoad:   return "クイックロード";
    default:                  return "";
    }
}

const char* Input::ActionId(Action a)
{
    switch (a) {
    case Action::MoveForward: return "move_forward";
    case Action::MoveBack:    return "move_back";
    case Action::MoveLeft:    return "move_left";
    case Action::MoveRight:   return "move_right";
    case Action::Dash:        return "dash";
    case Action::Dodge:       return "dodge";
    case Action::Jump:        return "jump";
    case Action::Skill1:      return "skill1";
    case Action::Skill2:      return "skill2";
    case Action::Skill3:      return "skill3";
    case Action::Skill4:      return "skill4";
    case Action::UseItem:     return "use_item";
    case Action::Menu:        return "menu";
    case Action::Confirm:     return "confirm";
    case Action::QuickSave:   return "quick_save";
    case Action::QuickLoad:   return "quick_load";
    default:                  return "";
    }
}

namespace {
struct KeyNameEntry { int code; const char* name; };
const KeyNameEntry kKeyNames[] = {
    { KEY_INPUT_A, "A" }, { KEY_INPUT_B, "B" }, { KEY_INPUT_C, "C" }, { KEY_INPUT_D, "D" }, { KEY_INPUT_E, "E" },
    { KEY_INPUT_F, "F" }, { KEY_INPUT_G, "G" }, { KEY_INPUT_H, "H" }, { KEY_INPUT_I, "I" }, { KEY_INPUT_J, "J" },
    { KEY_INPUT_K, "K" }, { KEY_INPUT_L, "L" }, { KEY_INPUT_M, "M" }, { KEY_INPUT_N, "N" }, { KEY_INPUT_O, "O" },
    { KEY_INPUT_P, "P" }, { KEY_INPUT_Q, "Q" }, { KEY_INPUT_R, "R" }, { KEY_INPUT_S, "S" }, { KEY_INPUT_T, "T" },
    { KEY_INPUT_U, "U" }, { KEY_INPUT_V, "V" }, { KEY_INPUT_W, "W" }, { KEY_INPUT_X, "X" }, { KEY_INPUT_Y, "Y" },
    { KEY_INPUT_Z, "Z" },
    { KEY_INPUT_0, "0" }, { KEY_INPUT_1, "1" }, { KEY_INPUT_2, "2" }, { KEY_INPUT_3, "3" }, { KEY_INPUT_4, "4" },
    { KEY_INPUT_5, "5" }, { KEY_INPUT_6, "6" }, { KEY_INPUT_7, "7" }, { KEY_INPUT_8, "8" }, { KEY_INPUT_9, "9" },
    { KEY_INPUT_F1, "F1" }, { KEY_INPUT_F2, "F2" }, { KEY_INPUT_F3, "F3" }, { KEY_INPUT_F4, "F4" },
    { KEY_INPUT_F5, "F5" }, { KEY_INPUT_F6, "F6" }, { KEY_INPUT_F7, "F7" }, { KEY_INPUT_F8, "F8" },
    { KEY_INPUT_F9, "F9" }, { KEY_INPUT_F10, "F10" }, { KEY_INPUT_F11, "F11" }, { KEY_INPUT_F12, "F12" },
    { KEY_INPUT_SPACE, "Space" }, { KEY_INPUT_LSHIFT, "左Shift" }, { KEY_INPUT_RSHIFT, "右Shift" },
    { KEY_INPUT_LCONTROL, "左Ctrl" }, { KEY_INPUT_RCONTROL, "右Ctrl" }, { KEY_INPUT_LALT, "左Alt" }, { KEY_INPUT_RALT, "右Alt" },
    { KEY_INPUT_TAB, "Tab" }, { KEY_INPUT_RETURN, "Enter" }, { KEY_INPUT_BACK, "BackSpace" }, { KEY_INPUT_ESCAPE, "ESC" },
    { KEY_INPUT_UP, "↑" }, { KEY_INPUT_DOWN, "↓" }, { KEY_INPUT_LEFT, "←" }, { KEY_INPUT_RIGHT, "→" },
    { KEY_INPUT_INSERT, "Insert" }, { KEY_INPUT_DELETE, "Delete" }, { KEY_INPUT_HOME, "Home" }, { KEY_INPUT_END, "End" },
    { KEY_INPUT_PGUP, "PageUp" }, { KEY_INPUT_PGDN, "PageDown" },
    { KEY_INPUT_NUMPAD0, "テンキー0" }, { KEY_INPUT_NUMPAD1, "テンキー1" }, { KEY_INPUT_NUMPAD2, "テンキー2" },
    { KEY_INPUT_NUMPAD3, "テンキー3" }, { KEY_INPUT_NUMPAD4, "テンキー4" }, { KEY_INPUT_NUMPAD5, "テンキー5" },
    { KEY_INPUT_NUMPAD6, "テンキー6" }, { KEY_INPUT_NUMPAD7, "テンキー7" }, { KEY_INPUT_NUMPAD8, "テンキー8" },
    { KEY_INPUT_NUMPAD9, "テンキー9" },
    { KEY_INPUT_MINUS, "-" }, { KEY_INPUT_COMMA, "," }, { KEY_INPUT_PERIOD, "." }, { KEY_INPUT_SLASH, "/" },
    { KEY_INPUT_SEMICOLON, ";" }, { KEY_INPUT_COLON, ":" }, { KEY_INPUT_LBRACKET, "[" }, { KEY_INPUT_RBRACKET, "]" },
    { KEY_INPUT_AT, "@" }, { KEY_INPUT_BACKSLASH, "\\" }, { KEY_INPUT_YEN, "￥" }, { KEY_INPUT_PREVTRACK, "^" },
};
}

const char* Input::KeyName(int keyCode)
{
    for (const auto& k : kKeyNames) if (k.code == keyCode) return k.name;
    return nullptr;
}

int Input::DetectPressedKey() const
{
    for (const auto& k : kKeyNames) if (KeyPressed(k.code)) return k.code;
    return -1;
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
