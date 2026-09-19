/* printf flags, width and precision */
int main() {
    printf("[%5d][%-5d][%05d][%5s][%-5s][%.3s][%c][%3c]\n",
           42, 42, 42, "ab", "ab", "abcdef", 'x', 'y');
    printf("[%d][%s][%ld][%8ld][%-8d|]\n", -7, "hi", 123456789012345L,
           1234L, 9);
    printf("[%1s][%0s][%.0s][%10.2s]\n", "abc", "abc", "abc", "abcdef");
    return 0;
}
