#include "stdafx.h"
#include "CLocalDebugger.h"
#include <algorithm>

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string item;

    while (getline(ss, item, delim))
        result.push_back(item);

    return result;
}

CLocalDebugger::CLocalDebugger()
{
    fnDebugCreate debugCreate = (fnDebugCreate)GetProcAddress(m_DbgEng, "DebugCreate");

    if (debugCreate == nullptr)
        Assert::Fail(L"Failed to find DebugCreate in DbgEng.dll");

    HRESULT hr = debugCreate(__uuidof(IDebugClient), reinterpret_cast<void**>(&m_Client5.p));

    Init(hr);
}

std::vector<std::string> CLocalDebugger::GetServers() const
{
    std::vector<std::string> list;

    m_OutputCallback->BeginLogging();

    ULONG mask;
    m_Client5->GetOutputMask(&mask);

    WCHAR buffer[15];
    GetEnvironmentVariable(L"COMPUTERNAME", buffer, 15);

    HRESULT hr = m_Client5->OutputServersWide(DEBUG_OUTCTL_THIS_CLIENT, buffer, DEBUG_SERVERS_DEBUGGER);

    m_OutputCallback->EndLogging();

    for (const auto& entry : m_OutputCallback->m_LogList)
    {
        auto lines = split(entry, '\n');

        for(auto line : lines)
        {
            line += ",Server=localhost";

            std::string::size_type first = line.find('-');

            if (first != std::string::npos)
                list.emplace_back(&line[0] + first + 2); //+2: remove the - and the space after it
            else
                list.push_back(line);
        }
    }

    return list;
}