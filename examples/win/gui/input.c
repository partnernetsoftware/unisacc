/* examples/win/gui/input.c -- MEASURED FAILURE: no hands.
 *
 * The second half of computer use: put a keystroke or a click into the
 * machine. The cheap Win32 routes are keybd_event and mouse_event (four and
 * five arguments, so they fit under the forward's ceiling) and SendInput,
 * which is the one an agent actually wants because it can express a Unicode
 * keystroke and absolute coordinates in a single call.
 *
 * Every one of them lives in user32, which is not one of the four libraries a
 * forward searches. So this probe COMPILES -- prototypes with no definition
 * are a legal forward -- and then fails at run time before the first call,
 * with "unisacc: no host function keybd_event" and exit status 127.
 *
 * That failure mode matters for an agent harness: a missing host function is
 * discovered when the action is attempted, not when the program is built.
 *
 *   out/ua-ref-win.exe -b win/x86_64 -o input.exe examples/win/gui/input.c
 *   ./input.exe
 */
#include <stdio.h>

#define KEYEVENTF_KEYUP 2UL
#define MOUSEEVENTF_MOVE 1UL
#define MOUSEEVENTF_LEFTDOWN 2UL
#define MOUSEEVENTF_LEFTUP 4UL
#define INPUT_MOUSE 0UL
#define INPUT_KEYBOARD 1UL

void keybd_event(unsigned char vk, unsigned char scan, unsigned long flags, unsigned long extra);
void mouse_event(unsigned long flags, int dx, int dy, unsigned long data, unsigned long extra);
int SetCursorPos(int x, int y);
int GetCursorPos(void *point);
unsigned short GetAsyncKeyState(int vkey);

int main(void)
{
    unsigned char point[8];
    int x;
    int y;

    /* Query first: if the query path fails, nothing below it is evidence. */
    if (GetCursorPos(point)) {
        x = (int)point[0] + ((int)point[1] << 8);
        y = (int)point[2] + ((int)point[3] << 8);
        printf("cursor %d,%d\n", x, y);
    } else {
        printf("cursor unavailable\n");
    }

    mouse_event(MOUSEEVENTF_MOVE, 10, 10, 0, 0);
    printf("moved\n");

    keybd_event(0x41, 0, 0, 0);
    keybd_event(0x41, 0, KEYEVENTF_KEYUP, 0);
    printf("typed A\n");

    SetCursorPos(100, 100);
    printf("shift state %u\n", (unsigned int)GetAsyncKeyState(0x10));
    return 0;
}