/* Run with -nostdinc: exercise the product's undeclared printf fallback.
   Its width/precision handling and zero return are implementation behaviour,
   not claims about the standard library's printf contract. */
int main(void) {
    printf("%d %i %u %x %X %o %p %s %c %%\n",
           -1, 2, 4294967295u, 31, 31, 31, (void *)31, "ok", 33);
    printf("outer %d %s\n", printf("inner %s\n", "a"), "b");
    printf("escaped \045s / " "\x25" "c / %05.2ld\n", "z", 81, 7L);
    return 0;
}
