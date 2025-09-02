#include "pch.h"
#include "Config.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <windows.h>

static std::wstring ExpandEnvVars(const std::wstring& in) {
    DWORD need = ExpandEnvironmentStringsW(in.c_str(), nullptr, 0);
    if (need == 0) return in;
    std::wstring out; out.resize(need);
    DWORD wrote = ExpandEnvironmentStringsW(in.c_str(), out.data(), need);
    if (wrote > 0 && !out.empty() && out.back() == L'\0') out.pop_back();
    return out;
}
static std::wstring WFromUtf8(const std::string& s) {
    if (s.empty()) return std::wstring();
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w; w.resize(n);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), w.data(), n);
    return w;
}

EditorConfig::EditorConfig() {}
EditorConfig::~EditorConfig() {}

std::optional<EditorConfig> EditorConfig::LoadFromFile(const std::wstring& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) return std::nullopt;

    nlohmann::json j; ifs >> j;

    EditorConfig ec;
    ec.m_binDir = ExpandEnvVars(WFromUtf8(j["ffmpeg"]["binDir"].get<std::string>()));
    ec.m_ffmpegExe = WFromUtf8(j["ffmpeg"]["ffmpegExe"].get<std::string>());
    ec.m_ffprobeExe = WFromUtf8(j["ffmpeg"]["ffprobeExe"].get<std::string>());

    for (auto& a : j["ffmpeg"]["defaultArgs"])
        ec.m_defaultArgs.emplace_back(WFromUtf8(a.get<std::string>()));

    if (j.contains("temp") && j["temp"].contains("dir"))
        ec.m_tempDir = ExpandEnvVars(WFromUtf8(j["temp"]["dir"].get<std::string>()));

    if (j.contains("renderProfiles")) {
        for (auto it = j["renderProfiles"].begin(); it != j["renderProfiles"].end(); ++it) {
            RenderProfile p;
            p.container = it.value().value("container", "");
            p.videoCodec = it.value().value("videoCodec", "");
            p.audioCodec = it.value().value("audioCodec", "");
            p.pixFmt = it.value().value("pixFmt", "");
            p.videoBitrate = it.value().value("videoBitrate", "");
            p.audioBitrate = it.value().value("audioBitrate", "");
            p.preset = it.value().value("preset", "");

            auto get_int = [](const nlohmann::json& node, const char* key, int defVal)->int {
                if (!node.contains(key)) return defVal;
                const auto& v = node.at(key);
                if (v.is_number_integer() || v.is_number_unsigned()) return v.get<int>();
                if (v.is_string()) { try { return std::stoi(v.get<std::string>()); } catch (...) {} }
                return defVal;
                };

            p.crf = get_int(it.value(), "crf", 20);
            p.audioChannels = get_int(it.value(), "audioChannels", 2);
            p.audioRate = get_int(it.value(), "audioRate", 48000);

            ec.m_profiles.emplace(it.key(), p);
        }
    }
    return std::optional<EditorConfig>(ec);
}

std::wstring EditorConfig::FfmpegExePath() const { return m_binDir + L"\\" + m_ffmpegExe; }
std::wstring EditorConfig::FfprobeExePath() const { return m_binDir + L"\\" + m_ffprobeExe; }
const std::vector<std::wstring>& EditorConfig::DefaultArgs() const { return m_defaultArgs; }
const RenderProfile* EditorConfig::GetProfile(const std::string& name) const {
    auto it = m_profiles.find(name);
    if (it == m_profiles.end()) return nullptr;
    return &it->second;
}
