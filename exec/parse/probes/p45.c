int main() {
    char *p;
    long n;
    p = 0;
    n = __open(p, 0, 0);
    __close(n);
    __lseek(1, 0, 1);
    __unlink(p);
    __rename(p, p);
    __mmap(0, 1, 2);
    __munmap(p, 1);
    __close(1, 2, 3, 4);
    __exit();
    return 0;
}
