#include "stdafx.h"
#include "CDebugger.h"

CDebugger::CDebugger()
{
    SetDllDirectory(DEBUGGERS_PATH);

    m_DbgEng = LoadLibrary(L"dbgeng.dll");

    if (m_DbgEng == nullptr)
        Assert::Fail(L"Failed to load DbgEng.dll");
}

void CDebugger::Init(HRESULT hr)
{
    if (FAILED(hr))
    {
        std::wstringstream ss;
        ss << L"Failed to create IDebugClient" << std::hex << hr;

        Assert::Fail(ss.str().c_str());
    }

    m_Control4 = m_Client5;
    m_Symbols = m_Client5;

    m_OutputCallback = new COutputCallback();

    if ((hr = m_Client5->SetOutputCallbacks(m_OutputCallback)) != S_OK)
        Assert::Fail(L"Failed to set output callbacks");
}

CDebugger::~CDebugger()
{
    m_Symbols.Release();
    m_Control4.Release();

    if (m_Client5.p)
    {
        m_Client5->TerminateProcesses();
        m_Client5.Release();
    }

    if (m_OutputCallback)
    {
        m_OutputCallback->Release();
        m_OutputCallback = nullptr;
    }

    if (m_DbgEng)
    {
        FreeLibrary(m_DbgEng);
        m_DbgEng = nullptr;
    }    
}