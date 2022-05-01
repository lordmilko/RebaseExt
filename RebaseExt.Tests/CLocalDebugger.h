#pragma once
#include "CDebugger.h"

typedef HRESULT(STDAPICALLTYPE *fnDebugCreate)(
    _In_ REFIID InterfaceId,
    _Out_ PVOID* Interface);

class CLocalDebugger : public CDebugger
{
public:
    CLocalDebugger();

    std::vector<std::string> GetServers() const;
};