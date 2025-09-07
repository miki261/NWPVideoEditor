#include "pch.h"
#include "Config.h"
#include <windows.h>
#include <fstream>

static std::wstring ExpandEnvVars(const std::wstring& in) {
    DWORD need = ExpandEnvironmentStringsW(in.c_str(), nullptr, 0);
    if (need == 0) return in;
    std::wstring out; out.resize(need);
    DWORD wrote = ExpandEnvironmentStringsW(in.c_str(), out.data(), need);
    if (wrote > 0 && !out.empty() && out.back() == L'\0') out.pop_back();
    return out;
}

EditorConfig::EditorConfig() {}
EditorConfig::~EditorConfig() {}

bool EditorConfig::LoadFromFile(const std::wstring& path, EditorConfig& config) {
    config.m_binDir = ExpandEnvVars(L"%ProgramFiles%\\ffmpeg\\bin");
    config.m_ffmpegExe = L"ffmpeg.exe";
    config.m_ffprobeExe = L"ffprobe.exe";
    config.m_tempDir = ExpandEnvVars(L"%TEMP%");

    config.m_defaultArgs.clear();
    config.m_defaultArgs.push_back(L"-y");
    config.m_defaultArgs.push_back(L"-hide_banner");

    // Add default profile
    RenderProfile defaultProfile;
    defaultProfile.container = "mp4";
    defaultProfile.videoCodec = "libx264";
    defaultProfile.audioCodec = "aac";
    defaultProfile.pixFmt = "yuv420p";
    defaultProfile.videoBitrate = "2000k";
    defaultProfile.audioBitrate = "128k";
    defaultProfile.preset = "medium";
    defaultProfile.crf = 23;
    defaultProfile.audioChannels = 2;
    defaultProfile.audioRate = 44100;

    config.m_profiles["default"] = defaultProfile;
    return true;
}

std::wstring EditorConfig::FfmpegExePath() const { return m_binDir + L"\\" + m_ffmpegExe; }
std::wstring EditorConfig::FfprobeExePath() const { return m_binDir + L"\\" + m_ffprobeExe; }
const std::vector<std::wstring>& EditorConfig::DefaultArgs() const { return m_defaultArgs; }
const RenderProfile* EditorConfig::GetProfile(const std::string& name) const {
    auto it = m_profiles.find(name);
    if (it == m_profiles.end()) return nullptr;
    return &it->second;
}
