#include "stdafx.h"
#include "CLocalDebugger.h"
#include "CDebugServer.h"

std::string CDebugServer::Start()
{
    std::wstringstream ss;

    DWORD pid = GetCurrentProcessId();

    ss << DEBUGGERS_PATH << L"\\ntsd.exe -server npipe:pipe=" << PIPE_PREFIX << pid << " -noio notepad";

    WCHAR process[MAX_PATH];
    wcscpy_s(process, ss.str().c_str());

    PROCESS_INFORMATION pi;
    STARTUPINFO si;

    ZeroMemory(&pi, sizeof(pi));
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    BOOL success = CreateProcess(
        NULL,       //Application name
        process, //Command line
        NULL,       //Security attributes
        NULL,       //Thread attributes
        TRUE,       //Inherit handles
        0,          //Creation flags
        NULL,       //Use our environment
        NULL,       //Use our current directory
        &si,        //Startup info
        &pi         //Process info
    );

    if (!success)
        Assert::Fail(L"Failed to create debug server");

    auto server = GetServer(GetCurrentProcessId());

    return server;
}

std::vector<std::string> CDebugServer::GetServers()
{
    return m_Debugger.GetServers();
}

std::string CDebugServer::GetServer(DWORD clientId)
{
    for(int i = 0; i < 10; i++)
    {
        auto servers = GetServers();

        for (auto str : servers)
        {
            auto startPos = str.find(PIPE_PREFIX);

            if (startPos == std::string::npos)
                continue;

            startPos += strlen(PIPE_PREFIX);

            auto endPos = str.find(',');

            if (endPos == std::string::npos)
                continue;

            auto subStr = str.substr(startPos, endPos - startPos);

            auto pid = (DWORD)std::stoi(subStr);

            if (pid == clientId)
            {
                std::string ret = str;

                return ret;
            }
        }

        //Retry in 100 ms, maybe the process is still starting up
        Sleep(100);
    }

    Assert::Fail(L"Failed to get server connection string; server must not have initialized properly");

    throw "This exception should be unreachable";
}