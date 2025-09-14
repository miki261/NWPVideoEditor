#include "pch.h"
#include "Config.h"
#include <windows.h>
#include <fstream>
#include <shlobj.h>

static std::wstring ExpandEnvVars(const std::wstring& in) {
    DWORD need = ExpandEnvironmentStringsW(in.c_str(), nullptr, 0);
    if (need == 0) return in;
    std::wstring out;
    out.resize(need);
    DWORD wrote = ExpandEnvironmentStringsW(in.c_str(), out.data(), need);
    if (wrote > 0 && !out.empty() && out.back() == L'\0') out.pop_back();
    return out;
}

EditorConfig::EditorConfig() {
    std::wstring projectPath = L"C:\\Users\\MM\\source\\repos\\NWPVideoEditor\\ffmpeg";
    if (PathFileExistsW((projectPath + L"\\ffmpeg.exe").c_str()) &&
        PathFileExistsW((projectPath + L"\\ffprobe.exe").c_str())) {
        SetFFmpegFolder(projectPath);
    }
}

EditorConfig::~EditorConfig() {}

void EditorConfig::SetFFmpegFolder(const std::wstring& folderPath) {
    m_ffmpegFolder = folderPath;
    UpdateExecutablePaths();
}

void EditorConfig::UpdateExecutablePaths() {
    if (m_ffmpegFolder.empty()) {
        m_ffmpegPath.clear();
        m_ffprobePath.clear();
    }
    else {
        m_ffmpegPath = m_ffmpegFolder + L"\\ffmpeg.exe";
        m_ffprobePath = m_ffmpegFolder + L"\\ffprobe.exe";
    }
}

bool EditorConfig::IsFFmpegConfigured() const {
    return !m_ffmpegPath.empty() && !m_ffprobePath.empty() &&
        PathFileExistsW(m_ffmpegPath.c_str()) &&
        PathFileExistsW(m_ffprobePath.c_str());
}

bool EditorConfig::LoadFromFile(const std::wstring& path, EditorConfig& config) {
    config.m_tempDir.clear();

    config.m_defaultArgs.clear();
    config.m_defaultArgs.push_back(L"-y");
    config.m_defaultArgs.push_back(L"-hide_banner");

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

const std::vector<std::wstring>& EditorConfig::DefaultArgs() const {
    return m_defaultArgs;
}

const RenderProfile* EditorConfig::GetProfile(const std::string& name) const {
    auto it = m_profiles.find(name);
    if (it == m_profiles.end()) return nullptr;
    return &it->second;
}
