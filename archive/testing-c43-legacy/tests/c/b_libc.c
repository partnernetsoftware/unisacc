/* No #include: the functions we carry in include/ are linked on demand,
   exactly as a real toolchain would find them in libc. */
int main(void) {
    char buf[32];
    char *p;
    strcpy(buf, "abc");
    strcat(buf, "def");
    p = strchr(buf, 'd');
    printf("%s %d %d %d\n", buf, (int) strlen(buf),
           strcmp(buf, "abcdef"), (int) (p - buf));
    memset(buf, 'z', 3);
    printf("%s %d\n", buf, memcmp(buf, "zzz", 3));
    return 0;
}
