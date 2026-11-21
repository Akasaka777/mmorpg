#pragma once
// アプリケーション本体: DxLib の初期化・メインループ・シーン切り替え
#include "../Scene/Scene.h"
#include <memory>

namespace ag {

class Game {
public:
    bool Init();
    void Run();
    void Shutdown();

private:
    void ChangeScene(const SceneRequest& req);

    std::unique_ptr<Scene> scene_;
    bool running_ = true;
    bool dxInitialized_ = false;
};

} // namespace ag
