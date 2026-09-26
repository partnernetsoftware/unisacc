/* queens: count N-queens solutions by backtracking with bit masks,
   and print the first solution for N = 8. */
#include <stdio.h>

static int n, count, first[16], board[16];

static void place(int row, unsigned cols, unsigned d1, unsigned d2) {
    unsigned all = (1u << n) - 1, free_;
    if (row == n) {
        if (count == 0) { int i; for (i = 0; i < n; i++) first[i] = board[i]; }
        count++;
        return;
    }
    free_ = all & ~(cols | d1 | d2);
    while (free_) {
        unsigned bit = free_ & (0u - free_);
        int c = 0;
        while ((1u << c) != bit) c++;
        board[row] = c;
        free_ &= free_ - 1;
        place(row + 1, cols | bit, (d1 | bit) << 1, (d2 | bit) >> 1);
    }
}

int main(void) {
    int r, c;
    for (n = 1; n <= 10; n++) {
        count = 0;
        place(0, 0, 0, 0);
        printf("N = %2d: %4d solutions\n", n, count);
        if (n == 8) {
            for (r = 0; r < n; r++) {
                for (c = 0; c < n; c++) printf(" %c", first[r] == c ? 'Q' : '.');
                printf("\n");
            }
        }
    }
    return 0;
}
