#pragma once

class CMappedFile
{
public:
    LPVOID m_Map = nullptr;

    //The Throw members on g_Ext don't seem to like being inside a macro for some reason, so we have to #ifdef all the calls as a workaround

    CMappedFile(PCSTR fileName)
    {
        DWORD attrib = GetFileAttributesA(fileName);

        if (attrib == INVALID_FILE_ATTRIBUTES || (attrib & FILE_ATTRIBUTE_DIRECTORY))
        {
#ifdef TEST
            Assert::Fail(L"DLL does not exist");
#else
            g_Ext->ThrowStatus(E_INVALIDARG, "DLL file %s does not exist", fileName);
#endif
        }

        m_File = CreateFileA(
            fileName,
            GENERIC_READ,
            FILE_SHARE_READ,
            nullptr,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );

        if (m_File == INVALID_HANDLE_VALUE)
        {
#ifdef TEST
            Assert::Fail(L"Could not load module for calculating base address");
#else
            g_Ext->ThrowLastError("Could not load module for calculating base address");
#endif
        }

        if (GetFileSize(m_File, nullptr) == INVALID_FILE_SIZE)
        {
#ifdef TEST
            Assert::Fail(L"Could not get module file size for calculating base address");
#else
            g_Ext->ThrowLastError("Could not get module file size for calculating base address");
#endif
        }

        m_FileMapping = CreateFileMapping(
            m_File,
            nullptr,
            PAGE_READONLY,
            0,
            0,
            nullptr
        );

        if (m_FileMapping == nullptr)
        {
#ifdef TEST
            Assert::Fail(L"Could not create file mapping for calculating base address");
#else
            g_Ext->ThrowLastError("Could not create file mapping for calculating base address");
#endif
        }

        m_Map = MapViewOfFile(
            m_FileMapping,
            FILE_MAP_READ,
            0,
            0,
            0
        );

        if (m_Map == nullptr)
        {
#ifdef TEST
            Assert::Fail(L"Could not perform final map view of file for calculating base address");
#else
            g_Ext->ThrowLastError("Could not perform final map view of file for calculating base address");
#endif
        }
    }

    ~CMappedFile()
    {
        FlushViewOfFile(m_Map, 0);
        UnmapViewOfFile(m_Map);
        CloseHandle(m_FileMapping);
        CloseHandle(m_File);
    }

    ULONG64 GetPhysicalAddress(ULONG64 offset) const
    {
        return reinterpret_cast<ULONG64>(m_Map) + offset;
    }

    ULONG64 GetModuleBaseAddress() const
    {
        PIMAGE_DOS_HEADER dosHeader = static_cast<PIMAGE_DOS_HEADER>(m_Map);
        PIMAGE_NT_HEADERS ntHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(GetPhysicalAddress(dosHeader->e_lfanew));

        ULONG64 imageBase = ntHeader->OptionalHeader.ImageBase;

        return imageBase;
    }

private:
    HANDLE m_File = nullptr;
    HANDLE m_FileMapping = nullptr;
};