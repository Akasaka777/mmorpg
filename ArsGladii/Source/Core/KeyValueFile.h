#pragma once
// "key=value" 形式のテキストファイルの読み書き。
// 設定ファイル・セーブデータ・モデル定義(model.cfg)で共通利用する。
// '#' で始まる行はコメント。
#include <string>
#include <vector>
#include <utility>

namespace ag {

class KeyValueFile {
public:
    bool Load(const std::string& path);
    bool Save(const std::string& path) const;

    bool Has(const std::string& key) const;
    std::string GetString(const std::string& key, const std::string& def = "") const;
    int   GetInt(const std::string& key, int def = 0) const;
    float GetFloat(const std::string& key, float def = 0.0f) const;
    bool  GetBool(const std::string& key, bool def = false) const;

    void SetString(const std::string& key, const std::string& value);
    void SetInt(const std::string& key, int value);
    void SetFloat(const std::string& key, float value);
    void SetBool(const std::string& key, bool value);

    const std::vector<std::pair<std::string, std::string>>& Entries() const { return entries_; }

    static bool FileExists(const std::string& path);
    static std::vector<std::string> Split(const std::string& s, char delim);

private:
    const std::string* Find(const std::string& key) const;
    std::vector<std::pair<std::string, std::string>> entries_;
};

} // namespace ag
