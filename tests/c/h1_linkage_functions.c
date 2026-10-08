/* C99 6.2.2: extern and plain functions inherit prior internal linkage. */
static int first(void), second(void);
extern int first(void), second(void);
int first(void) { return 3; }
static int second(void) { return 4; }
static int (*pick(void))(void);
extern int (*pick(void))(void);
static int (*pick(void))(void) { return first; }
int main(void) { extern int first(void); extern int second(void); return first() == 3 && second() == 4 && pick()() == 3 ? 0 : 1; }
