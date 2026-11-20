#pragma once
// ESC メニュー: ステータス / 装備・所持品 / スキル選択 / ゲーム設定 / システム（セーブ・ロード）
#include "../World/GameEvent.h"
#include "../Item/Item.h"
#include <string>

namespace ag {

class Player;

enum class MenuCommand { None, Close, Save, Load, ToTitle, Quit };

class Menu {
public:
    enum class Tab { Status, Equipment, Skills, Settings, System, Count };

    void Open();
    void Close();
    bool IsOpen() const { return open_; }

    // 入力処理と描画を同時に行う（即時モード UI）
    MenuCommand Run(Player& player, EventSink& events);

private:
    void DrawStatusTab(Player& player, int x, int y, int w, int h);
    void DrawEquipmentTab(Player& player, EventSink& events, int x, int y, int w, int h);
    void DrawSkillTab(Player& player, int x, int y, int w, int h);
    void DrawSettingsTab(int x, int y, int w, int h);
    MenuCommand DrawSystemTab(int x, int y, int w, int h);
    void DrawItemDetail(Player& player, EventSink& events, const ItemInstance& item, int invIndex, EquipSlot equippedSlot, int x, int y, int w, int h);

    bool open_ = false;
    Tab tab_ = Tab::Equipment;
    int selectedInv_ = -1;
    EquipSlot selectedSlot_ = EquipSlot::None;
    int scroll_ = 0;
    int filter_ = 0; // 0:全て 1:装備 2:消費 3:素材
    std::string selectedSkill_;
    bool settingsDirty_ = false;
    bool confirmQuit_ = false;
};

} // namespace ag
