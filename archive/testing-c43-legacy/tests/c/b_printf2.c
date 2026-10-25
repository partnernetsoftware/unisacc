/* printf evaluates every argument before it writes anything */
int fred(int p) { printf("yo %d\n", p); return 42; }
int (*fp)(int) = &fred;
int main() {
    printf("a %d\n", fred(1));
    printf("b %d\n", (*fp)(2));
    /* no probe of the order BETWEEN arguments: C leaves that unspecified
       and gcc and clang disagree.  What is tested here is that no output is
       written until every argument has been evaluated. */
    return 0;
}
