#include "pch.h"
#include "FfmpegProcess.h"
#include <windows.h>
#include <sstream>

static std::wstring BuildCommandLine(const std::wstring& exe, const std::vector<std::wstring>& args) {
    auto quote = [](const std::wstring& s) -> std::wstring {
        return L"\"" + s + L"\"";
        };
    std::wstringstream ss;
    ss << quote(exe);
    for (const auto& a : args) ss << L' ' << quote(a);
    return ss.str();
}

ExecResult FfmpegProcess::Run(const std::wstring& exePath, const std::vector<std::wstring>& args) const {
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE outRd = nullptr, outWr = nullptr;
    HANDLE errRd = nullptr, errWr = nullptr;

    if (!CreatePipe(&outRd, &outWr, &sa, 0))
        return { -1, "", "CreatePipe stdout failed" };

    if (!CreatePipe(&errRd, &errWr, &sa, 0)) {
        CloseHandle(outRd); CloseHandle(outWr);
        return { -1, "", "CreatePipe stderr failed" };
    }

    SetHandleInformation(outRd, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(errRd, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = outWr;
    si.hStdError = errWr;

    PROCESS_INFORMATION pi{};
    std::wstring cmd = BuildCommandLine(exePath, args);
    std::vector<wchar_t> mutableCmd(cmd.begin(), cmd.end());
    mutableCmd.push_back(L'\0');

    BOOL ok = CreateProcessW(nullptr, mutableCmd.data(), nullptr, nullptr,
        TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);

    CloseHandle(outWr);
    CloseHandle(errWr);

    if (!ok) {
        CloseHandle(outRd); CloseHandle(errRd);
        return { -1, "", "CreateProcess failed" };
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    std::string out, err;
    BYTE buf[4096];
    DWORD read = 0;

    while (ReadFile(outRd, buf, sizeof(buf), &read, nullptr) && read > 0)
        out.append(reinterpret_cast<const char*>(buf), read);

    while (ReadFile(errRd, buf, sizeof(buf), &read, nullptr) && read > 0)
        err.append(reinterpret_cast<const char*>(buf), read);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(outRd);
    CloseHandle(errRd);

    return { static_cast<int>(exitCode), out, err };
}