#include "pch.h"
#include "ArgsBuilder.h"

std::vector<std::wstring> ArgsBuilder::BuildSimpleExport(const std::wstring& in,
    const std::wstring& out,
    const RenderProfile& p,
    const std::vector<std::wstring>& defaults,
    const std::wstring& vf)
{
    std::vector<std::wstring> a = defaults;
    a.push_back(L"-i"); a.push_back(in);
    if (!vf.empty()) { a.push_back(L"-vf"); a.push_back(vf); }

    a.push_back(L"-c:v");    a.push_back(std::wstring(p.videoCodec.begin(), p.videoCodec.end()));
    a.push_back(L"-preset"); a.push_back(std::wstring(p.preset.begin(), p.preset.end()));
    a.push_back(L"-pix_fmt");a.push_back(std::wstring(p.pixFmt.begin(), p.pixFmt.end()));
    a.push_back(L"-b:v");    a.push_back(std::wstring(p.videoBitrate.begin(), p.videoBitrate.end()));
    a.push_back(L"-c:a");    a.push_back(std::wstring(p.audioCodec.begin(), p.audioCodec.end()));
    a.push_back(L"-b:a");    a.push_back(std::wstring(p.audioBitrate.begin(), p.audioBitrate.end()));
    a.push_back(L"-ar");     a.push_back(std::to_wstring(p.audioRate));
    a.push_back(L"-ac");     a.push_back(std::to_wstring(p.audioChannels));
    a.push_back(L"-crf");    a.push_back(std::to_wstring(p.crf));

    a.push_back(out);
    return a;
}
