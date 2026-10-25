/* `##` pastes TOKENS, not halves of the body; and a macro argument is
   expanded before it is substituted, unless it is an operand of # or ##. */
#define CAT(a,b) a ## b
#define P(A,B) A ## B ; bob
#define Q(A,B) A ## B+
#define STR(x) #x
#define XSTR(x) STR(x)
#define EMPTY
#define VER 3
int xy = 5;
int main(void) {
    int bob, jim = 21;
    bob = P(jim,) *= 2;               /* -> bob = jim ; bob *= 2; */
    printf("%d %d\n", jim, bob);
    jim = 60 Q(+,)3;                  /* -> 60 + +3, not 60 ++3   */
    printf("%d\n", jim);
    printf("%d %s %s\n", CAT(x,y), STR(VER), XSTR(VER));
    printf("%d %d\n", +7, - +2);
    return 0;
}
