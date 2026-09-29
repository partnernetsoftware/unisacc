/* mandel: an ASCII Mandelbrot set, 78 x 32 characters.
 *
 * Pure double arithmetic in nested loops (escape-time iteration, up to 200
 * steps per cell) mapped onto a ten-character palette.  No library calls
 * beyond putchar, so its output is the same bytes on all six targets.
 *
 *   unisacc -run examples/apps/mandel.c
 */
#include <stdio.h>

int main(void)
{
    const char *pal = " .:-=+*#%@";
    int x, y, i, W = 78, H = 32, MAXI = 200;
    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            double cr = -2.2 + 3.0 * x / W;
            double ci = -1.2 + 2.4 * y / H;
            double zr = 0.0, zi = 0.0, t;
            for (i = 0; i < MAXI && zr * zr + zi * zi < 4.0; i++) {
                t = zr * zr - zi * zi + cr;
                zi = 2.0 * zr * zi + ci;
                zr = t;
            }
            putchar(i == MAXI ? '@' : pal[i % 9]);
        }
        putchar('\n');
    }
    return 0;
}
