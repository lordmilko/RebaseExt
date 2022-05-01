#pragma once

class CModuleName;

typedef void (CModuleName::* NameMethod)();

#pragma comment(lib, "dbghelp.lib")

class CModuleName
{
public:
    PSTR m_ShortName = nullptr;
    PSTR m_FileName = nullptr;
    PSTR m_FilePath = nullptr;

    CModuleName(ULONG64 baseAddress);
    ~CModuleName();

private:
    static NameMethod NameMethods[];

    ULONG64 m_BaseAddress;

    void IDebugSymbols_GetModuleNames();
    void IDebugSymbols2_GetModuleNameString();
    void IDebugSymbols2_GetModuleNameStringInternal(ULONG type);
    void GetFileNameFromFilePath();
    void IDebugSystemObjects_GetCurrentProcessExecutableName();
    void DbgHelp_SymFindFileInPath();
    void ExtEngCpp_ReadPebModules();

    void TrySetAnyName(PSTR name);
    void TrySetModuleName(PSTR name);
    void TrySetFileName(PSTR name);
    void TrySetFilePath(PSTR name);

    bool MissingNames() const;
};