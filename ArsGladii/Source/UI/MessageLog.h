#pragma once
// 画面左下のログ（撃破・ドロップ品・システムメッセージ）
#include <string>
#include <deque>

namespace ag {

class MessageLog {
public:
    void Add(const std::string& text, unsigned int color);
    void Update(float dt);
    void Draw() const;
    void Clear() { lines_.clear(); }

private:
    struct Line {
        std::string text;
        unsigned int color;
        float age;
    };
    std::deque<Line> lines_;
    static constexpr int kMaxLines = 10;
    static constexpr float kLifetime = 12.0f;
};

} // namespace ag
