#pragma once
#include <string>

class EditorConfig {
private:
    std::wstring m_ffmpegPath;
    std::wstring m_ffprobePath;
    std::wstring m_folderPath;
    std::wstring m_ffplayPath;
    bool m_isConfigured;

public:
    EditorConfig();
    void SetFFmpegFolder(const std::wstring& folderPath);
    bool IsFFmpegConfigured() const { return m_isConfigured; }
    const std::wstring& GetFFmpegExePath() const { return m_ffmpegPath; }
    const std::wstring& GetFFprobeExePath() const { return m_ffprobePath; }
    const std::wstring& GetFolderPath() const { return m_folderPath; }
    const std::wstring& GetFFplayExePath() const { return m_ffplayPath; }

private:
    void UpdatePaths();
};