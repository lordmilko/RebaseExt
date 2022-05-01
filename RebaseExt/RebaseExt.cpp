#include "RebaseExt.h"
#include "CReCommand.h"

EXT_DECLARE_GLOBALS();

/* Usage:
 *     !re <address> [<module>]
 *     
 *     e.g. !re kernel32!CreateFileW
 */
EXT_COMMAND(re, "Resolve a virtual address to its rebased virtual address and vice versa",
   /* https://docs.microsoft.com/en-us/windows-hardware/drivers/debugger/parsing-extension-arguments
    * 
    * Address (Index 0)
    *     ;                          Argument is unnamed (positional)
    *     e                          Argument is an expression (numeric value). Can be a number, register, symbol, etc
    *     o                          Argument is optional
    *     address                    Display name of the argument. Used in !help
    *     The address to resolve     Description displayed in !help */
    "{;e;address;The address to resolve}"

   /* Module (Index 1)
    *     ;                          Argument is unnamed (positional)
    *     s                          Argument is a string
    *     module                     Display name of the argument. Used in !help
    *     Module of address          Description displayed in !help */
    "{;s,o;module;Module of address}"
)
{
    ULONG opts;
    m_Symbols->GetSymbolOptions(&opts);

    CHAR buff[MAX_PATH];
    ULONG size;

    m_Symbols->GetSymbolPath(buff, MAX_PATH, &size);

    CReCommand command;
    command.Execute();
}