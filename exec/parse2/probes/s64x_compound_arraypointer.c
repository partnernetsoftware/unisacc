/* Reference defect at 12be67c; model must reject, not copy the defect. */
int main(void){int x=3; return *((int *[]){&x})[0];}
