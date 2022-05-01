#pragma once

#include <atlbase.h>
#include "dbgeng.h"
#include "COutputCallback.h"

class CDebugger
{
public:
    CDebugger();
    ~CDebugger();

protected:
    void Init(HRESULT hr);
    
    HMODULE m_DbgEng = nullptr;
    COutputCallback* m_OutputCallback = nullptr;

    CComPtr<IDebugClient5> m_Client5;
    CComPtr<IDebugControl4> m_Control4;
    CComPtr<IDebugSymbols> m_Symbols;
};