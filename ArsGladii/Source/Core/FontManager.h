#pragma once
// 日本語フォント管理。
// Assets/Fonts/font.cfg に `file=xxx.ttf` と `name=フォント名` を書くと、そのフォントを使う。
// 未指定・読み込み失敗時は Windows 標準の「メイリオ」→「ＭＳ ゴシック」の順で使う。
#include <string>

namespace ag {

enum class FontId {
    Small,   // 18px ログ・補足
    Normal,  // 22px 標準
    Medium,  // 28px 見出し
    Large,   // 40px リザルト見出し
    Title,   // 96px タイトルロゴ
    Count
};

class FontManager {
public:
    static FontManager& Get();

    void Init();
    void Release();

    int Handle(FontId id) const { return handles_[static_cast<int>(id)]; }
    int Size(FontId id) const;

    // 描画ヘルパー（文字列は UTF-8）
    void Draw(int x, int y, const std::string& text, unsigned int color, FontId id = FontId::Normal, unsigned int edge = 0) const;
    void DrawCentered(int cx, int y, const std::string& text, unsigned int color, FontId id = FontId::Normal, unsigned int edge = 0) const;
    void DrawRight(int rightX, int y, const std::string& text, unsigned int color, FontId id = FontId::Normal, unsigned int edge = 0) const;
    int  Width(const std::string& text, FontId id = FontId::Normal) const;

    const std::string& FaceName() const { return faceName_; }

private:
    FontManager() = default;
    int handles_[static_cast<int>(FontId::Count)] = { -1, -1, -1, -1, -1 };
    std::string faceName_;
    std::string loadedFontFile_;
};

// よく使う書式化ヘルパー
std::string Format(const char* fmt, ...);

} // namespace ag
