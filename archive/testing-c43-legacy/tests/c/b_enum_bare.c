/* A bare enum tag may be repeated before its definition (F1 regression). */
enum E;
enum E;
enum E { EA = 7 };
int main(void) {
    enum E x = EA;
    return x != 7;
}
