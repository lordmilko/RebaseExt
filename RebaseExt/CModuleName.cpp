#include <engextcpp.hpp>
#include "CModuleName.h"
#include "CReCommand.h"

#include <dbghelp.h>

bool stringEndsWith(const char* str, const char* subStr)
{
    int strLen = strlen(str);
    int subStrLen = strlen(subStr);

    if (strLen >= subStrLen)
        return strncmp(str + (strLen - subStrLen), subStr, subStrLen) == 0;

    return false;
}

int stringIndexOf(PSTR str, char c)
{
    PSTR result = strchr(str, c);

    if (result == nullptr)
        return -1;

    return int(result - str);
}

int stringLastIndexOf(PSTR str, char c)
{
    PSTR result = strrchr(str, c);

    if (result == nullptr)
        return -1;

    return int(result - str);
}

NameMethod CModuleName::NameMethods[] = {
    &CModuleName::IDebugSymbols_GetModuleNames,
    &CModuleName::IDebugSymbols2_GetModuleNameString,
    &CModuleName::GetFileNameFromFilePath,
    &CModuleName::IDebugSystemObjects_GetCurrentProcessExecutableName,
    &CModuleName::ExtEngCpp_ReadPebModules,
    &CModuleName::DbgHelp_SymFindFileInPath
};

CModuleName::CModuleName(ULONG64 baseAddress)
{
    //The name and path of a module. How hard could it be?

    m_BaseAddress = baseAddress;

    for(int i = 0; i < ARRAYSIZE(NameMethods) && MissingNames(); i++)
        (this->*NameMethods[i])();

    if (m_ShortName == nullptr)
        g_Ext->ThrowStatus(E_FAIL, "Could not find the module name of the specified address. I tried D:");

    if (m_FileName == nullptr)
        g_Ext->ThrowStatus(E_FAIL, "Could not find the file name of the module at the specified address. I tried D:");

    if (m_FilePath == nullptr)
        g_Ext->ThrowStatus(E_FAIL, "Could not find the full file path of the module at the specified address. I tried D:");
}

CModuleName::~CModuleName()
{
    if (m_ShortName != nullptr)
    {
        free(m_ShortName);
        m_ShortName = nullptr;
    }

    if (m_FileName != nullptr)
    {
        free(m_FileName);
        m_FileName = nullptr;
    }   

    if (m_FilePath != nullptr)
    {
        free(m_FilePath);
        m_FilePath = nullptr;
    }
}

void CModuleName::IDebugSymbols_GetModuleNames()
{
    CHAR Name_ImageNameBuffer[MAX_PATH];
    ULONG ImageNameBufferSize = sizeof(Name_ImageNameBuffer);
    ULONG ImageNameSize;

    CHAR Name_ModuleNameBuffer[MAX_PATH];
    ULONG ModuleNameBufferSize = sizeof(Name_ModuleNameBuffer);
    ULONG ModuleNameSize;

    CHAR Name_LoadedImageNameBuffer[MAX_PATH];
    ULONG LoadedImageBufferSize = sizeof(Name_LoadedImageNameBuffer);
    ULONG LoadedImageSize;

    HRESULT status = g_Ext->m_Symbols->GetModuleNames(DEBUG_ANY_ID, m_BaseAddress,
        Name_ImageNameBuffer, ImageNameBufferSize, &ImageNameSize,   //ImageName
        Name_ModuleNameBuffer, ModuleNameBufferSize, &ModuleNameSize,  //ModuleName
        Name_LoadedImageNameBuffer, LoadedImageBufferSize, &LoadedImageSize  //LoadedImageName
    );

    TrySetAnyName(Name_ImageNameBuffer);
    TrySetAnyName(Name_ModuleNameBuffer);
    TrySetAnyName(Name_LoadedImageNameBuffer);
}

void CModuleName::IDebugSymbols2_GetModuleNameString()
{
    IDebugSymbols2_GetModuleNameStringInternal(DEBUG_MODNAME_IMAGE);
    IDebugSymbols2_GetModuleNameStringInternal(DEBUG_MODNAME_MODULE);
    IDebugSymbols2_GetModuleNameStringInternal(DEBUG_MODNAME_LOADED_IMAGE);
    IDebugSymbols2_GetModuleNameStringInternal(DEBUG_MODNAME_MAPPED_IMAGE);
}

void CModuleName::IDebugSymbols2_GetModuleNameStringInternal(ULONG type)
{
    CHAR name[MAX_PATH];

    HRESULT result = g_Ext->m_Symbols2->GetModuleNameString(
        type,
        DEBUG_ANY_ID,
        m_BaseAddress,
        name,
        sizeof(name),
        NULL
    );

    if (result == S_OK)
        TrySetAnyName(name);
}

void CModuleName::GetFileNameFromFilePath()
{
    if (m_FileName == nullptr && m_FilePath != nullptr)
    {
        PSTR str = strrchr(m_FilePath, '\\') + 1;

        TrySetFileName(str);
    }
}

