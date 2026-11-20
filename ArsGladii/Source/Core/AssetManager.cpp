#include "AssetManager.h"
#include "KeyValueFile.h"
#include "Config.h"
#include "MathUtil.h"
#include "DxLib.h"
#include <cctype>
#include <cstdlib>

namespace ag {

AssetManager& AssetManager::Get()
{
    static AssetManager instance;
    return instance;
}

const ModelDef* AssetManager::GetModelDef(const std::string& key)
{
    auto it = models_.find(key);
    if (it != models_.end()) return it->second.get();
    if (missingModels_.count(key)) return nullptr;

    auto def = LoadModelDef(key);
    if (!def) {
        missingModels_[key] = true;
        return nullptr;
    }
    ModelDef* raw = def.get();
    models_[key] = std::move(def);
    return raw;
}

int AssetManager::CreateModelInstance(const std::string& key)
{
    const ModelDef* def = GetModelDef(key);
    if (!def) return -1;
    return MV1DuplicateModel(def->baseHandle);
}

int AssetManager::LoadAnimSource(const std::string& dir, const std::string& file)
{
    const std::string path = dir + file;
    auto it = animSources_.find(path);
    if (it != animSources_.end()) return it->second;
    int h = -1;
    if (KeyValueFile::FileExists(path)) h = MV1LoadModel(path.c_str());
    animSources_[path] = h;
    return h;
}

std::unique_ptr<ModelDef> AssetManager::LoadModelDef(const std::string& key)
{
    const std::string dir = "Assets/Models/" + key + "/";
    KeyValueFile cfg;
    if (!cfg.Load(dir + "model.cfg")) return nullptr;

    const std::string file = cfg.GetString("file");
    if (file.empty() || !KeyValueFile::FileExists(dir + file)) return nullptr;

    auto def = std::make_unique<ModelDef>();
    def->key = key;
    def->baseHandle = MV1LoadModel((dir + file).c_str());
    if (def->baseHandle == -1) return nullptr;

    def->scale = cfg.GetFloat("scale", 1.0f);
    def->rotYOffset = DegToRad(cfg.GetFloat("rot_y", 180.0f));
    def->yOffset = cfg.GetFloat("y_offset", 0.0f);
    def->animFps = cfg.GetFloat("anim_fps", 30.0f);

    // 各アニメ: "idle=0" (本体のインデックス) / "idle=Idle.fbx" / "idle=Anims.fbx:2"
    for (int i = 0; i < static_cast<int>(AnimState::Count); ++i) {
        const std::string k = AnimStateKey(static_cast<AnimState>(i));
        AnimClipRef& clip = def->clips[i];
        clip.speed = cfg.GetFloat(k + "_speed", 1.0f);
        const std::string v = cfg.GetString(k);
        if (v.empty()) continue;

        const bool numeric = std::isdigit(static_cast<unsigned char>(v[0])) != 0;
        if (numeric) {
            clip.index = std::atoi(v.c_str());
        } else {
            std::string animFile = v;
            int index = 0;
            size_t colon = v.rfind(':');
            if (colon != std::string::npos) {
                animFile = v.substr(0, colon);
                index = std::atoi(v.substr(colon + 1).c_str());
            }
            int src = LoadAnimSource(dir, animFile);
            if (src != -1) {
                clip.srcHandle = src;
                clip.index = index;
            }
        }
    }
    return def;
}

int AssetManager::GetImage(const std::string& path)
{
    auto it = images_.find(path);
    if (it != images_.end()) return it->second;
    int h = -1;
    if (KeyValueFile::FileExists(path)) h = LoadGraph(path.c_str());
    images_[path] = h;
    return h;
}

void AssetManager::ReleaseAll()
{
    for (auto& m : models_) if (m.second->baseHandle != -1) MV1DeleteModel(m.second->baseHandle);
    for (auto& a : animSources_) if (a.second != -1) MV1DeleteModel(a.second);
    for (auto& g : images_) if (g.second != -1) DeleteGraph(g.second);
    models_.clear();
    missingModels_.clear();
    animSources_.clear();
    images_.clear();
}

// ---------------------------------------------------------------------------

SoundManager& SoundManager::Get()
{
    static SoundManager instance;
    return instance;
}

int SoundManager::Load(const std::string& name)
{
    auto it = sounds_.find(name);
    if (it != sounds_.end()) return it->second;
    int h = -1;
    static const char* exts[] = { ".wav", ".ogg", ".mp3" };
    for (const char* ext : exts) {
        const std::string path = "Assets/Sounds/" + name + ext;
        if (KeyValueFile::FileExists(path)) {
            h = LoadSoundMem(path.c_str());
            break;
        }
    }
    sounds_[name] = h;
    return h;
}

void SoundManager::PlaySE(const std::string& name)
{
    int h = Load(name);
    if (h == -1) return;
    ChangeVolumeSoundMem(Settings().seVolume * 255 / 100, h);
    PlaySoundMem(h, DX_PLAYTYPE_BACK, TRUE);
}

void SoundManager::PlayBGM(const std::string& name)
{
    int h = Load(name);
    if (h == bgmHandle_ && h != -1 && CheckSoundMem(h) == 1) return;
    StopBGM();
    bgmHandle_ = h;
    if (h == -1) return;
    ChangeVolumeSoundMem(Settings().bgmVolume * 255 / 100, h);
    PlaySoundMem(h, DX_PLAYTYPE_LOOP, TRUE);
}

void SoundManager::StopBGM()
{
    if (bgmHandle_ != -1) StopSoundMem(bgmHandle_);
    bgmHandle_ = -1;
}

void SoundManager::ApplyVolume()
{
    if (bgmHandle_ != -1) ChangeVolumeSoundMem(Settings().bgmVolume * 255 / 100, bgmHandle_);
}

void SoundManager::ReleaseAll()
{
    StopBGM();
    for (auto& s : sounds_) if (s.second != -1) DeleteSoundMem(s.second);
    sounds_.clear();
}

} // namespace ag
