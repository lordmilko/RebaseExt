#include "stdafx.h"
#include "CRemoteDebugger.h"
#define TEST
#include "..\RebaseExt\CMappedFile.h"
#include <strsafe.h>
#undef TEST
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace RvaExtTests
{
    enum AddressType
    {
        Loaded,
        Original,
        RVA
    };

    struct Config
    {
        ULONG64 LoadedBase;
        ULONG64 LoadedAddress;
        ULONG64 Rva;
        ULONG64 OriginalBase;
        ULONG64 OriginalAddress;
    };

    TEST_CLASS(RvaExtTests)
    {
    public:
        TEST_METHOD(ResolveSymbol)
        {
            PCSTR expected =
                "\n"
                "RVA: %I64x\n"
                "\n"
                "Loaded Address (notepad.exe)\n"
                "    Base:    %I64x (KERNEL32)\n"
                "    Address: %I64x (KERNEL32!CreateFileW)\n"
                "\n"
                "Original Address (KERNEL32.DLL)\n"
                "    Base:    %I64x (KERNEL32)\n"
                "    Address: %I64x (KERNEL32!CreateFileW)\n"
                "\n";

            AssertExpected("!re kernel32!CreateFileW", expected);
        }

        TEST_METHOD(ResolveLoadedAddress)
        {
            PCSTR expected =
                "\n"
                "RVA: %I64x\n"
                "\n"
                "Loaded Address (notepad.exe)\n"
                "    Base:    %I64x (KERNEL32)\n"
                "    Address: %I64x (KERNEL32!CreateFileW)\n"
                "\n"
                "Original Address (KERNEL32.DLL)\n"
                "    Base:    %I64x (KERNEL32)\n"
                "    Address: %I64x (KERNEL32!CreateFileW)\n"
                "\n";

            AssertAddressExpected("!re %I64x", "kernel32!CreateFileW", Loaded, expected);
        }

        TEST_METHOD(ResolveLoadedAddressWithModuleName)
        {
            PCSTR expected =
                "\n"
                "RVA: %I64x\n"
                "\n"
                "Loaded Address (notepad.exe)\n"
                "    Base:    %I64x (KERNEL32)\n"
                "    Address: %I64x (KERNEL32!CreateFileW)\n"
                "\n"
                "Original Address (KERNEL32.DLL)\n"
                "    Base:    %I64x (KERNEL32)\n"
                "    Address: %I64x (KERNEL32!CreateFileW)\n"
                "\n";

            AssertAddressExpected("!re %I64x kernel32", "kernel32!CreateFileW", Loaded, expected);
        }

        TEST_METHOD(ResolveOriginalAddress)
        {
            PCSTR expected =
                "\n"
                "RVA: %I64x\n"
                "\n"
                "Loaded Address (notepad.exe)\n"
                "    Base:    %I64x (KERNEL32)\n"
                "    Address: %I64x (KERNEL32!CreateFileW)\n"
                "\n"
                "Original Address (KERNEL32.DLL)\n"
                "    Base:    %I64x (KERNEL32)\n"
                "    Address: %I64x (KERNEL32!CreateFileW)\n"
                "\n";

            AssertAddressExpected("!re %I64x kernel32", "kernel32!CreateFileW", Original, expected);
        }

        TEST_METHOD(ResolveRVA)
        {
            PCSTR expected =
                "\n"
                "RVA: %I64x\n"
                "\n"
                "Loaded Address (notepad.exe)\n"
                "    Base:    %I64x (KERNEL32)\n"
                "    Address: %I64x (KERNEL32!CreateFileW)\n"
                "\n"
                "Original Address (KERNEL32.DLL)\n"
                "    Base:    %I64x (KERNEL32)\n"
                "    Address: %I64x (KERNEL32!CreateFileW)\n"
                "\n";

            AssertAddressExpected("!re %I64x kernel32", "kernel32!CreateFileW", RVA, expected);
        }

        TEST_METHOD(ResolveSymbolAtOffset)
        {
            PCSTR expected =
                "\n"
                "RVA: %I64x\n"
                "\n"
                "Loaded Address (notepad.exe)\n"
                "    Base:    %I64x (KERNEL32)\n"
                "    Address: %I64x (KERNEL32!CreateFileW+2)\n"
                "\n"
                "Original Address (KERNEL32.DLL)\n"
                "    Base:    %I64x (KERNEL32)\n"
                "    Address: %I64x (KERNEL32!CreateFileW+2)\n"
                "\n";

            AssertAddressExpected("!re %I64x", "kernel32!CreateFileW+2", Loaded, expected);
        }

        TEST_METHOD(ResolveNoNearSymbol)
        {
            PCSTR expected =
                "\n"
                "RVA: %I64x\n"
                "\n"
                "Loaded Address (notepad.exe)\n"
                "    Base:    %I64x (KERNEL32)\n"
                "    Address: %I64x (No Symbol)\n"
                "\n"
                "Original Address (KERNEL32.DLL)\n"
                "    Base:    %I64x (KERNEL32)\n"
                "    Address: %I64x (No Symbol)\n"
                "\n";

            AssertExpected("!re 9999999 kernel32", expected, RVA);
        }

        TEST_METHOD(InvalidAddress)
        {
            AssertContains("!re 999999999999 kernel32", "Address 0x999999999999 does not exist within module kernel32");
        }

        TEST_METHOD(RvaWithMissingModuleName)
        {
            AssertContains("!re 22ed0", "Could not retrieve module base from address");
        }

        TEST_METHOD(OriginalWithMissingModuleName)
        {
            AssertContains("!re 180022ed0", "Could not retrieve module base from address");
        }

    private:
        static CRemoteDebugger debugger;

        CHAR commandBuffer[200];

        Config GetConfig(PCSTR expr, AddressType addressType, PCSTR moduleName = "kernel32", PCSTR filePath = "C:\\Windows\\system32\\kernel32.dll")
        {
            Config config;

            config.LoadedBase = debugger.Evaluate(moduleName);
            config.OriginalBase = CMappedFile(filePath).GetModuleBaseAddress();

            switch(addressType)
            {
            case Loaded:
                config.LoadedAddress = debugger.Evaluate(expr);
                config.Rva = config.LoadedAddress - config.LoadedBase;
                config.OriginalAddress = config.OriginalBase + config.Rva;
                break;

            case Original:
                config.OriginalAddress = debugger.Evaluate(expr);
                config.Rva = config.OriginalAddress - config.OriginalBase;
                config.LoadedAddress = config.LoadedBase + config.Rva;
                break;
            case RVA:
                config.Rva = debugger.Evaluate(expr);
                config.LoadedAddress = config.LoadedBase + config.Rva;
                config.OriginalAddress = config.OriginalBase + config.Rva;
                break;
            default:
                Assert::Fail(L"Unknown address type");
            }

            return config;
        }

        PSTR GetCommand(PCSTR format, PCSTR expr, AddressType addressType, PCSTR moduleName = "kernel32", PCSTR filePath = "C:\\Windows\\system32\\kernel32.dll")
        {
            ULONG64 loadedBase;
            ULONG64 originalBase;
            ULONG64 rva;
            ULONG64 eval = debugger.Evaluate(expr);

            switch (addressType)
            {
            case Loaded:
                //The expr is the loaded evaluated address. Nothing more to do
                sprintf_s(commandBuffer, format, eval);
                break;
            case Original:
                //The expr is the loaded evaluated address. Get the RVA and original base which then gives us the original address
                loadedBase = debugger.Evaluate(moduleName);
                rva = eval - loadedBase;
                originalBase = CMappedFile(filePath).GetModuleBaseAddress();
                eval = originalBase + rva;
                sprintf_s(commandBuffer, format, eval);
                break;
            case RVA:
                //The the expr is the loaded evaluated address, but we want the RVA
                loadedBase = debugger.Evaluate(moduleName);
                rva = eval - loadedBase;
                sprintf_s(commandBuffer, format, rva);
                break;
            default:
                Assert::Fail(L"Unknown address type");
            }

            return commandBuffer;
        }

        void AssertAddressExpected(
            PCSTR format,
            PCSTR expr,
            AddressType addressType,
            PCSTR rawExpected,
            PCSTR moduleName = "kernel32",
            PCSTR filePath = "C:\\Windows\\system32\\kernel32.dll")
        {
            PSTR command = GetCommand(format, expr, addressType, moduleName, filePath);

            AssertExpected(command, rawExpected, addressType, moduleName, filePath);
        }

        void AssertExpected(
            PCSTR command,
            PCSTR rawExpected,
            AddressType addressType = Loaded,
            PCSTR moduleName = "kernel32",
            PCSTR filePath = "C:\\Windows\\system32\\kernel32.dll")
        {
            Config config = GetConfig(strchr(command, ' ') + 1, addressType, moduleName, filePath);

            std::string actual = debugger.Execute(command);

            CHAR expected[2000];

            sprintf_s(
                expected,
                rawExpected,
                config.Rva,
                config.LoadedBase,
                config.LoadedAddress,
                config.OriginalBase,
                config.OriginalAddress
            );

            Assert::AreEqual(expected, actual.c_str());
        }

        void AssertContains(PCSTR command, PCSTR expected)
        {
            std::string actual = debugger.Execute(command);

            std::wstring wstr(actual.begin(), actual.end());

            Assert::IsTrue(actual.find(expected) != std::string::npos, wstr.c_str());
        }
    };

    CRemoteDebugger RvaExtTests::debugger;
}