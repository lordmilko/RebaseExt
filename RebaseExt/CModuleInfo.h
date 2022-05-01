#pragma once

class CModuleInfo
{
public:
    CHAR m_FileName[MAX_PATH];
    ULONG64 m_BaseAddress;
    ULONG64 m_Address;
};