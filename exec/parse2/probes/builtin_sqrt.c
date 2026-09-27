/* Each conversion kind, nested calls, and the carried math header's path. */
int main(void) {
    double d = 9.0;
    float s;
    unsigned long u = 16;
    s = (float)d;
    __builtin_sqrt(s);
    __builtin_sqrtf(s);
    __builtin_sqrt(d);
    __builtin_sqrt(__builtin_sqrtf(d));
    __builtin_sqrt(25);
    __builtin_sqrt(u);
    __builtin_sqrtf(d);
    __builtin_sqrtf(__builtin_sqrtf(d));
    __builtin_sqrtf(25);
    __builtin_sqrtf(u);
    return (int)__builtin_sqrt(__builtin_sqrt(81.0));
}
