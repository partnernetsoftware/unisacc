int f(void) __asm__("a1");
int f(void) __asm__("a2");
int f(void){return 0;}
int main(void){return f();}
