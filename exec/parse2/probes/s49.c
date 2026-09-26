/* Automatic and static objects shadow an enum only for their scope. */
enum { A = 7 };
int main(void) {
    int r;
    { int A = 2; r = A; }
    { static int A = 3; r = r * 10 + A; }
    return r * 10 + A;
}
