/* examples/win/gui/dll.c -- MEASURED: resolution works, calling does not.
 *
 * This is the decisive probe for computer use on Windows, and it separates
 * two questions that are easy to confuse:
 *
 *   1. Can a unisacc program find a user32 function?  YES. LoadLibraryA and
 *      GetProcAddress are kernel32, so "user32.dll" loads and
 *      "EnumWindows"/"SendInput"/"BitBlt" resolve to real addresses. This
 *      probe prints the first bytes at each address so the addresses are
 *      visibly real code and not NULL.
 *
 *   2. Can it CALL through such a pointer?  NO, not today. unisacc's own
 *      generated code uses a private convention -- arguments on a stack,
 *      r9 as the frame pointer, the caller pops -- while user32 expects the
 *      Microsoft x64 convention. Nothing in this compiler translates between
 *      them, which is what archive/plans/v0.0.23.md item A1 (Windows forwarding: IAT,
 *      Win64 hostcall) and A2 (cc interop) are about.
 *
 * So the missing piece for an agent harness is not "reach user32" but "call
 * it with the platform ABI". Everything else -- window lists, screen pixels,
 * injected input -- is a one-line change on top of that.
 *
 *   out/ua-ref-win.exe -b win/x86_64 -o dll.exe examples/win/gui/dll.c
 *   ./dll.exe
 */
#include <stdio.h>

#define BYTES_SHOWN 8

void *LoadLibraryA(const char *name);
void *GetProcAddress(void *module, const char *name);
int FreeLibrary(void *module);
unsigned long GetLastError(void);

static void show(const char *label, void *fn)
{
    unsigned char *p;
    unsigned long i;

    if (fn == 0) {
        printf("%s unresolved\n", label);
        return;
    }
    p = (unsigned char *)fn;
    printf("%s %p first bytes", label, fn);
    for (i = 0; i < BYTES_SHOWN; i = i + 1) {
        printf(" %02lx", (unsigned long)p[i]);
    }
    printf("\n");
}

int main(void)
{
    void *user32;
    void *gdi32;

    user32 = LoadLibraryA("user32.dll");
    show("user32.dll", user32);
    if (user32 == 0) {
        printf("LoadLibraryA failed %lu\n", GetLastError());
        return 1;
    }
    show("EnumWindows", GetProcAddress(user32, "EnumWindows"));
    show("SendInput", GetProcAddress(user32, "SendInput"));
    show("GetWindowTextW", GetProcAddress(user32, "GetWindowTextW"));
    show("keybd_event", GetProcAddress(user32, "keybd_event"));

    gdi32 = LoadLibraryA("gdi32.dll");
    show("gdi32.dll", gdi32);
    if (gdi32 != 0) {
        show("BitBlt", GetProcAddress(gdi32, "BitBlt"));
        show("CreateCompatibleDC", GetProcAddress(gdi32, "CreateCompatibleDC"));
    }

    show("kernel32 GetTickCount", GetProcAddress(0, "GetTickCount"));
    printf("freed %d\n", FreeLibrary(user32));
    return 0;
}