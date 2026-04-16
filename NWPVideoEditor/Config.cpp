#include "pch.h"
#include "Config.h"
#include <windows.h>

EditorConfig::EditorConfig() : m_isConfigured(false) {}

void EditorConfig::SetFFmpegFolder(const std::wstring& folderPath) {
    if (folderPath.empty()) {
        m_ffmpegPath.clear();
        m_ffprobePath.clear();
        m_folderPath.clear();
        m_isConfigured = false;
        return;
    }
    m_folderPath = folderPath;
    m_ffmpegPath = folderPath + L"\\ffmpeg.exe";
    m_ffprobePath = folderPath + L"\\ffprobe.exe";
    UpdatePaths();
}

void EditorConfig::UpdatePaths() {
    m_isConfigured = PathFileExistsW(m_ffmpegPath.c_str()) &&
        PathFileExistsW(m_ffprobePath.c_str());
}