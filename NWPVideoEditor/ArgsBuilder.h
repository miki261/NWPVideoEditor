#pragma once
#include <afx.h>
#include <string>
#include <vector>
#include "Config.h"

class ArgsBuilder : public CObject
{
public:
    static std::vector<std::wstring> BuildSimpleExport(const std::wstring& in,
        const std::wstring& out,
        const RenderProfile& p,
        const std::vector<std::wstring>& defaults,
        const std::wstring& vfChain);
};
