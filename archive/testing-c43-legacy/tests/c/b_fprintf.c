/* printf of floating values, exactly: %f %e %g at their rounding ties,
   widths, flags and precisions -- the digits must be the platform libc's. */
int main(void) {
    double a = 12.34, b = 56.78, z = 0.0, big = 1e21, tiny = 1e-7;
    float f = 99.0f;
    printf("%f %f %f %f\n", a + b, a - b, a * b, a / b);
    printf("%.1f %.0f %.0f %.0f %.3f %.10f\n", 0.25, 0.5, 1.5, 2.5, 1.0005, 1.0 / 3);
    printf("%e %E %.2e %e %e\n", a, b, 123456.789, z, tiny);
    printf("%g %g %g %g %g %G %g\n", a, 100000.0, 1000000.0, 0.0001, 0.00001, 1e-10, z);
    printf("%f %g %e\n", big, big, big);
    printf("[%8.3f] [%-8.2f] [%08.2f] [%+.1f] [% .1f] [%+05d] [%05d] [%#g] [%#.0f]\n", a, b, -a, a, a, 42, -42, 2.0, 3.0);
    printf("%f %d %c\n", f, (int)f, (int)f);
    printf("%.17g %.20f\n", 0.1, 0.1);
    return 0;
}
