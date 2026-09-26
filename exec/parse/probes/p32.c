int f(char *p){ return *p; }
int g(char *p){ *p = 65; return 0; }
int main(){ char c; c = 3; g(&c); return f(&c) - 65; }
