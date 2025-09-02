#pragma once
#include <string>
#include <vector>
#include <optional>
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
    EditorConfig(EditorConfig&&) noexcept = default;
    EditorConfig& operator=(EditorConfig&&) noexcept = default;

    std::wstring FfmpegExePath() const;
    std::wstring FfprobeExePath() const;
    const RenderProfile* GetProfile(const std::string& name) const;
    const std::vector<std::wstring>& DefaultArgs() const;

    static std::optional<EditorConfig> LoadFromFile(const std::wstring& path);

private:
    std::wstring m_binDir;
    std::wstring m_ffmpegExe;
    std::wstring m_ffprobeExe;
    std::wstring m_tempDir;
    std::vector<std::wstring> m_defaultArgs;
    std::unordered_map<std::string, RenderProfile> m_profiles;
};
