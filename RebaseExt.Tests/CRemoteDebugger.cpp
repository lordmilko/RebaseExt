#include "stdafx.h"
#include "CRemoteDebugger.h"
#include "CDebugServer.h"
#include "CLocalDebugger.h"

CRemoteDebugger::CRemoteDebugger()
{
    /* Microsoft.VisualStudio.TestTools.CppUnitTestFramework.Executor[.x64].dll directly references DbgHelp.dll, which will result in it
     * being loaded from system32 rather than the proper DbgEng directory; since its a directly referenced assembly we can't simply unload it,
     * and attempts to load the real DbgHelp alongside it don't work as DbgEng inadvertently uses the wrong one. As such, we are forced to create
     * a debug engine out of process so that the real DbgHelp can be used. If DbgHelp is used from system32, when it goes to load symsrv.dll for
     * doing anything with symbols, it will be loaded from system32 (which is wrong) and thus everything to do with symbols will be broken. */

    fnDebugConnect debugConnect = (fnDebugConnect) GetProcAddress(m_DbgEng, "DebugConnect");

    if (debugConnect == nullptr)
        Assert::Fail(L"Failed to find DebugConnect in DbgEng.dll");

    TerminateExistingServers(debugConnect);
    std::string server = m_DebugServer.Start();

    HRESULT hr = debugConnect(server.c_str(), __uuidof(IDebugClient5), reinterpret_cast<void**>(&m_Client5.p));
    Init(hr);

    WaitForBreak();
    LoadExtension();

    m_OutputCallback->AssertNoFailures();
}

void CRemoteDebugger::CreateProcess() const
{
    HRESULT hr;

    if ((hr = m_Client5->CreateProcess(NULL, "notepad", DEBUG_PROCESS)) != S_OK)
        Assert::Fail(L"Failed to create debugger process");

    if ((hr = m_Control4->SetEngineOptions(DEBUG_ENGOPT_INITIAL_BREAK)) != S_OK)
        Assert::Fail(L"Failed to set engine options");

    if ((hr = m_Control4->WaitForEvent(DEBUG_WAIT_DEFAULT, INFINITE)) != S_OK)
        Assert::Fail(L"Failed to wait for initial break event");

    ULONG64 ext;
    if ((hr = m_Control4->AddExtension(TARGET_PATH, 0, &ext)))
        Assert::Fail(L"Failed to load extension");

    m_OutputCallback->AssertNoFailures();
}

std::string CRemoteDebugger::Execute(PCSTR command) const
{
    m_OutputCallback->BeginLogging();

    HRESULT hr = m_Control4->Execute(DEBUG_OUTCTL_THIS_CLIENT, command, DEBUG_EXECUTE_DEFAULT);

    m_OutputCallback->EndLogging();

    std::string str = m_OutputCallback->GetLog();

    if (FAILED(hr))
        Assert::Fail(L"Failed to execute command");

    return str;
}

ULONG64 CRemoteDebugger::Evaluate(PCSTR str) const
{
    HRESULT hr;
    DEBUG_VALUE val;
    ULONG end;

    if ((hr = m_Control4->Evaluate(str, DEBUG_VALUE_INT64, &val, &end) != S_OK))
        Assert::Fail(L"Failed to evaluate expression");

    return val.I64;
}

void CRemoteDebugger::TerminateExistingServers(fnDebugConnect debugConnect)
{
    std::vector<std::string> servers = m_DebugServer.GetServers();

    for (const auto& str : servers)
    {
        if (str.find(PIPE_PREFIX) != std::string::npos)
        {
            IDebugControl* control;
            HRESULT hr = debugConnect(str.c_str(), __uuidof(IDebugControl), reinterpret_cast<void**>(&control));

            if (hr == S_OK)
                control->Execute(DEBUG_OUTCTL_THIS_CLIENT, "qq", DEBUG_EXECUTE_DEFAULT);
        }
    }
}

void CRemoteDebugger::WaitForBreak() const
{
    ULONG status;

    do
    {
        m_Control4->GetExecutionStatus(&status);

        Sleep(100);
    } while (status != DEBUG_STATUS_BREAK);
}

void CRemoteDebugger::LoadExtension() const
{
    HRESULT hr;

    ULONG64 ext;
    if ((hr = m_Control4->AddExtension(TARGET_PATH, 0, &ext)))
        Assert::Fail(L"Failed to load extension");
}
