extern int ext(int);
int pub(int) __attribute__((alias("ext")));
int main(void){return 0;}
