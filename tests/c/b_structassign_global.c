/* Whole-struct assignment INTO a file-scope object, static or not, with a
   typedef'd or tagged type, from a local, a global, a parameter and a
   function result.  cJSON 1.7.18's `global_error = local_error;` was refused
   by the reference with "expected ';'" while the product accepted it
   (R16-12 corpus, 2026-10-01): globals of struct type were registered as
   "a name is its address" and never became assignable lvalues. */
#include <stdio.h>
typedef struct { const char *json; unsigned long position; } error;
static error global_error = { "none", 0 };
struct Pair { int x; int y; };
struct Pair gp = { 1, 2 };
struct Pair gq;
static struct Pair make(int a) { struct Pair r; r.x = a; r.y = a * 2; return r; }
static void take(struct Pair p) { gq = p; }
int main(void) {
    error local_error;
    struct Pair l = { 7, 8 };
    local_error.json = "boom"; local_error.position = 42;
    global_error = local_error;                 /* local -> static global */
    printf("%s %lu\n", global_error.json, global_error.position);
    gq = gp;                                    /* global -> global */
    printf("%d %d\n", gq.x, gq.y);
    gp = l;                                     /* local -> global */
    printf("%d %d\n", gp.x, gp.y);
    take(make(5));                              /* parameter -> global */
    printf("%d %d\n", gq.x, gq.y);
    gq = make(9);                               /* result -> global */
    printf("%d %d %d\n", gq.x, gq.y, (gp = gq).y);
    return 0;
}
