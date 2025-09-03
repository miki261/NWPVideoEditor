#include "pch.h"
#include "FfmpegProcess.h"
#include <windows.h>
#include <sstream>

static std::wstring Quote(const std::wstring& s)
{
    std::wstring q; q.reserve(s.size() + 2);
    q.push_back(L'\"'); q += s; q.push_back(L'\"');
    return q;
}

static std::wstring JoinCmd(const std::wstring& exe, const std::vector<std::wstring>& args)
{
    std::wstringstream ss;
    ss << Quote(exe);
    for (const auto& a : args) ss << L' ' << Quote(a);
    return ss.str();
}

ExecResult FfmpegProcess::Run(const std::wstring& exePath, const std::vector<std::wstring>& args) const
{
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE outRd = nullptr, outWr = nullptr;
    HANDLE errRd = nullptr, errWr = nullptr;

    if (!CreatePipe(&outRd, &outWr, &sa, 0))
        return { -1, "", "CreatePipe stdout failed" };

    if (!CreatePipe(&errRd, &errWr, &sa, 0))
    {
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
    std::wstring cmd = JoinCmd(exePath, args);

    // FIXED - Create mutable copy for CreateProcessW
    std::vector<wchar_t> mutableCmd(cmd.begin(), cmd.end());
    mutableCmd.push_back(L'\0');

    BOOL ok = CreateProcessW(
        nullptr,
        mutableCmd.data(),  // FIXED - Use wchar_t* instead of wstring.data()
        nullptr,
        nullptr,
        TRUE,
        CREATE_NO_WINDOW,
        nullptr,
        nullptr,
        &si,
        &pi
    );

    CloseHandle(outWr);
    CloseHandle(errWr);

    std::string out, err;

    if (ok)
    {
        WaitForSingleObject(pi.hProcess, INFINITE);

        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);

        BYTE  buf[4096];
        DWORD read = 0;

        // Read stdout
        for (;;)
        {
            if (!ReadFile(outRd, buf, sizeof(buf), &read, nullptr) || read == 0)
                break;
            out.append(reinterpret_cast<const char*>(buf), read);
        }

        // Read stderr  
        for (;;)
        {
            if (!ReadFile(errRd, buf, sizeof(buf), &read, nullptr) || read == 0)
                break;
            err.append(reinterpret_cast<const char*>(buf), read);
        }

        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        CloseHandle(outRd);
        CloseHandle(errRd);

        return { static_cast<int>(exitCode), out, err };
    }

    CloseHandle(outRd);
    CloseHandle(errRd);
    return { -1, "", "CreateProcess failed" };
}
