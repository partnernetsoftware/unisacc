/* Repeated tentative definitions share storage; initialisation is retained. */
int a;
int a;
int a = 7;
int b[3];
int b[3];
int main(void) { b[1] = a; return b[1] - 7; }
