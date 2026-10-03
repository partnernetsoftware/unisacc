/* examples/win/gui/capture.c -- MEASURED FAILURE: no pixels.
 *
 * Screen capture is the first half of computer use. The shortest possible
 * route is four calls: GetDC(NULL) for the screen, CreateCompatibleDC for a
 * memory device context, CreateCompatibleBitmap for a bitmap, then BitBlt to
 * copy the pixels out. The first lives in user32, the last three in gdi32.
 *
 * BitBlt takes nine arguments, so this probe is expected to fail at compile
 * time on the argument count alone -- which is the smaller half of the
 * problem. The larger half is that neither user32 nor gdi32 is among the
 * four libraries a forward searches (ucrtbase, kernel32, ws2_32, msvcrt), so
 * even a two-argument gdi32 call cannot be resolved at run time. See
 * examples/win/gui/dll.c for the second half, measured directly.
 *
 *   out/ua-ref-win.exe -b win/x86_64 -o capture.exe examples/win/gui/capture.c
 */
#include <stdio.h>

void *GetDC(void *hwnd);
void *CreateCompatibleDC(void *dc);
void *CreateCompatibleBitmap(void *dc, int width, int height);
int BitBlt(void *dst, int x, int y, int w, int h, void *src, int sx, int sy, unsigned long rop);
int GetDeviceCaps(void *dc, int index);

int main(void)
{
    void *screen;
    void *mem;
    void *bitmap;
    int w;
    int h;

    screen = GetDC(0);
    if (screen == 0) {
        printf("no screen dc\n");
        return 1;
    }
    w = GetDeviceCaps(screen, 8);
    h = GetDeviceCaps(screen, 10);
    printf("screen %d x %d\n", w, h);

    mem = CreateCompatibleDC(screen);
    bitmap = CreateCompatibleBitmap(screen, w, h);
    if (mem == 0 || bitmap == 0) {
        printf("no memory dc or bitmap\n");
        return 1;
    }
    printf("blt %d\n", BitBlt(mem, 0, 0, w, h, screen, 0, 0, 0x00CC0020));
    return 0;
}