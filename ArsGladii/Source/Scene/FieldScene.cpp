#include "FieldScene.h"
#include "../Save/SaveData.h"
#include "../Core/Config.h"
#include "../Core/Input.h"
#include "../Core/FontManager.h"
#include "../Core/AssetManager.h"
#include "../Core/MathUtil.h"
#include "../UI/UIDraw.h"
#include "../Item/ItemDatabase.h"
#include "DxLib.h"

namespace ag {

namespace {
unsigned int ColLog() { return GetColor(230, 230, 230); }
unsigned int ColKill() { return GetColor(255, 200, 150); }
unsigned int ColSystem() { return GetColor(160, 210, 255); }
}

FieldScene::FieldScene(bool continueGame) : continueGame_(continueGame) {}

void FieldScene::Enter()
{
    if (!continueGame_ || !LoadGame()) StartNewGame();
    camera_.Reset(player_.pos, player_.yaw);
    SoundManager::Get().PlayBGM("bgm_field");
}

void FieldScene::Exit()
{
    Input::Get().SetMouseCaptured(false);
    SoundManager::Get().StopBGM();
}

void FieldScene::StartNewGame()
{
    floor_ = 1;
    world_.Load(floor_);
    player_.NewGame(world_.Floor().playerStart);
    log_.Clear();
    log_.Add(world_.Floor().townName + " にようこそ。北のボスエリアに第1層の主がいる。", ColSystem());
    log_.Add("F1 で操作説明の表示を切り替えられます。", ColSystem());
    hud_.ShowBanner(world_.Floor().name, world_.Floor().townName, ui::Gold());
}

bool FieldScene::LoadGame()
{
    int floor = 1;
    if (!SaveSystem::Load(kSavePath, player_, floor)) {
        log_.Add("セーブデータの読み込みに失敗しました", GetColor(255, 120, 120));
        return false;
    }
    floor_ = floor;
    world_.Load(floor_);
    world_.ResolveCollision(player_.pos, player_.radius);
    camera_.Reset(player_.pos, player_.yaw);
    floating_.Clear();
    log_.Add("セーブデータをロードしました", ColSystem());
    hud_.ShowBanner(world_.Floor().name, "ロード完了", ui::Gold());
    return true;
}

void FieldScene::SaveGame()
{
    if (player_.IsDead()) {
        hud_.ShowWarning("戦闘不能中はセーブできません");
        return;
    }
    if (SaveSystem::Save(kSavePath, player_, floor_)) {
        log_.Add("セーブしました", ColSystem());
        hud_.ShowWarning("セーブしました");
    } else {
        log_.Add("セーブに失敗しました", GetColor(255, 120, 120));
    }
}

void FieldScene::HandleMenuCommand(MenuCommand cmd)
{
    switch (cmd) {
    case MenuCommand::Close:   menu_.Close(); break;
    case MenuCommand::Save:    SaveGame(); break;
    case MenuCommand::Load:    menu_.Close(); LoadGame(); break;
    case MenuCommand::ToTitle: menu_.Close(); RequestScene(SceneId::Title); break;
    case MenuCommand::Quit:    menu_.Close(); RequestScene(SceneId::Quit); break;
    default: break;
    }
}

void FieldScene::Update(float dt)
{
    Input& in = Input::Get();

    HandleMenuCommand(pendingCommand_);
    pendingCommand_ = MenuCommand::None;

    const bool modal = result_.IsOpen() || menu_.IsOpen();
    if (result_.IsOpen()) {
        result_.Update(dt);
    } else if (menu_.IsOpen()) {
        if (in.ActionPressed(Action::Menu)) menu_.Close();
    } else {
        if (in.ActionPressed(Action::Menu)) {
            in.SetMouseCaptured(false);
            menu_.Open();
        }
        if (in.ActionPressed(Action::QuickSave)) SaveGame();
        if (in.ActionPressed(Action::QuickLoad)) LoadGame();
    }

    // メニュー・リザルト表示中は時間を止める（シングルプレイのプロトタイプ仕様）
    if (!modal) {
        camera_.HandleInput(true);
        player_.Update(dt, camera_, world_, *this, !player_.IsDead());
        world_.Update(dt, player_, *this);

        // ボスエリアへの出入り
        const bool inArena = world_.GetField().InBossArena(player_.pos);
        if (inArena && !wasInBossArena_) log_.Add("ボスエリアに入った", GetColor(255, 140, 120));
        wasInBossArena_ = inArena;

        // 戦闘不能 → 町で復活
        if (player_.IsDead()) {
            if (deathTimer_ == 0.0f) {
                log_.Add("力尽きた……", GetColor(255, 90, 90));
                SoundManager::Get().PlaySE("player_death");
            }
            deathTimer_ += dt;
            if (deathTimer_ > 4.0f) {
                deathTimer_ = 0.0f;
                player_.Respawn(world_.Floor().playerStart);
                camera_.Reset(player_.pos, 0.0f);
                log_.Add(world_.Floor().townName + " で目を覚ました", ColSystem());
            }
        }
    } else {
        camera_.HandleInput(false);
    }

    ProcessEvents();
    log_.Update(dt);
    floating_.Update(dt);
    hud_.Update(dt, player_, world_);
    camera_.Update(player_.pos, dt);
}

void FieldScene::GrantRewards(const GameEvent& e, int& levelsGained)
{
    player_.gold += e.gold;
    levelsGained = player_.AddExp(e.exp);
    for (const auto& item : e.drops) {
        const ItemDef* d = item.Def();
        if (!d) continue;
        const int left = player_.inventory.Add(item);
        std::string name = "[" + d->name + "]";
        if (d->IsEquipment()) name += Format(" 個体値%c", item.IvRank());
        if (left > 0) log_.Add(name + " は所持品がいっぱいで拾えなかった", GetColor(255, 120, 120));
        else log_.Add(name + Format(" x%d を入手", item.count), RarityColor(d->rarity));
    }
    if (levelsGained > 0) {
        log_.Add(Format("レベルアップ！ Lv%d になった", player_.level), GetColor(255, 240, 120));
        hud_.ShowBanner("LEVEL UP", Format("Lv %d", player_.level), GetColor(255, 230, 120));
        SoundManager::Get().PlaySE("level_up");
    }
}

void FieldScene::ProcessEvents()
{
    // 処理中に新たなイベントが積まれても安全なように入れ替える
    std::vector<GameEvent> list;
    list.swap(events_);
    const bool numbers = Settings().showDamageNumbers;
    for (const auto& e : list) {
        switch (e.type) {
        case GameEvent::Type::DamageToEnemy:
            if (numbers) floating_.Spawn(e.pos, e.critical ? Format("%d!", e.amount) : Format("%d", e.amount),
                                         e.critical ? GetColor(255, 220, 60) : GetColor(255, 255, 255), e.critical);
            break;
        case GameEvent::Type::DamageToPlayer:
            if (numbers) floating_.Spawn(e.pos, Format("%d", e.amount), GetColor(255, 80, 80), e.critical);
            camera_.AddShake(e.amount >= player_.stats.maxHP / 6 ? 14.0f : 6.0f, 0.25f);
            break;
        case GameEvent::Type::PlayerHealed:
            floating_.Spawn(e.pos, Format("+%d", e.amount), GetColor(120, 255, 140), false);
            break;
        case GameEvent::Type::EnemyKilled: {
            log_.Add(Format("%s を倒した！  EXP +%d  %d G", e.enemy->name.c_str(), e.exp, e.gold), ColKill());
            int lv = 0;
            GrantRewards(e, lv);
            break;
        }
        case GameEvent::Type::BossKilled: {
            const std::string bossName = e.enemy->title.empty() ? e.enemy->name : e.enemy->title + " " + e.enemy->name;
            log_.Add(bossName + " を討伐した！", GetColor(255, 210, 90));
            int lv = 0;
            GrantRewards(e, lv);
            Input::Get().SetMouseCaptured(false);
            result_.Open(bossName, e.fightTime, e.exp, e.gold, lv, e.drops);
            SoundManager::Get().PlayBGM("bgm_field");
            break;
        }
        case GameEvent::Type::BossEngaged:
            hud_.ShowBanner("BOSS BATTLE", e.enemy->title + "  " + e.enemy->name, GetColor(255, 90, 70));
            SoundManager::Get().PlayBGM("bgm_boss");
            break;
        case GameEvent::Type::BossEnraged:
            hud_.ShowWarning(e.text);
            log_.Add(e.text, GetColor(255, 100, 150));
            camera_.AddShake(10.0f, 0.5f);
            break;
        case GameEvent::Type::PlayerDied:
            break;
        case GameEvent::Type::Info:
            log_.Add(e.text, ColLog());
            break;
        case GameEvent::Type::Warning:
            hud_.ShowWarning(e.text);
            break;
        }
    }
}

void FieldScene::SetupRenderState() const
{
    const unsigned int sky = world_.Floor().skyColor;
    SetBackgroundColor((sky >> 16) & 0xFF, (sky >> 8) & 0xFF, sky & 0xFF);
    SetUseZBuffer3D(TRUE);
    SetWriteZBuffer3D(TRUE);
    SetUseLighting(TRUE);
    SetLightDirection(VGet(-0.4f, -1.0f, 0.5f));
    SetGlobalAmbientLight(GetColorF(0.35f, 0.35f, 0.40f, 1.0f));
    SetFogEnable(TRUE);
    SetFogColor((sky >> 16) & 0xFF, (sky >> 8) & 0xFF, sky & 0xFF);
    SetFogStartEnd(6000.0f, 16000.0f);
}

void FieldScene::Draw()
{
    SetupRenderState();
    camera_.Apply();

    world_.Draw(camera_.Position());
    player_.Draw();

    SetFogEnable(FALSE);
    // ---- 2D ----
    world_.DrawOverlays(camera_.Position());
    floating_.Draw();
    hud_.Draw(player_, world_, camera_);
    log_.Draw();

    if (player_.IsDead()) {
        ui::DimScreen(static_cast<int>(Clamp(deathTimer_ / 2.0f, 0.0f, 1.0f) * 160));
        FontManager::Get().DrawCentered(kScreenWidth / 2, kScreenHeight / 2 - 40, "力尽きた……", GetColor(255, 110, 110), FontId::Large, GetColor(0, 0, 0));
        FontManager::Get().DrawCentered(kScreenWidth / 2, kScreenHeight / 2 + 20, "まもなく町で復活します", ui::TextColor(), FontId::Normal, GetColor(0, 0, 0));
    }

    result_.Draw();
    const MenuCommand cmd = menu_.Run(player_, *this);
    if (cmd != MenuCommand::None) pendingCommand_ = cmd;
}

} // namespace ag
