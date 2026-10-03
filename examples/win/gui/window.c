/* examples/win/gui/window.c -- MEASURED FAILURE: no window, so no shell.
 *
 * A replacement for explorer.exe has to create a window before anything else
 * matters. This probe is the first step of that and it does not get there:
 * RegisterClassExA takes twelve arguments, and a forward past six arguments
 * needs the libffi bridge, which exists only on macOS. So the refusal here
 * is at COMPILE time, before any DLL question is asked.
 *
 * What it measures: the argument-count ceiling of a Windows forward.
 * Expected: the compiler reports the generated stub's unresolved libffi call.
 *
 *   out/ua-ref-win.exe -b win/x86_64 -o window.exe examples/win/gui/window.c
 */
#include <stdio.h>

struct win_class {
    unsigned int size;
    unsigned long style;
    void *lpfnwndproc;
    int cb_cls_extra;
    int cb_wnd_extra;
    void *hinstance;
    void *hicon;
    void *hcursor;
    void *hbr_background;
    void *lpsz_menu_name;
    void *lpsz_class_name;
    void *hicon_small;
};

int RegisterClassExA(void *wc);
void *CreateWindowExA(unsigned long ex, const char *cls, const char *title,
                      unsigned long style, int x, int y, int w, int h,
                      void *parent, void *menu, void *inst, void *param);
unsigned short GetMessageA(void *msg, void *hwnd, unsigned int min, unsigned int max);

int main(void)
{
    struct win_class wc;
    void *hwnd;

    wc.size = (unsigned int)sizeof(struct win_class);
    RegisterClassExA(&wc);
    hwnd = CreateWindowExA(0, "probe", "probe", 0, 0, 0, 64, 64,
                           0, 0, 0, 0);
    printf("window %d\n", hwnd != 0);
    return 0;
}