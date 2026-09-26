int main(){ short s; short *p; long l; long *q; p = &s; p = p + 2; p = p - 2; *p = 7; q = &l; q = q + 1; q = q - 1; *q = 3; return *p + *q; }
