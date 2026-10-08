/* Pointer qualifiers must preserve each unit's private names. */
typedef int I;
static I value = 2;
static const struct { int number; } aggregate = {2};
static const char *const name = "a";
static I *volatile pointer = &value;
static I *restrict alias = &value;
static const char *const names[] = {"a"};
static int get(void) { return *pointer; }
static int (*const choose)(void) = get;
static int helper(void) {
    return choose() + (aggregate.number != value) + (*alias != value) + (name[0] != 'a') + (names[0][0] != 'a');
}
int other(void); int main(void) { return helper() * 10 + other(); }
