/* R3a (research/c43-r3a/REPORT.txt): an initializer designator whose constant expression defines an enum tag.
   gcc accepts and exits 9.  The product refuses by name ("not covered: enum tag redefinition"): the INITCOUNT
   pre-scan of the brace list (JUMP fpback) parses the sizeof operand once, binding E, and the real pass binds it
   again.  A named refusal, not wrong code; the TDESC refactor (R4) builds descriptors without that rescan. */
static int f(int x) { return x + 1; }
int main(void) {
    int (*fps[])(int) = { [sizeof(enum E { EA = 1, EB })] = f, f };
    return (int)(sizeof fps / sizeof fps[0]) + fps[4](3) - 1;
}
