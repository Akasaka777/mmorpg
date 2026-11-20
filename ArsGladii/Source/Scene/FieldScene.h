#pragma once
// フィールド（層）シーン。プレイヤー・ワールド・UI をまとめ、イベントを UI へ反映する。
#include "Scene.h"
#include "../Core/Camera.h"
#include "../Entity/Player.h"
#include "../World/World.h"
#include "../UI/HUD.h"
#include "../UI/MessageLog.h"
#include "../UI/FloatingText.h"
#include "../UI/ResultScreen.h"
#include "../UI/Menu.h"
#include <vector>

namespace ag {

class FieldScene : public Scene, public EventSink {
public:
    explicit FieldScene(bool continueGame);

    void Enter() override;
    void Exit() override;
    void Update(float dt) override;
    void Draw() override;

    void Push(const GameEvent& e) override { events_.push_back(e); }

private:
    void StartNewGame();
    bool LoadGame();
    void SaveGame();
    void ProcessEvents();
    void GrantRewards(const GameEvent& e, int& levelsGained);
    void HandleMenuCommand(MenuCommand cmd);
    void SetupRenderState() const;

    bool continueGame_;
    int floor_ = 1;
    Camera camera_;
    Player player_;
    World world_;
    HUD hud_;
    MessageLog log_;
    FloatingTextSystem floating_;
    ResultScreen result_;
    Menu menu_;
    std::vector<GameEvent> events_;
    MenuCommand pendingCommand_ = MenuCommand::None;
    float deathTimer_ = 0.0f;
    bool wasInBossArena_ = false;
};

} // namespace ag
