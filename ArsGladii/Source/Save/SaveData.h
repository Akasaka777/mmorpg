#pragma once
// セーブ / ロード（テキスト形式 key=value。デバッグしやすく、項目追加にも強い）
// 将来オンライン化する場合は、この入出力をサーバー API に差し替える。
#include <string>

namespace ag {

class Player;

struct SaveHeader {
    bool exists = false;
    int level = 0;
    int floor = 1;
    std::string savedAt;
};

class SaveSystem {
public:
    static bool Save(const std::string& path, const Player& player, int floor);
    static bool Load(const std::string& path, Player& player, int& floor);
    static SaveHeader Peek(const std::string& path);
};

} // namespace ag
