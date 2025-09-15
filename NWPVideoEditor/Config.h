#pragma once
#include <string>

class EditorConfig {
private:
    std::wstring m_ffmpegPath;      // Full path to ffmpeg.exe
    std::wstring m_ffprobePath;     // Full path to ffprobe.exe
    bool m_isConfigured;            // Whether FFmpeg is configured

public:
    EditorConfig();

    // Sets FFmpeg folder and updates paths
    void SetFFmpegFolder(const std::wstring& folderPath);

    // Checks if FFmpeg is configured
    bool IsFFmpegConfigured() const { return m_isConfigured; }

    // Returns full paths to executables
    const std::wstring& GetFFmpegExePath() const { return m_ffmpegPath; }
    const std::wstring& GetFFprobeExePath() const { return m_ffprobePath; }

private:
    void UpdatePaths();  // Updates paths and checks if they exist
};
