#include <stdio.h>
#include <string.h>
int main(void)
{
    char s[] = "  alpha, beta;;gamma  "; char out[32]; char *t; int n = 0;
    out[0] = 0;
    t = strtok(s, " ,;");
    while (t) { n++; strncat(out, t, 3); strncat(out, "-", 5); t = strtok(NULL, " ,;"); }
    printf("%d %s %lu\n", n, out, (unsigned long)strlen(out));
    strncat(out, "xyz", 0);
    printf("%s %d\n", out, strtok(out, "") != NULL);
    return 0;
}
