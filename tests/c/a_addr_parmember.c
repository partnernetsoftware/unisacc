union G { struct { int x; } h; };
int *f(void *o) { return &(&((union G *)o)->h)->x; }
int main(void) { union G g; g.h.x=7; return *f(&g); }
