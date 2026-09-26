void f(int x){ if (x) return; x = 1; }
int main(void){int i; i=0; f(1); do { i=i+1; if (i==2) continue; if (i>4) break; } while (i<9); while (1) { break; } return i;}
