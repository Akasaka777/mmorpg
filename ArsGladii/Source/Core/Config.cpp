#include "Config.h"
#include "KeyValueFile.h"

namespace ag {

GameSettings& Settings()
{
    static GameSettings settings;
    return settings;
}

bool GameSettings::Load(const char* path)
{
    KeyValueFile kv;
    if (!kv.Load(path)) return false;
    mouseSensitivity  = kv.GetFloat("mouse_sensitivity", mouseSensitivity);
    invertY           = kv.GetBool("invert_y", invertY);
    cameraDistance    = kv.GetFloat("camera_distance", cameraDistance);
    bgmVolume         = kv.GetInt("bgm_volume", bgmVolume);
    seVolume          = kv.GetInt("se_volume", seVolume);
    showDamageNumbers = kv.GetBool("show_damage_numbers", showDamageNumbers);
    fullscreen        = kv.GetBool("fullscreen", fullscreen);
    return true;
}

bool GameSettings::Save(const char* path) const
{
    KeyValueFile kv;
    kv.SetFloat("mouse_sensitivity", mouseSensitivity);
    kv.SetBool("invert_y", invertY);
    kv.SetFloat("camera_distance", cameraDistance);
    kv.SetInt("bgm_volume", bgmVolume);
    kv.SetInt("se_volume", seVolume);
    kv.SetBool("show_damage_numbers", showDamageNumbers);
    kv.SetBool("fullscreen", fullscreen);
    return kv.Save(path);
}

} // namespace ag
