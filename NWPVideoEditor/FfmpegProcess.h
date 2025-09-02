#pragma once
#include <afx.h>
#include <string>
#include <vector>

struct ExecResult
{
    int         exitCode;
    std::string stdoutText;
    std::string stderrText;
};

class FfmpegProcess : public CObject
{
public:
    ExecResult Run(const std::wstring& exePath, const std::vector<std::wstring>& args) const;
};
