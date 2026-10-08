/* Pointer qualifiers must preserve each unit's private names. */
typedef int I;
static I value = 7;
static const struct { int number; } aggregate = {7};
static const char *const name = "b";
static I *volatile pointer = &value;
static I *restrict alias = &value;
static const char *const names[] = {"b"};
static int get(void) { return *pointer; }
static int (*const choose)(void) = get;
static int helper(void) {
    return choose() + (aggregate.number != value) + (*alias != value) + (name[0] != 'b') + (names[0][0] != 'b');
}
int other(void) { return helper(); }
