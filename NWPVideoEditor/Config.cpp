#include "pch.h"
#include "Config.h"
#include <windows.h>

EditorConfig::EditorConfig() {
    // Start empty - user must configure via button
    m_isConfigured = false;
    m_ffmpegPath.clear();
    m_ffprobePath.clear();
}

void EditorConfig::SetFFmpegFolder(const std::wstring& folderPath) {
    if (folderPath.empty()) {
        m_ffmpegPath.clear();
        m_ffprobePath.clear();
        m_isConfigured = false;
        return;
    }

    // Create full paths
    m_ffmpegPath = folderPath + L"\\ffmpeg.exe";
    m_ffprobePath = folderPath + L"\\ffprobe.exe";

    UpdatePaths();
}

void EditorConfig::UpdatePaths() {
    // Check if files actually exist
    m_isConfigured = PathFileExistsW(m_ffmpegPath.c_str()) &&
        PathFileExistsW(m_ffprobePath.c_str());
}
