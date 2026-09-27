/* Regression: 12be67c reference returned the wrong result or crashed. */
int main(void){int x=3; return *(int *){&x};}
