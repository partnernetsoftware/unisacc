/* examples/win/refused/createfile7.c -- MEASURED FAILURE: the sixth argument
 * is the wall.
 *
 * A forward with at most six integer/pointer arguments is emitted as a
 * direct host call. A seventh argument needs the libffi bridge, and that
 * bridge is compiled only for macOS and Linux. CreateFileA -- the way every
 * Win32 program obtains a file handle -- takes seven, so handle-based file
 * I/O, and everything built on it, is out of reach on Windows today.
 *
 * This is the same wall gui/window.c hits at twelve arguments, and it is the
 * reason a screen-capture pipeline cannot be assembled out of forwards even
 * after the DLL list is widened: GDI'"'"'s own entry points are wide too
 * (BitBlt is nine).
 *
 * Expected: the reference reports an undefined function, because the stub it
 * would need cannot be built.
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