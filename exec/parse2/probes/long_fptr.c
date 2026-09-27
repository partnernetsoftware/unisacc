/* A 64-bit indirect result must not become a 32-bit descriptor. Also the
   callable cast used by the assembly kernel binding, without a named slot. */
long wide(long *p) { return *p + 4294967296L; }
long negative(long *p) { return -*p - 4294967296L; }
int main(void) {
    long n=7;
    long (*fp)(long *)=(long (*)(long *))(long)wide;
    long a=fp(&n);
    long b=((long (*)(long *))(long)negative)(&n);
    if (a!=4294967303L || b!=-4294967303L) return 1;
    if ((fp(&n)>>32)!=1) return 2;
    if ((fp(&n)/4294967296L)!=1) return 3;
    return 0;
}
