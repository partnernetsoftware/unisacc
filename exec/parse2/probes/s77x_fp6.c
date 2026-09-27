/* Reference limitation: six register arguments leave no callee register.
   This is a valid C program, deliberately excluded from the equal list. */
int six(int a,int b,int c,int d,int e,int f) { return a+b+c+d+e+f; }
int main(void) { int (*p)(int,int,int,int,int,int)=six; return p(1,2,3,4,5,6); }
