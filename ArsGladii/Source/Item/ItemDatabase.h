#pragma once
// アイテムのマスターデータ。プロトタイプではコード内に定義している。
// 層が増えたら CSV / JSON からの読み込みに置き換える想定（Find の呼び出し側は変更不要）。
#include "Item.h"
#include <vector>

namespace ag {

class ItemDatabase {
public:
    static ItemDatabase& Get();
    const ItemDef* Find(const std::string& id) const;
    const std::vector<ItemDef>& All() const { return items_; }

private:
    ItemDatabase();
    void Add(const ItemDef& def) { items_.push_back(def); }
    std::vector<ItemDef> items_;
};

} // namespace ag
