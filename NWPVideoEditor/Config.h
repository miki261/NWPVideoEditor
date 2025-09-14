#pragma once
#include <string>
#include <vector>
#include <unordered_map>

struct RenderProfile {
    std::string container;
    std::string videoCodec;
    std::string audioCodec;
    std::string pixFmt;
    std::string videoBitrate;
    std::string audioBitrate;
    std::string preset;
    int         crf;
    int         audioChannels;
    int         audioRate;
};

class EditorConfig {
public:
    EditorConfig();
    ~EditorConfig();
    
    EditorConfig(const EditorConfig&) = default;
    EditorConfig& operator=(const EditorConfig&) = default;
    
    const RenderProfile* GetProfile(const std::string& name) const;
    const std::vector<std::wstring>& DefaultArgs() const;
    
    // Path getters - return full paths or empty if not configured
    const std::wstring& GetFFmpegExePath() const { return m_ffmpegPath; }
    const std::wstring& GetFFprobeExePath() const { return m_ffprobePath; }
    const std::wstring& GetTempDir() const { return m_tempDir; }
    const std::wstring& GetFFmpegFolder() const { return m_ffmpegFolder; }
    
    // FFmpeg folder methods
    void SetFFmpegFolder(const std::wstring& folderPath);
    bool IsFFmpegConfigured() const;
    
    static bool LoadFromFile(const std::wstring& path, EditorConfig& config);

private:
    std::wstring m_ffmpegFolder;    // Base folder
    std::wstring m_ffmpegPath;      // Full path to ffmpeg.exe
    std::wstring m_ffprobePath;     // Full path to ffprobe.exe
    std::wstring m_tempDir;
    std::vector<std::wstring> m_defaultArgs;
    std::unordered_map<std::string, RenderProfile> m_profiles;
    
    void UpdateExecutablePaths();
};
