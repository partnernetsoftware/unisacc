/* A later unit's function prototype must not reclassify the earlier format. */
int puts(const char *);
void printfpool_first(void);

int main(void) {
    const char *s = "SECOND";
    printfpool_first();
    puts(s);
    return s[0] != 'S';
}
