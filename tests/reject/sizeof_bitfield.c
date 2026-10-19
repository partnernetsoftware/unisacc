struct S { unsigned x:3; };
int main(void){ struct S s; return sizeof(s.x); }
