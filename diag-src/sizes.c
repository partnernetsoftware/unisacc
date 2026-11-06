#include <stdio.h>
int f(void){return 1;}
int main(void){
    printf("long=%d ptr=%d ll=%d int=%d size_t=%d\n",(int)sizeof(long),(int)sizeof(void*),(int)sizeof(long long),(int)sizeof(int),(int)sizeof(size_t));
    void *p=(void*)&f;
    long l=(long)p;
    printf("addr=%llx cast=%llx rt=%d\n",(unsigned long long)p,(unsigned long long)(unsigned long)l,((void*)l==p)?1:0);
    return 0;
}
