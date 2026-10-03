/* examples/win/gui/input.c -- input injection, and it works.
 *
 * Before 0.0.23 this probe compiled and then failed at run time with
 * "unisacc: no host function GetCursorPos" and exit 127, because nothing in a
 * forward names a DLL and the resolver only searched ucrtbase, kernel32,
 * ws2_32 and msvcrt. user32 is in the list now, and every call here is at or
 * under four arguments -- which is the whole reason this one works: the
 * Microsoft x64 convention has four integer register slots, and a fifth
 * argument would be dropped in silence (outparam.c measures that, and
 * src/fwdstub.c fwd_maxargs now refuses such a call by name instead).
 *
 * So the reachable half of computer use -- read the cursor, move it, inject a
 * keystroke, read a modifier key -- is available to a unisacc program on
 * Windows, at four arguments or fewer. Window creation and screen capture
 * need the wide call (CreateWindowExA is twelve arguments, BitBlt nine) and
 * stay refused; see gui/window.c and gui/capture.c.
 *
 * The cursor is saved and put back, so running this probe does not disturb
 * the machine it runs on.
 *
 * Build and run:
 *   out/ua-ref-win.exe -b win/x86_64 -o input.exe examples/win/gui/input.c
 *   ./input.exe
 */
#include <stdio.h>

#define KEYEVENTF_KEYUP 2UL
#define MOUSEEVENTF_MOVE 1UL

/* user32, four arguments or fewer: all of these resolve now. */
int GetCursorPos(void *point);
int SetCursorPos(int x, int y);
void mouse_event(unsigned long flags, int dx, int dy, unsigned long data, unsigned long extra);
void keybd_event(unsigned char vk, unsigned char scan, unsigned long flags, unsigned long extra);
unsigned short GetAsyncKeyState(int vkey);
int GetForegroundWindow(void);
unsigned int GetWindowThreadProcessId(void *hwnd, unsigned int *pid);

int main(void)
{
    unsigned char point[8];
    unsigned int pid;
    void *window;
    int x;
    int y;
    int moved;

    /* A POINT is two LONGs; read it at its own offsets, as structs.c says. */
    if (GetCursorPos(point) == 0) {
        printf("GetCursorPos failed\n");
        return 1;
    }
    x = (int)point[0] + ((int)point[1] << 8)
      + ((int)point[2] << 16) + ((int)point[3] << 24);
    y = (int)point[4] + ((int)point[5] << 8)
      + ((int)point[6] << 16) + ((int)point[7] << 24);
    printf("cursor %d,%d\n", x, y);

    /* Which window has focus, and which process owns it: the two facts a
       shell needs before it does anything to the foreground. */
    window = GetForegroundWindow();
    pid = 0;
    if (window != 0) {
        GetWindowThreadProcessId(window, &pid);
    }
    printf("foreground %d pid %u\n", window != 0, pid);

    /* Inject: nudge the cursor one pixel and put it back. */
    moved = SetCursorPos(x + 1, y);
    SetCursorPos(x, y);
    printf("moved %d restored %d\n", moved, SetCursorPos(x, y));

    /* Inject a keystroke. VK_LEFT is the safest key on a live desktop: it
       moves the caret rather than activating anything. */
    keybd_event(0x25, 0, 0, 0);
    keybd_event(0x25, 0, KEYEVENTF_KEYUP, 0);
    printf("key injected\n");

    printf("shift down %d\n", (GetAsyncKeyState(0x10) & 0x8000) != 0);
    return 0;
}