#pragma once
// ボス撃破時に画面中央へ表示するリザルト
#include "../Item/Item.h"
#include <string>
#include <vector>

namespace ag {

class ResultScreen {
public:
    void Open(const std::string& bossName, float fightTime, int exp, int gold, int levelsGained,
              const std::vector<ItemInstance>& drops);
    void Update(float dt);
    void Draw() const;
    bool IsOpen() const { return open_; }

private:
    bool open_ = false;
    float time_ = 0.0f;
    std::string bossName_;
    float fightTime_ = 0.0f;
    int exp_ = 0, gold_ = 0, levels_ = 0;
    std::vector<ItemInstance> drops_;
    mutable bool closeRequested_ = false;
};

// 装備の性能を1行で表す（インベントリ・リザルト共通）
std::string ItemStatSummary(const ItemInstance& item);

} // namespace ag
