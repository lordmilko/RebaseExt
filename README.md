# RebaseExt

RebaseExt is a WinDbg extension that assists in cross referencing between rebased and original addresses.

When reverse engineering a DLL, often you may be stepping through it with WinDbg while cross referencing and annotating in IDA Pro. A major challenge that can arise here however is that WinDbg shows the rebased address of a DLL, while IDA Pro shows the original raw address. You can sometimes find yourself searching for bytes in IDA to find exactly what you're looking at in WinDbg.

RebaseExt solves this problem by allowing you to specify an expression that should be interpreted. Regardless of whether the expression came from WinDbg or IDA Pro, RebaseExt will figure out the meaning and show you where that address can be found both in memory and on disk.

## Installation

To install RebaseExt simply copy `RebaseExt.dll` to the `winext` folder of your WinDbg installation. e.g.

* `x86\RebaseExt.dll` -> `C:\Program Files (x86)\Windows Kits\10\Debuggers\x86\winext`
* `x64\RebaseExt.dll` -> `C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\winext`

RebaseExt can then be utilized within a debugging session by typing `.load rebaseext`

RebaseExt requires the [Visual C++ 2015 Runtime](https://www.microsoft.com/en-au/download/details.aspx?id=48145). You should ensure that both the x86 and x64 versions are installed, so that you can use RebaseExt in both x86 and x64 WinDbg.

## Compilation

RebaseExt requires the following components in order to compile

* Visual Studio 2015 (newer versions may also work)
* Windows 8.1 SDK
* Windows 8.1 x86/x64 Debugging Tools

Note that even if your system has the Windows 10+ x86/x64 Debugging Tools installed, you will still need to install the Windows 8.1 x86/x64 Debugging Tools, as the Windows 10+ version conflicts with the Windows 8.1 SDK headers. The Windows 8.1 x86/x64 Debugging Tools can be installed via the Windows 8.1 SDK installer. If you already have any Debugging Tools installed, the shortcuts in your Start Menu will unfortunately be overwritten.

Once all prerequisites are installed, simply open up `RebaseExt.sln` in Visual Studio and compile.

## Usage

In order to use RebaseExt, it must first be loaded into debugger, which can be done via the command

```c++
.load rebaseext
```

RebaseExt provides a single command `!re`

```c++
!re <address> [<module>]
```

The `<address>` parameter allows specifying any kind of expression that the debugger engine can resolve to an address, be it a symbol, a register, or something more complex. If a `<module>` is not specified, RebaseExt will assume that `<address>` refers to a location within the active process, and will scan all loaded modules to find where it may refer to.

```c++
// Specify a symbol to resolve
0:000> !re kernel32!CreateFileW+2

RVA: 22ed2

Loaded Address (notepad.exe)
    Base:    7ff810760000 (KERNEL32)
    Address: 7ff810782ed2 (KERNEL32!CreateFileW+2)

Original Address (KERNEL32.DLL)
    Base:    180000000 (KERNEL32)
    Address: 180022ed2 (KERNEL32!CreateFileW+2)
```

If the `<address>` that is specified refers to an RVA or a location in the DLL on disk, the `<module>` parameter must be specified so that RebaseExt can figure out what the `<address>` actually means.

```c++
// Specify an address to resolve from kernel32
0:000> !re 180022ed2 kernel32

RVA: 22ed2

Loaded Address (notepad.exe)
    Base:    7ff810760000 (KERNEL32)
    Address: 7ff810782ed2 (KERNEL32!CreateFileW+2)

Original Address (KERNEL32.DLL)
    Base:    180000000 (KERNEL32)
    Address: 180022ed2 (KERNEL32!CreateFileW+2)
```