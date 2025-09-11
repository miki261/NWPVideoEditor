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

    // Add public getters for the paths
    const std::wstring& GetFFmpegExePath() const { return m_ffmpegExe; }
    const std::wstring& GetFFprobeExePath() const { return m_ffprobeExe; }
    const std::wstring& GetTempDir() const { return m_tempDir; }

    static bool LoadFromFile(const std::wstring& path, EditorConfig& config);

private:
    std::wstring m_ffmpegExe;
    std::wstring m_ffprobeExe;
    std::wstring m_tempDir;
    std::vector<std::wstring> m_defaultArgs;
    std::unordered_map<std::string, RenderProfile> m_profiles;
};
