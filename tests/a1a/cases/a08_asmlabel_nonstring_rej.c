int f(void) __asm__(sym);
int f(void){return 0;}int main(void){return f();}
