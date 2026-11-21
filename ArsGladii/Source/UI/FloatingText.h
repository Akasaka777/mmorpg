#pragma once
// ダメージ数値などのワールド座標に浮かぶテキスト
#include "DxLib.h"
#include <string>
#include <vector>

namespace ag {

class FloatingTextSystem {
public:
    void Spawn(const VECTOR& worldPos, const std::string& text, unsigned int color, bool big);
    void Update(float dt);
    void Draw() const;
    void Clear() { items_.clear(); }

private:
    struct Item {
        VECTOR pos;
        std::string text;
        unsigned int color;
        bool big;
        float age;
        float offsetX;
    };
    std::vector<Item> items_;
};

} // namespace ag
