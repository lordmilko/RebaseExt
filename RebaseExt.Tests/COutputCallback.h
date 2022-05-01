#pragma once
#include "dbgeng.h"

class COutputCallback : public IDebugOutputCallbacks
{
public:
    COutputCallback()
    {
        m_RefCount = 1;
    }

    STDMETHODIMP QueryInterface(_In_ REFIID InterfaceId, _Out_ PVOID* Interface) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;
    STDMETHODIMP Output(_In_ ULONG Mask, _In_ PCSTR Text) override;

    void AssertNoFailures();
    void BeginLogging();
    void EndLogging();
    std::string GetLog();

    std::vector<std::string> m_LogList;

private:
    long m_RefCount;
    bool m_Log = false;

    std::vector<std::wstring> m_Failures;
};