void CModuleName::IDebugSystemObjects_GetCurrentProcessExecutableName()
{
    if (m_FileName != nullptr)
    {
        CHAR Name_GetCurrentProcessExecutableName[MAX_PATH];

        g_Ext->m_System->GetCurrentProcessExecutableName(Name_GetCurrentProcessExecutableName, sizeof(Name_GetCurrentProcessExecutableName), NULL);

        if (stringEndsWith(Name_GetCurrentProcessExecutableName, m_FileName))
            TrySetAnyName(Name_GetCurrentProcessExecutableName);
    }
}

void CModuleName::DbgHelp_SymFindFileInPath()
{
    if (m_FileName == nullptr)
        g_Ext->ThrowStatus(E_FAIL, "Cannot run SymFindFileInPath as module file name was not discovered");

    IMAGE_NT_HEADERS64 headers;
    g_Ext->m_Data3->ReadImageNtHeaders(m_BaseAddress, &headers);

    HANDLE handle;
    GetCurrentProcessHandle(&handle);

    CHAR Name_FilePath[MAX_PATH];
    auto result = SymFindFileInPath(
        handle,                             //Handle
        NULL,                               //Search Path
        Name_FilePath,                      //FileName
        &headers.FileHeader.TimeDateStamp,  //ID 1
        headers.OptionalHeader.SizeOfImage, //ID 2
        0,                                  //ID 3
        SSRVOPT_DWORDPTR,                   //Flags
        Name_FilePath,                      //FilePath
        NULL,                               //Callback
        NULL                                //Context
    );

    if (result)
    {
        TrySetAnyName(Name_FilePath);
    }
}

void CModuleName::ExtEngCpp_ReadPebModules()
{
    try
    {
        ExtRemoteTyped Peb("(ntdll!_PEB*)@$peb");

        ExtRemoteTyped moduleListHead = Peb.Field("Ldr.InMemoryOrderModuleList");
        ExtRemoteTypedList moduleList(moduleListHead, "ntdll!_LDR_DATA_TABLE_ENTRY", "InMemoryOrderLinks");

        CHAR Name_FileName[MAX_PATH];
        CHAR Name_FilePath[MAX_PATH];

        bool found = true;

        for (moduleList.StartHead(); moduleList.HasNode(); moduleList.Next())
        {
            ExtRemoteTyped module = moduleList.GetTypedNode();

            ExtRemoteTyped itemBaseRemote = module.Field("DllBase");

            //GetUlong64() will assert that the buffer is 8 bytes, which will fail when compiled for x86.
            //We always store the base address as a ULONG64, so we're happy to just upcast a smaller value if need be
            auto itemBase = itemBaseRemote.m_Data;

            if (itemBase == m_BaseAddress)
            {
                ExtRemoteTyped moduleNameUS = module.Field("BaseDllName");
                CReCommand::GetUnicodeString(moduleNameUS, Name_FileName, sizeof(Name_FileName));

                ExtRemoteTyped modulePathUS = module.Field("FullDllName");
                CReCommand::GetUnicodeString(modulePathUS, Name_FilePath, sizeof(Name_FilePath));

                found = true;
                break;
            }
        }

        if (found)
        {
            TrySetAnyName(Name_FileName);
            TrySetAnyName(Name_FilePath);
        }
    }
    catch (...)
    {
        //We'll try something else
    }
}


void CModuleName::TrySetAnyName(PSTR name)
{
    if (m_ShortName == nullptr)
        TrySetModuleName(name);
    if (m_FileName == nullptr)
        TrySetFileName(name);
    if (m_FilePath == nullptr)
        TrySetFilePath(name);
}

void CModuleName::TrySetModuleName(PSTR name)
{
    int strLen = strlen(name);

    //If the name does not have a file extension (indicated by a fullstop 4 chars from the back) and does not have a slash in it, its a module name
    if (stringLastIndexOf(name, '.') != strLen - 4 && stringIndexOf(name, '\\') == -1)
    {
        int length = strLen + 1;
        m_ShortName = static_cast<PSTR>(malloc(length));
        strcpy_s(m_ShortName, length, name);
    }
}

void CModuleName::TrySetFileName(PSTR name)
{
    int strLen = strlen(name);

    //If the name has a file extension (indicated by fullstop 4 chars from the back) and has no slash in it, its a file name
    if (stringLastIndexOf(name, '.') == strLen - 4 && stringIndexOf(name, '\\') == -1)
    {
        int length = strLen + 1;
        m_FileName = static_cast<PSTR>(malloc(length));
        strcpy_s(m_FileName, length, name);
    }

    //If the name has a fullstop 4 chars from the back and has no slash in it, im happy with that
}

void CModuleName::TrySetFilePath(PSTR name)
{
    if (stringIndexOf(name, '\\') != -1)
    {
        int length = strlen(name) + 1;
        m_FilePath = static_cast<PSTR>(malloc(length));
        strcpy_s(m_FilePath, length, name);
    }
}

bool CModuleName::MissingNames() const
{
    return m_ShortName == nullptr || m_FileName == nullptr || m_FilePath == nullptr;
}
