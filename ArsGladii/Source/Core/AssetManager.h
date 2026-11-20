#pragma once
// 外部アセット（3Dモデル・画像・サウンド）の読み込み管理。
//
// ■ 3Dモデルの差し替え方法
//   Assets/Models/<キー>/model.cfg を置くと、そのモデルが使われる。
//   キー例: player, wolf, goblin, boss_floor1, weapon_one_hand_sword ...
//   model.cfg が無い／読み込みに失敗した場合は、プリミティブ（球・カプセル）の
//   サンプルモデルで描画されるので、素材が揃っていなくてもゲームは動く。
//
//   DxLib の MV1LoadModel が読める形式（.mv1 / .x / .pmx / .pmd / .mqo / .fbx）に対応。
//   アニメーションはモデル内蔵のインデックス、または別ファイル（Mixamo 等）を指定できる。
//   書式は Assets/Models/README.md を参照。
#include "../Entity/AnimState.h"
#include <string>
#include <unordered_map>
#include <memory>

namespace ag {

struct AnimClipRef {
    int srcHandle = -1;   // -1: モデル本体のアニメ / それ以外: 別ファイルのモデルハンドル
    int index = -1;       // -1: 未設定
    float speed = 1.0f;   // 再生速度倍率
    bool Valid() const { return index >= 0; }
};

struct ModelDef {
    std::string key;
    int baseHandle = -1;
    float scale = 1.0f;
    float rotYOffset = 3.14159265f; // DxLib のモデルは通常 -Z 向きなので 180 度回す
    float yOffset = 0.0f;
    float animFps = 30.0f;          // アニメ時間の単位（FBX は通常 30）
    AnimClipRef clips[static_cast<int>(AnimState::Count)];
};

class AssetManager {
public:
    static AssetManager& Get();

    // モデル定義を取得（無ければ nullptr → プリミティブ描画にフォールバック）
    const ModelDef* GetModelDef(const std::string& key);
    // 個体ごとにアニメ状態を持てるよう、複製したハンドルを返す（呼び出し側で MV1DeleteModel）
    int CreateModelInstance(const std::string& key);

    // 画像（無ければ -1）
    int GetImage(const std::string& path);

    void ReleaseAll();

private:
    AssetManager() = default;
    std::unique_ptr<ModelDef> LoadModelDef(const std::string& key);
    int LoadAnimSource(const std::string& dir, const std::string& file);

    std::unordered_map<std::string, std::unique_ptr<ModelDef>> models_;
    std::unordered_map<std::string, bool> missingModels_;
    std::unordered_map<std::string, int> animSources_;
    std::unordered_map<std::string, int> images_;
};

// 効果音・BGM（Assets/Sounds/<名前>.wav|.ogg|.mp3 があれば再生、無ければ無音）
class SoundManager {
public:
    static SoundManager& Get();
    void PlaySE(const std::string& name);
    void PlayBGM(const std::string& name);
    void StopBGM();
    void ApplyVolume();
    void ReleaseAll();

private:
    SoundManager() = default;
    int Load(const std::string& name);
    std::unordered_map<std::string, int> sounds_;
    int bgmHandle_ = -1;
};

} // namespace ag
