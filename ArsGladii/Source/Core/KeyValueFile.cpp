#include "KeyValueFile.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>

namespace ag {

namespace {
std::string Trim(const std::string& s)
{
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}
} // namespace

bool KeyValueFile::FileExists(const std::string& path)
{
    std::error_code ec;
    return std::filesystem::is_regular_file(path, ec);
}

std::vector<std::string> KeyValueFile::Split(const std::string& s, char delim)
{
    std::vector<std::string> out;
    std::string cur;
    std::istringstream ss(s);
    while (std::getline(ss, cur, delim)) out.push_back(Trim(cur));
    return out;
}

bool KeyValueFile::Load(const std::string& path)
{
    std::ifstream ifs(path);
    if (!ifs) return false;
    entries_.clear();
    std::string line;
    while (std::getline(ifs, line)) {
        // UTF-8 BOM を除去
        if (line.size() >= 3 && (unsigned char)line[0] == 0xEF && (unsigned char)line[1] == 0xBB && (unsigned char)line[2] == 0xBF)
            line = line.substr(3);
        line = Trim(line);
        if (line.empty() || line[0] == '#') continue;
        // 行末コメント（空白 + #）を除去
        for (size_t i = 1; i < line.size(); ++i) {
            if (line[i] == '#' && (line[i - 1] == ' ' || line[i - 1] == '\t')) {
                line = Trim(line.substr(0, i));
                break;
            }
        }
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        entries_.emplace_back(Trim(line.substr(0, eq)), Trim(line.substr(eq + 1)));
    }
    return true;
}

bool KeyValueFile::Save(const std::string& path) const
{
    std::error_code ec;
    std::filesystem::path p(path);
    if (p.has_parent_path()) std::filesystem::create_directories(p.parent_path(), ec);
    std::ofstream ofs(path, std::ios::trunc);
    if (!ofs) return false;
    for (const auto& e : entries_) ofs << e.first << '=' << e.second << '\n';
    return static_cast<bool>(ofs);
}

const std::string* KeyValueFile::Find(const std::string& key) const
{
    for (const auto& e : entries_) if (e.first == key) return &e.second;
    return nullptr;
}

bool KeyValueFile::Has(const std::string& key) const { return Find(key) != nullptr; }

std::string KeyValueFile::GetString(const std::string& key, const std::string& def) const
{
    const std::string* v = Find(key);
    return v ? *v : def;
}

int KeyValueFile::GetInt(const std::string& key, int def) const
{
    const std::string* v = Find(key);
    return v ? std::atoi(v->c_str()) : def;
}

float KeyValueFile::GetFloat(const std::string& key, float def) const
{
    const std::string* v = Find(key);
    return v ? static_cast<float>(std::atof(v->c_str())) : def;
}

bool KeyValueFile::GetBool(const std::string& key, bool def) const
{
    const std::string* v = Find(key);
    if (!v) return def;
    return *v == "1" || *v == "true" || *v == "TRUE" || *v == "yes";
}

void KeyValueFile::SetString(const std::string& key, const std::string& value)
{
    for (auto& e : entries_) if (e.first == key) { e.second = value; return; }
    entries_.emplace_back(key, value);
}

void KeyValueFile::SetInt(const std::string& key, int value) { SetString(key, std::to_string(value)); }
void KeyValueFile::SetFloat(const std::string& key, float value)
{
    std::ostringstream ss;
    ss << value;
    SetString(key, ss.str());
}
void KeyValueFile::SetBool(const std::string& key, bool value) { SetString(key, value ? "1" : "0"); }

} // namespace ag
