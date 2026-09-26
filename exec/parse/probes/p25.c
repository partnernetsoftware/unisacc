int main(){int a=3; int *p=&a; int **pp=&p; int **r=pp+1; r=r-1; return **r;}
