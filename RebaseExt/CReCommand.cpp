#include "RebaseExt.h"
#include "CReCommand.h"
#include "CMappedFile.h"
#include "CModuleName.h"
#include <strsafe.h>

#include "dbghelp.h"

CReCommand::CReCommand()
{
    m_AddressArg = g_Ext->GetUnnamedArgU64(ARGUMENT_ADDRESS);

    if (g_Ext->HasUnnamedArg(ARGUMENT_MODULE))
        m_ModuleArg = g_Ext->GetUnnamedArgStr(ARGUMENT_MODULE);
    else
        m_ModuleArg = nullptr;

    ZeroMemory(&m_OriginalModule, sizeof(CModuleInfo));
    ZeroMemory(&m_LoadedModule, sizeof(CModuleInfo));
}

void CReCommand::Execute()
{
    if (m_ModuleArg != nullptr)
        ProcessNamedModule();
    else
        ProcessUnnamedModule();
}

#pragma region Named

void CReCommand::ProcessNamedModule()
{
    /* Original.FileName
     *     AlwaysSet
     * Original.BaseAddress
     *     AlwaysSet
     * Original.Address
     *     RVA      : Set by ProcessRVA
     *     Loaded   : Set if within loaded/original bounds
     *     Original : Set if within loaded/original bounds
     * 
     * Loaded.FileName
     *     AlwaysSetCommon
     * Loaded.BaseAddress
     *     AlwaysSet
     * Loaded.Address
     *     RVA      : Set by ProcessRVA
     *     Loaded   : Set if within loaded/original bounds
     *     Original : Set if within loaded/original bounds
     * 
     * ShortModuleName
     *     AlwaysSet
     * ModulePath
     *     AlwaysSet
     * SymbolName
     *     AlwaysSetCommon
     * Rva
     *     RVA      : Set by ProcessRVA
     *     Loaded   : Set if within loaded/original bounds
     *     Original : Set if within loaded/original bounds
     */

    ULONG moduleIndex = g_Ext->FindFirstModule(m_ModuleArg);

    ULONG64 loadedBaseAddress = GetModuleRebaseAddressFromIndex(moduleIndex);

    CModuleName moduleName(loadedBaseAddress);

    strcpy_s(m_ShortModuleName, moduleName.m_ShortName);
    strcpy_s(m_ModulePath, moduleName.m_FilePath);
    strcpy_s(m_OriginalModule.m_FileName, moduleName.m_FileName);

    m_LoadedModule.m_BaseAddress = loadedBaseAddress;
    m_OriginalModule.m_BaseAddress = GetModuleBaseAddressFromFile();

    if (m_AddressArg < m_OriginalModule.m_BaseAddress && m_AddressArg < m_LoadedModule.m_BaseAddress) //The address specified is an RVA
        ProcessRVA();
    else
    {
        IMAGEHLP_MODULEW64 info;
        g_Ext->GetModuleImagehlpInfo(m_LoadedModule.m_BaseAddress, &info);

        if (m_AddressArg >= m_OriginalModule.m_BaseAddress && m_AddressArg <= m_OriginalModule.m_BaseAddress + info.ImageSize)
        {
            //It's a physical address
            m_Rva = m_AddressArg - m_OriginalModule.m_BaseAddress;

            m_OriginalModule.m_Address = m_AddressArg;
            m_LoadedModule.m_Address = m_LoadedModule.m_BaseAddress + m_Rva;

            ProcessModuleCommon();
        }
        else if (m_AddressArg >= m_LoadedModule.m_BaseAddress && m_AddressArg <= m_LoadedModule.m_BaseAddress + info.ImageSize)
        {
            //It's a loaded address
            m_Rva = m_AddressArg - m_LoadedModule.m_BaseAddress;

            m_LoadedModule.m_Address = m_AddressArg;
            m_OriginalModule.m_Address = m_OriginalModule.m_BaseAddress + m_Rva;

            ProcessModuleCommon();
        }
        else
        {
            g_Ext->ThrowInvalidArg(
                "Address 0x%I64x does not exist within module %s.\n     Please specify an address within the following bounds:\n"
                "        Loaded:   0x%I64x - 0x%I64x\n"
                "        Original: 0x%I64x - 0x%I64x",
                m_AddressArg, m_ModuleArg,
                m_LoadedModule.m_BaseAddress, m_LoadedModule.m_BaseAddress + info.ImageSize,
                m_OriginalModule.m_BaseAddress, m_OriginalModule.m_BaseAddress + info.ImageSize
            );
        }
    }
}

void CReCommand::ProcessRVA()
{
    m_Rva = m_AddressArg;

    m_OriginalModule.m_Address = m_OriginalModule.m_BaseAddress + m_AddressArg;
    m_LoadedModule.m_Address = m_LoadedModule.m_BaseAddress + m_AddressArg;

    ProcessModuleCommon();
}

ULONG64 CReCommand::GetModuleRebaseAddressFromIndex(ULONG moduleIndex)
{
    ULONG64 baseAddress;
    HRESULT status = g_Ext->m_Symbols->GetModuleByIndex(moduleIndex, &baseAddress);

    if (status == S_FALSE)
        g_Ext->ThrowStatus(status, "Could not retrieve module base as module is not loaded");

    return baseAddress;
}

void CReCommand::GetUnicodeString(ExtRemoteTyped unicodeString, PSTR buffer, ULONG bufferSize)
{
    ExtRemoteTyped unicodeStringBuffer = unicodeString.Field("Buffer");
    ExtRemoteTyped MaxLength = unicodeString.Field("MaximumLength");

    auto status = g_Ext->m_Data4->ReadUnicodeStringVirtual(unicodeStringBuffer.GetPtr(), MaxLength.GetUshort(), CP_ACP, buffer, bufferSize, NULL);

    if (status != S_OK)
    {
        g_Ext->ThrowStatus(status, "Could not retrieve unicode string");
    }
}

