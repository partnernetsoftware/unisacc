#ifdef __APPLE__
#ifdef __x86_64__
#define NR_write (0x2000000+4)
#else
#define NR_write 4
#endif
#else
#ifdef __x86_64__
#define NR_write 1
#else
#define NR_write 64
#endif
#endif
int main(void){ long r=__syscall6(NR_write,1,(long)"gate ok\n",8,0,0); return r==8 ? 0 : 1; }
