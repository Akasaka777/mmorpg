#pragma once
// ゲーム中の常時表示 UI
//   左上: キャラアイコン＋HP/MP/EXP バー（独自デザイン）
//   上中央: ボス HP バー
//   右上: ミニマップ
//   下中央: スキルホットバー（クールダウン表示）
#include <string>

namespace ag {

class Player;
class World;
class Camera;

class HUD {
public:
    void Update(float dt, const Player& player, const World& world);
    void Draw(const Player& player, const World& world, const Camera& camera) const;

    void ShowWarning(const std::string& text);
    void ShowBanner(const std::string& title, const std::string& sub, unsigned int color);
    void ToggleHelp() { showHelp_ = !showHelp_; }

private:
    void DrawStatus(const Player& player) const;
    void DrawIcon(int x, int y, int size) const;
    void DrawBossBar(const World& world) const;
    void DrawMinimap(const Player& player, const World& world, const Camera& camera) const;
    void DrawHotbar(const Player& player) const;
    void DrawCenterTexts() const;
    void DrawHelp() const;

    float hpTrail_ = 1.0f;
    float bossTrail_ = 1.0f;
    std::string warning_;
    float warningTimer_ = 0.0f;
    std::string bannerTitle_, bannerSub_;
    unsigned int bannerColor_ = 0;
    float bannerTimer_ = 0.0f;
    bool showHelp_ = true;
    float time_ = 0.0f;
};

} // namespace ag
