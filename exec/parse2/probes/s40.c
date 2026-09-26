/* Global expressions are emitted by EXPR, not folded into a different tape. */
int negative = 0 - 1;
int value = 2 + 3 * 4;
int choice = 0 ? 3 : 7;
enum E { BELOW = -3, ABOVE = BELOW + 8 };
int main(void) { return negative + value + choice + BELOW + ABOVE; }
