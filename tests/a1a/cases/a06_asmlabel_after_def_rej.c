int f(void){return 0;}
int f(void) __asm__("late");
int main(void){return f();}
