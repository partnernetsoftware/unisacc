/* Comma-separated declarators share one declaration base and shape routines. */
int add_two(int x) { return x + 2; }
int main(void) {
    char rows[2][4], (*p)[4], *q;
    int matrix[2][4], (*mp)[4] = matrix, *tail;
    int head = 0, (*fn)(int) = add_two, ordinary = 31;
    p = rows;
    q = &rows[1][3];
    rows[1][3] = 2;
    mp[1][2] = 19;
    tail = &matrix[1][2];
    if (p[1][3] != 2 || *q != 2) return 1;
    if (matrix[1][2] != 19 || *tail != 19) return 2;
    if (fn(5) != 7 || ordinary != 31 || sizeof ordinary != sizeof(int)) return 3;
    return head;
}
