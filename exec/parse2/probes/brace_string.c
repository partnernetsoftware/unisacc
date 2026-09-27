/* C string initialization may have one surrounding brace pair. */
char g[] = {"abc"};
unsigned char ug[7] = {"\101" "b",};
char exact[3] = {"abc"};
unsigned char inferred[] = {"abc",};
char empty[] = {"",};
int main(void) {
    char a[] = {"abc",};
    unsigned char u[] = {"\x61" "b"};
    char sized[8] = {"abc"};
    static char s[] = {"abc"};
    static unsigned char su[6] = {"abc",};
    static unsigned char si[] = {"abc",};
    char plain[] = "abc";
    char aggregate[] = {97,98,99,0};
    return sizeof(inferred)!=4 || inferred[0]!=97 || sizeof(empty)!=1 || empty[0]!=0
        || sizeof(si)!=4 || si[0]!=97 || sizeof(g)!=4 || sizeof(a)!=4 || sizeof(u)!=3 || sizeof(s)!=4
        || g[0]!=97 || g[3]!=0 || a[0]!=97 || a[3]!=0
        || u[0]!=97 || u[2]!=0 || s[0]!=97 || s[3]!=0
        || ug[0]!=65 || ug[6]!=0 || sized[7]!=0 || su[5]!=0
        || exact[2]!=99 || plain[0]!=97 || aggregate[2]!=99;
}
