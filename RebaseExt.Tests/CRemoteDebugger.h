#pragma once
#include "CDebugger.h"
#include "CDebugServer.h"

typedef HRESULT(STDAPICALLTYPE *fnDebugConnect)(
    _In_ PCSTR RemoteOptions,
    _In_ REFIID InterfaceId,
    _Out_ PVOID* Interface
    );

class CRemoteDebugger : public CDebugger
{
public:
    CRemoteDebugger();
    void CreateProcess() const;
    std::string Execute(PCSTR command) const;
    ULONG64 Evaluate(PCSTR str) const;

private:
    CDebugServer m_DebugServer;

    void TerminateExistingServers(fnDebugConnect debugConnect);
    void WaitForBreak() const;
    void LoadExtension() const;
};