#pragma once
// ゲーム内イベント。World やキャラクターが発行し、FieldScene が UI（ログ・ダメージ表示・リザルト）へ反映する。
// 将来マルチプレイを実装する際は、このイベントをネットワーク同期の単位として使える。
#include "../Item/Item.h"
#include "DxLib.h"
#include <string>
#include <vector>

namespace ag {

struct EnemyDef;

struct GameEvent {
    enum class Type {
        DamageToEnemy,
        DamageToPlayer,
        PlayerHealed,
        EnemyKilled,
        BossKilled,
        BossEngaged,
        BossEnraged,
        PlayerDied,
        Parried,     // パリィ成功
        Info,        // システムメッセージ（ログに表示）
        Warning,     // 画面中央に短く表示（MP不足など）
    };

    Type type = Type::Info;
    VECTOR pos = VGet(0, 0, 0);
    int amount = 0;
    bool critical = false;
    std::string text;
    const EnemyDef* enemy = nullptr;
    std::vector<ItemInstance> drops;
    int exp = 0;
    int gold = 0;
    float fightTime = 0.0f;
};

class EventSink {
public:
    virtual ~EventSink() = default;
    virtual void Push(const GameEvent& e) = 0;

    void Info(const std::string& text)
    {
        GameEvent e;
        e.type = GameEvent::Type::Info;
        e.text = text;
        Push(e);
    }
    void Warning(const std::string& text)
    {
        GameEvent e;
        e.type = GameEvent::Type::Warning;
        e.text = text;
        Push(e);
    }
};

} // namespace ag
