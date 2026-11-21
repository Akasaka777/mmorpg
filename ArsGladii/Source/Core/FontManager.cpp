#include "FontManager.h"
#include "KeyValueFile.h"
#include "DxLib.h"
#include <cstdarg>
#include <cstdio>
#include <vector>

namespace ag {

namespace {
constexpr int kSizes[] = { 18, 22, 28, 40, 96 };
constexpr int kThick[] = { 4, 5, 6, 7, 9 };
}

FontManager& FontManager::Get()
{
    static FontManager instance;
    return instance;
}

int FontManager::Size(FontId id) const { return kSizes[static_cast<int>(id)]; }

void FontManager::Init()
{
    // 1) Assets/Fonts/font.cfg で指定された同梱フォントを登録
    KeyValueFile cfg;
    std::string wantedFace;
    if (cfg.Load("Assets/Fonts/font.cfg")) {
        std::string file = cfg.GetString("file");
        wantedFace = cfg.GetString("name");
        if (!file.empty()) {
            std::string path = "Assets/Fonts/" + file;
            if (KeyValueFile::FileExists(path) && AddFontResourceExA(path.c_str(), FR_PRIVATE, nullptr) > 0) {
                loadedFontFile_ = path;
            } else {
                wantedFace.clear();
            }
        }
    }

    // 2) 候補を順に試す（最初に作成できたものを採用）
    std::vector<std::string> candidates;
    if (!wantedFace.empty()) candidates.push_back(wantedFace);
    candidates.push_back("メイリオ");
    candidates.push_back("Meiryo");
    candidates.push_back("ＭＳ ゴシック");

    for (const auto& face : candidates) {
        int test = CreateFontToHandle(face.c_str(), kSizes[1], kThick[1], DX_FONTTYPE_ANTIALIASING_EDGE_8X8, -1, 2);
        if (test != -1) {
            DeleteFontToHandle(test);
            faceName_ = face;
            break;
        }
    }

    for (int i = 0; i < static_cast<int>(FontId::Count); ++i) {
        const int edge = (i >= static_cast<int>(FontId::Large)) ? 3 : 2;
        handles_[i] = CreateFontToHandle(faceName_.empty() ? nullptr : faceName_.c_str(),
                                         kSizes[i], kThick[i], DX_FONTTYPE_ANTIALIASING_EDGE_8X8, -1, edge);
    }
}

void FontManager::Release()
{
    for (int& h : handles_) {
        if (h != -1) DeleteFontToHandle(h);
        h = -1;
    }
    if (!loadedFontFile_.empty()) {
        RemoveFontResourceExA(loadedFontFile_.c_str(), FR_PRIVATE, nullptr);
        loadedFontFile_.clear();
    }
}

void FontManager::Draw(int x, int y, const std::string& text, unsigned int color, FontId id, unsigned int edge) const
{
    DrawStringToHandle(x, y, text.c_str(), color, Handle(id), edge);
}

void FontManager::DrawCentered(int cx, int y, const std::string& text, unsigned int color, FontId id, unsigned int edge) const
{
    Draw(cx - Width(text, id) / 2, y, text, color, id, edge);
}

void FontManager::DrawRight(int rightX, int y, const std::string& text, unsigned int color, FontId id, unsigned int edge) const
{
    Draw(rightX - Width(text, id), y, text, color, id, edge);
}

int FontManager::Width(const std::string& text, FontId id) const
{
    return GetDrawStringWidthToHandle(text.c_str(), static_cast<int>(text.size()), Handle(id));
}

std::string Format(const char* fmt, ...)
{
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    return std::string(buf);
}

} // namespace ag
