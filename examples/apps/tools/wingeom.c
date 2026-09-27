/* wingeom: real on-screen window geometry for winlayout.c, on macOS.
 *
 * This is NOT built by unisacc -- it links CoreGraphics, and unisacc's own
 * C library has no framework bindings (examples/apps/winlayout.c's comment
 * explains why). It is the "target" the owner asked for: prove the real
 * data and the analysis logic work together via a compiler that already
 * has full system access, so unisacc's eventual real-data path (recorded
 * as a gap in prd.md) has a known-correct reference to match.
 *
 * Build and run:
 *   cc -o /tmp/wingeom examples/apps/tools/wingeom.c -framework CoreGraphics
 *   /tmp/wingeom | unisacc -run examples/apps/winlayout.c -
 *
 * CGWindowListCopyWindowInfo returns on-screen windows front-to-back;
 * winlayout.c wants bottom-to-top (topmost last), so this reverses the list.
 * Windows with an empty title are skipped -- most background/menu-bar
 * items report one, and winlayout.c already treats a zero-size window as
 * absent.
 */
#include <stdio.h>
#include <ApplicationServices/ApplicationServices.h>

static double num(CFDictionaryRef d, CFStringRef k)
{
    CFNumberRef n = (CFNumberRef)CFDictionaryGetValue(d, k);
    double v = 0;
    if (n) CFNumberGetValue(n, kCFNumberDoubleType, &v);
    return v;
}

int main(void)
{
    CFArrayRef wins = CGWindowListCopyWindowInfo(
        kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements,
        kCGNullWindowID);
    CFIndex n = CFArrayGetCount(wins), i;
    CGRect screen = CGDisplayBounds(CGMainDisplayID());

    printf("screen %d %d\n", (int)screen.size.width, (int)screen.size.height);
    for (i = n - 1; i >= 0; i--) {
        CFDictionaryRef w = (CFDictionaryRef)CFArrayGetValueAtIndex(wins, i);
        CFDictionaryRef b = (CFDictionaryRef)CFDictionaryGetValue(w, kCGWindowBounds);
        CFStringRef name = (CFStringRef)CFDictionaryGetValue(w, kCGWindowOwnerName);
        char title[128];
        int x, y, ww, hh;
        if (!b || !name) continue;
        if (!CFStringGetCString(name, title, sizeof title, kCFStringEncodingUTF8)) continue;
        if (title[0] == 0) continue;
        x = (int)num(b, CFSTR("X")); y = (int)num(b, CFSTR("Y"));
        ww = (int)num(b, CFSTR("Width")); hh = (int)num(b, CFSTR("Height"));
        if (ww <= 0 || hh <= 0) continue;
        printf("%d %d %d %d %s\n", x, y, ww, hh, title);
    }
    CFRelease(wins);
    return 0;
}
