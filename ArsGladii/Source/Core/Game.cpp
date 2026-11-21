#include "Game.h"
#include "Config.h"
#include "Input.h"
#include "FontManager.h"
#include "AssetManager.h"
#include "../Scene/TitleScene.h"
#include "../Scene/FieldScene.h"
#include "DxLib.h"
#include <algorithm>

namespace ag {

bool Game::Init()
{
    Settings().Load(kConfigPath);

    SetOutApplicationLogValidFlag(FALSE);
    SetUseCharCodeFormat(DX_CHARCODEFORMAT_UTF8); // ソースは UTF-8（/utf-8 オプション）
    ChangeWindowMode(Settings().fullscreen ? FALSE : TRUE);
    SetGraphMode(kScreenWidth, kScreenHeight, 32);
    SetMainWindowText(kGameTitle);
    SetAlwaysRunFlag(TRUE);
    SetWindowSizeChangeEnableFlag(TRUE, TRUE);

    // ウィンドウモードでデスクトップが 1920x1080 より小さい場合は縮小表示
    if (!Settings().fullscreen) {
        int dw = 0, dh = 0, depth = 0;
        if (GetDefaultState(&dw, &dh, &depth) == 0 && dw > 0 && dh > 0) {
            const double rate = std::min(1.0, std::min((dw - 40) / static_cast<double>(kScreenWidth), (dh - 100) / static_cast<double>(kScreenHeight)));
            if (rate < 1.0) SetWindowSizeExtendRate(rate);
        }
    }

    if (DxLib_Init() == -1) return false;
    dxInitialized_ = true;

    SetDrawScreen(DX_SCREEN_BACK);
    SetUseZBuffer3D(TRUE);
    SetWriteZBuffer3D(TRUE);
    SetMouseDispFlag(TRUE);

    FontManager::Get().Init();
    ChangeScene(SceneRequest{ SceneId::Title, false });
    return true;
}

void Game::ChangeScene(const SceneRequest& req)
{
    if (req.next == SceneId::None) return;
    if (scene_) scene_->Exit();
    scene_.reset();
    switch (req.next) {
    case SceneId::Title: scene_ = std::make_unique<TitleScene>(); break;
    case SceneId::Field: scene_ = std::make_unique<FieldScene>(req.continueGame); break;
    case SceneId::Quit:  running_ = false; return;
    default: return;
    }
    scene_->Enter();
}

void Game::Run()
{
    LONGLONG prev = GetNowHiPerformanceCount();
    while (running_ && ProcessMessage() == 0) {
        const LONGLONG now = GetNowHiPerformanceCount();
        float dt = static_cast<float>(now - prev) / 1000000.0f;
        prev = now;
        if (dt > 0.05f) dt = 0.05f; // ヒッチ時の暴走防止

        Input::Get().Update();
        ClearDrawScreen();
        if (scene_) {
            scene_->Update(dt);
            scene_->Draw();
            const SceneRequest req = scene_->TakeRequest();
            if (req.next != SceneId::None) ChangeScene(req);
        }
        ScreenFlip();
    }
}

void Game::Shutdown()
{
    if (scene_) scene_->Exit();
    scene_.reset();
    Settings().Save(kConfigPath);
    if (dxInitialized_) {
        SoundManager::Get().ReleaseAll();
        AssetManager::Get().ReleaseAll();
        FontManager::Get().Release();
        DxLib_End();
    }
}

} // namespace ag
