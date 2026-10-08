#include <stdio.h>
/* Compatible struct expressions consume the complete subobject, not its first slot. */
typedef struct { int a, b; } A;
typedef struct { int p; A a; int z; } N;
typedef struct { int head; N n; int end; } Nested;
typedef struct { int p; A a[2]; int z; } MemberArray;
typedef struct { char bytes[259]; int n; } Large;
typedef struct { int p; Large a; int z; } LargeMember;
int main(void) {
    A x = {7, 3};
    Nested nested = {1, {2, x, 4}, 5};
    MemberArray members = {1, {x, x}, 9};
    N designated = {.a = x, .z = 9, .p = 4};
    A list[3] = {x, x, x};
    Large large = {{0}, 31};
    LargeMember big;
    large.bytes[258] = 17;
    big = (LargeMember){2, large, 5};
    printf("%d %d %d %d %d %d\n", nested.head, nested.n.p, nested.n.a.a,
           nested.n.a.b, nested.n.z, nested.end);
    printf("%d %d %d %d\n", members.p, members.a[1].a, members.a[1].b, members.z);
    printf("%d %d %d %d\n", designated.p, designated.a.a, designated.a.b, designated.z);
    printf("%d %d | %d %d %d %d\n", list[2].a, list[2].b,
           big.p, big.a.bytes[258], big.a.n, big.z);
    return 0;
}
