#pragma once
// キーボード・マウス入力。ゲーム内の操作は Action 経由で参照し、
// 将来のキーコンフィグ（設定画面での割り当て変更）に対応できるようにする。
#include <array>

namespace ag {

enum class Action {
    MoveForward,
    MoveBack,
    MoveLeft,
    MoveRight,
    Dash,       // 押している間ダッシュ
    Dodge,      // 回避ステップ
    Skill1,
    Skill2,
    Skill3,
    Skill4,
    UseItem,    // クイックアイテム（回復薬）
    Menu,       // ESC メニュー
    Confirm,
    QuickSave,
    QuickLoad,
    Count
};

enum class MouseButton { Left, Right, Middle };

class Input {
public:
    static Input& Get();

    void Update();

    bool KeyDown(int keyCode) const;      // 押されている
    bool KeyPressed(int keyCode) const;   // このフレームで押された
    bool KeyReleased(int keyCode) const;

    bool ActionDown(Action a) const;
    bool ActionPressed(Action a) const;
    int  ActionKey(Action a) const { return bindings_[static_cast<int>(a)]; }
    void Bind(Action a, int keyCode) { bindings_[static_cast<int>(a)] = keyCode; }

    bool MouseDown(MouseButton b) const;
    bool MousePressed(MouseButton b) const;
    bool MouseReleased(MouseButton b) const;
    int  MouseX() const { return mouseX_; }
    int  MouseY() const { return mouseY_; }
    int  MouseDX() const { return mouseDX_; }
    int  MouseDY() const { return mouseDY_; }
    int  Wheel() const { return wheel_; }

    // カメラ回転中はカーソルを隠して中央に固定する
    void SetMouseCaptured(bool captured);
    bool IsMouseCaptured() const { return captured_; }

    // UI がクリックを処理したら呼ぶ（同フレームで攻撃が出ないように）
    void ConsumeMouse() { mouseConsumed_ = true; }
    bool IsMouseConsumed() const { return mouseConsumed_; }

private:
    Input();
    static int MouseMask(MouseButton b);

    std::array<char, 256> keys_{};
    std::array<char, 256> prevKeys_{};
    std::array<int, static_cast<int>(Action::Count)> bindings_{};
    int mouse_ = 0;
    int prevMouse_ = 0;
    int mouseX_ = 0, mouseY_ = 0;
    int mouseDX_ = 0, mouseDY_ = 0;
    int wheel_ = 0;
    bool captured_ = false;
    bool mouseConsumed_ = false;
};

} // namespace ag
