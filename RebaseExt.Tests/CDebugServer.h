#pragma once

#include "CLocalDebugger.h"

#define PIPE_PREFIX "RebaseExtTest_"

class CDebugServer
{
public:
    std::string Start();
    std::vector<std::string> GetServers();
    std::string GetServer(DWORD clientId);

private:
    CLocalDebugger m_Debugger;
};