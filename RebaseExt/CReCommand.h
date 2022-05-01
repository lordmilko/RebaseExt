#pragma once
#include "CModuleInfo.h"

#define ARGUMENT_ADDRESS 0
#define ARGUMENT_MODULE 1

class CReCommand
{
public:
    CReCommand();
    void Execute();

    static void GetUnicodeString(ExtRemoteTyped unicodeString, PSTR buffer, ULONG bufferSize);

private:
    ULONG64 m_AddressArg;             //The address that was specified to !re; could be an RVA, the original or loaded address
    PCSTR m_ModuleArg;
    
    CHAR m_ShortModuleName[MAX_PATH]; //The short name of the module (e.g. "kernel32")
    CHAR m_ModulePath[MAX_PATH];      //The full path to the module (e.g. C:\Windows\system32\kernel32.dll)

    CModuleInfo m_OriginalModule;
    CModuleInfo m_LoadedModule;

    ULONG64 m_Rva;
    CHAR m_SymbolName[MAX_PATH];

    #pragma region Named

    void ProcessNamedModule();
    void ProcessRVA();
    ULONG64 GetModuleRebaseAddressFromIndex(ULONG moduleIndex); //Gets the loaded address of a module from its module index
    void GetModulePathAndName(ULONG64 baseAddress, PSTR modulePath, PSTR moduleName, ULONG bufferSize);

    #pragma endregion
    #pragma region Unnamed

    void ProcessUnnamedModule();
    ULONG64 GetModuleRebaseFromAddress() const;

    #pragma endregion
    #pragma region Common

    void ProcessModuleCommon();
    void GetCurrentProcessExecutableName();
    void GetAddressSymbolName(ULONG64 address);
    void WriteOutput();
    void DisplayModuleInfo(PSTR title, CModuleInfo& info);
    ULONG64 GetModuleBaseAddressFromFile() const;

    #pragma endregion
};
