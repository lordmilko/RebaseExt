#include "stdafx.h"
#include "COutputCallback.h"

STDMETHODIMP COutputCallback::QueryInterface(
    _In_ REFIID InterfaceId,
    _Out_ PVOID* Interface)
{
    if (InterfaceId == IID_IUnknown)
        *Interface = static_cast<IUnknown*>(this);
    else if (InterfaceId == __uuidof(IDebugOutputCallbacks))
        *Interface = static_cast<IDebugOutputCallbacks*>(this);
    else
    {
        *Interface = nullptr;
        return E_NOINTERFACE;
    }

    AddRef();

    return S_OK;
}

STDMETHODIMP_(ULONG) COutputCallback::AddRef()
{
    return InterlockedIncrement(&m_RefCount);
}

STDMETHODIMP_(ULONG) COutputCallback::Release()
{
    auto ret = InterlockedDecrement(&m_RefCount);

    if (ret <= 0)
        delete this;

    return ret;
}

STDMETHODIMP COutputCallback::Output(
    _In_ ULONG Mask,
    _In_ PCSTR Text)
{
    int wideLength = MultiByteToWideChar(
        CP_UTF8,
        0,
        Text,
        -1,
        nullptr,
        0
    );

    std::wstring wide(wideLength, 0);;

    MultiByteToWideChar(
        CP_UTF8,
        0,
        Text,
        -1,
        &wide[0],
        wideLength
    );

    if ((Mask & DEBUG_OUTPUT_ERROR) == DEBUG_OUTPUT_ERROR)
        m_Failures.push_back(wide);
    else if ((Mask & DEBUG_OUTPUT_EXTENSION_WARNING) == DEBUG_OUTPUT_EXTENSION_WARNING)
        m_Failures.push_back(wide);

    if (m_Log)
        m_LogList.emplace_back(Text);

    OutputDebugStringA(Text);
    return S_OK;
}

void COutputCallback::AssertNoFailures()
{
    if (!m_Failures.empty())
    {
        std::wstringstream ss;

        if (m_Failures[0].find(L"Unable to deliver callback") != std::string::npos)
        {
            m_Failures.erase(m_Failures.begin(), m_Failures.begin() + 3);

            AssertNoFailures();

            return;
        }

        for (const auto& failure : m_Failures)
            ss << failure;

        Assert::Fail(ss.str().c_str());
    }
}

void COutputCallback::BeginLogging()
{
    m_LogList.clear();
    m_Log = true;
}

void COutputCallback::EndLogging()
{
    m_Log = false;
}

std::string COutputCallback::GetLog()
{
    std::stringstream ss;

    for (const auto& item : m_LogList)
        ss << item;

    return ss.str();
}