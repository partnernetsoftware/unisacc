/* examples/win/refused/createfile7.c -- MEASURED FAILURE: two ceilings, not
 * one.
 *
 * There are two of them, and this file is where the second one shows. A
 * forward is generated for at most six arguments (src/fwdstub.c fwd_emit), and
 * a target may deliver fewer: a Windows host call delivers four, so anything
 * from five to six arguments is refused by name with a diagnostic, and seven
 * or more never gets a stub at all and is reported as an undefined function.
 *
 * CreateFileA -- the way every Win32 program obtains a file handle -- takes
 * seven, so handle-based file I/O is out of reach through a forward. It works
 * inside the bundled libc, which is why posix.c can open a file: that call
 * goes through the import table (pe.IMPORTS), which has no argument limit.
 *
 *   out/ua-ref-win.exe -b win/x86_64 -o createfile7.exe examples/win/refused/createfile7.c
 */
#include <stdio.h>

#define GENERIC_READ 80000000UL
#define OPEN_EXISTING 3UL
#define FILE_ATTRIBUTE_NORMAL 128UL
#define INVALID_HANDLE_VALUE 18446744073709551615UL

void *CreateFileA(const char *name, unsigned long access, unsigned long share,
                  void *security, unsigned long disposition,
                  unsigned long flags, void *template_file);
int CloseHandle(void *handle);

int main(void)
{
    void *handle;

    handle = CreateFileA("C:\\Windows\\notepad.exe", GENERIC_READ,
                         0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    printf("handle %d\n", handle != 0 && handle != INVALID_HANDLE_VALUE);
    if (handle != 0) {
        CloseHandle(handle);
    }
    return 0;
}