#pragma endregion
#pragma region Unnamed

void CReCommand::ProcessUnnamedModule()
{
   /* Original.FileName
    *     AlwaysSet
    * Original.BaseAddress
    *     AlwaysSet
    * Original.Address
    *     RVA      : N/A (ProcessNamed -> ProcessRVA)
    *     Loaded   : AlwaysSet
    *     Original : AlwaysSet
    *
    * Loaded.FileName
    *     AlwaysSetCommon
    * Loaded.BaseAddress
    *     AlwaysSet
    * Loaded.Address
    *     RVA      : N/A (ProcessNamed -> ProcessRVA)
    *     Loaded   : AlwaysSet
    *     Original : AlwaysSet
    *
    * ShortModuleName
    *     AlwaysSet
    * ModulePath
    *     AlwaysSet
    * SymbolName
    *     AlwaysSetCommon
    * Rva
    *     RVA      : N/A (ProcessNamed -> ProcessRVA)
    *     Loaded   : AlwaysSet
    *     Original : AlwaysSet
    */

    ULONG64 loadedBaseAddress = GetModuleRebaseFromAddress();

    CModuleName moduleName(loadedBaseAddress);

    strcpy_s(m_ShortModuleName, moduleName.m_ShortName);
    strcpy_s(m_ModulePath, moduleName.m_FilePath);

    //Set the original module name; the loaded module name is the name of the program that's running,
    //which will be set in ProcessModuleCommon
    strcpy_s(m_OriginalModule.m_FileName, moduleName.m_FileName);

    m_Rva = m_AddressArg - loadedBaseAddress;

    m_LoadedModule.m_BaseAddress = loadedBaseAddress;
    m_OriginalModule.m_BaseAddress = GetModuleBaseAddressFromFile();

    m_LoadedModule.m_Address = m_AddressArg;
    m_OriginalModule.m_Address = m_OriginalModule.m_BaseAddress + m_Rva;

    ProcessModuleCommon();
}

ULONG64 CReCommand::GetModuleRebaseFromAddress() const
{
    ULONG64 baseAddress;

    HRESULT hr = g_Ext->m_Symbols->GetModuleByOffset(m_AddressArg, 0, 0, &baseAddress);

    if (hr != S_OK)
    {
        g_Ext->ThrowStatus(
            hr,
            "Could not retrieve module base from address. Possible causes:\n"
            "        -Address was an RVA or original address (must specify the module name)\n"
            "        -Module is unloaded\n"
            "        -Address is invalid"
        );
    }

    return baseAddress;
}

#pragma endregion
#pragma region Common

void CReCommand::ProcessModuleCommon()
{
    GetCurrentProcessExecutableName();

    GetAddressSymbolName(m_LoadedModule.m_Address);

    WriteOutput();
}

void CReCommand::GetCurrentProcessExecutableName()
{
    CHAR buffer[MAX_PATH];

    //Set the loaded module name (with file extensions). We already got the unloaded module name when we calculated the module path
    g_Ext->m_System->GetCurrentProcessExecutableName(buffer, MAX_PATH, NULL);

    char* name = strrchr(buffer, '\\');

    if (name != nullptr)
        strcpy_s(m_LoadedModule.m_FileName, MAX_PATH, name + 1);
    else
        strcpy_s(m_LoadedModule.m_FileName, MAX_PATH, buffer);
}

void CReCommand::GetAddressSymbolName(ULONG64 address)
{
    ULONG64 displacement;
    ULONG nameSize;
    ZeroMemory(m_SymbolName, MAX_PATH);
    
    if (g_Ext->m_Symbols->GetNameByOffset(address, m_SymbolName, MAX_PATH, &nameSize, &displacement) == S_OK)
    {
        if (displacement != 0)
        {
            PSTR ptr = m_SymbolName + nameSize - 1;
            ULONG remainingSize = MAX_PATH - nameSize - 1;

            if ((displacement >> 32) != 0)
                StringCbPrintf(ptr, remainingSize, "+%x`%08x", (ULONG)(displacement >> 32), (ULONG)displacement);
            else
                StringCbPrintf(ptr, remainingSize, "+%x", (ULONG)displacement);
        }
    }
    else
        strcpy_s(m_SymbolName, "No Symbol");
}

void CReCommand::WriteOutput()
{
    g_Ext->Out("\n");
    g_Ext->Out("RVA: %I64x\n", m_Rva);
    g_Ext->Out("\n");

    DisplayModuleInfo("Loaded", m_LoadedModule);
    g_Ext->Out("\n");
    DisplayModuleInfo("Original", m_OriginalModule);
    g_Ext->Out("\n");
}

void CReCommand::DisplayModuleInfo(PSTR title, CModuleInfo& info)
{
    //x64 formatting comes from FormatDisp64. lm and set a breakpoint there and compare with our running Out and see if we have a similar code path and why things are different

    g_Ext->Out("%s Address (%s)\n", title, info.m_FileName);
    g_Ext->Out("    Base:    %I64x (%s)\n", info.m_BaseAddress, m_ShortModuleName);

    if (m_SymbolName != "")
        g_Ext->Out("    Address: %I64x (%s)\n", info.m_Address, m_SymbolName);
    else
        g_Ext->Out("    Address: %I64x\n", info.m_Address);
}

ULONG64 CReCommand::GetModuleBaseAddressFromFile() const
{
    //When Windows loads DLLs, it even patches up the address in the header in memory, hence we can't g_Ext->m_Data->ReadVirtual
    //this data, we have to read it straight from the source
    CMappedFile file(m_ModulePath);

    return file.GetModuleBaseAddress();
}

#pragma endregion











