/* Condition conversions: -0 is false, NaN is true, at both FP widths.
   Each wrong branch exits immediately; no failure can create an endless loop. */
int main(void) {
    double z = 0.0 / (0.0 - 1.0);
    double nan = 0.0 / 0.0;
    float f = (float)z;
    int count = 0;
    if (z) return 1;
    if (f) return 2;
    if (!z == 0) return 3;
    if (!f == 0) return 4;
    while (z) { return 5; }
    for (; f; ) { return 6; }
    do { count = count + 1; } while (z);
    if ((z ? 1 : 2) != 2) return 7;
    if (z || f) return 8;
    if (1 && z) return 9;
    if (z && 1) return 10;
    if ((nan && 1) != 1) return 11;
    if ((0 || nan) != 1) return 12;
    if (!nan) return 13;
    if (count != 1) return 14;
    return 0;
}
