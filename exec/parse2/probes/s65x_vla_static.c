/* Constraint violation: sizeof a VLA is not a static initializer constant. */
int f(int n){int a[n];static unsigned long k=sizeof a;return k;}
int main(void){return f(3);}
