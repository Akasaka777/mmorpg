#pragma once
// ゲーム全体の定数とユーザー設定

namespace ag {

constexpr int kScreenWidth = 1920;
constexpr int kScreenHeight = 1080;
constexpr const char* kGameTitle = "Ars Gladii";
// バージョン: X.Y.Z（X=大規模改修 / Y=機能追加 / Z=不具合修正）。VERSION・CHANGELOG.md と合わせて更新する
constexpr const char* kVersion = "0.2.0";

// ワールド単位: 1.0 = 約 1cm（キャラの身長 ≒ 170）
constexpr float kWorldUnitPerMeter = 100.0f;

// 設定画面で変更できる値。save/config.ini に保存される。
struct GameSettings {
    float mouseSensitivity = 1.0f;   // 0.2 ～ 3.0
    bool  invertY = false;
    float cameraDistance = 650.0f;   // 300 ～ 1400
    int   bgmVolume = 70;            // 0 ～ 100
    int   seVolume = 80;             // 0 ～ 100
    bool  showDamageNumbers = true;
    bool  fullscreen = false;        // 次回起動時に反映

    bool Load(const char* path);
    bool Save(const char* path) const;
};

GameSettings& Settings();

constexpr const char* kConfigPath = "save/config.ini";
constexpr const char* kSavePath = "save/save01.dat";

} // namespace ag
