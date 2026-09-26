/* The callable cast shape used by main's in-memory execution entry. */
int add(long a, long b) { return a + b; }
int main(void) {
    long e = (long)add;
    int (*entry)(long, long);
    entry = (int (*)(long, long))e;
    return entry(2, 3) + ((int (*)(long, long))e)(4, 5);
}
