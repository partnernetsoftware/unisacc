union U { int i; char c[5]; long l; };
int main(void){ union U u; union U *p; u.i = 65; p = &u; return u.c[0] + p->i + (int)sizeof(union U); }
