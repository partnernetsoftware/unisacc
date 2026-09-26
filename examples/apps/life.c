/* life: Conway's Game of Life on a small torus, printed every few steps. */
#include <stdio.h>
#include <string.h>

#define W 20
#define H 10

static char grid[H][W], next[H][W];

static int neighbours(int r, int c) {
    int n = 0, dr, dc;
    for (dr = -1; dr <= 1; dr++)
        for (dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) continue;
            n += grid[(r + dr + H) % H][(c + dc + W) % W];
        }
    return n;
}

static void step(void) {
    int r, c;
    for (r = 0; r < H; r++)
        for (c = 0; c < W; c++) {
            int n = neighbours(r, c);
            next[r][c] = n == 3 || (grid[r][c] && n == 2);
        }
    memcpy(grid, next, sizeof grid);
}

static int population(void) {
    int r, c, n = 0;
    for (r = 0; r < H; r++)
        for (c = 0; c < W; c++) n += grid[r][c];
    return n;
}

static void show(int gen) {
    int r, c;
    printf("generation %d, population %d\n", gen, population());
    for (r = 0; r < H; r++) {
        for (c = 0; c < W; c++) putchar(grid[r][c] ? '#' : '.');
        putchar('\n');
    }
}

int main(void) {
    int gen;
    /* a glider, a blinker and an R-pentomino */
    grid[0][1] = grid[1][2] = grid[2][0] = grid[2][1] = grid[2][2] = 1;
    grid[5][14] = grid[5][15] = grid[5][16] = 1;
    grid[6][7] = grid[6][8] = grid[7][6] = grid[7][7] = grid[8][7] = 1;
    for (gen = 0; gen <= 12; gen++) {
        if (gen % 4 == 0) show(gen);
        step();
    }
    return 0;
}
