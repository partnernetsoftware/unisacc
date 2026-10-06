/* 进 raw mode 读一个键，退出时恢复原状。
 * 注意 #include 必须在文件顶层 —— 放进 #if 里会让两个后端都报
 * "no such file for #include"，把「探针写错」伪装成「能力缺失」。
 * 这条是实测踩到的：第一版把 include 放在函数里，gcc 与 unisacc 同时失败。 */
#include <stdio.h>
#if defined(__linux__) || defined(__APPLE__)
#  include <termios.h>
#  include <unistd.h>
#endif
int main(void) {
#if defined(__linux__) || defined(__APPLE__)
    struct termios old, raw;
    if (tcgetattr(0, &old) != 0) { printf("UNAVAILABLE tcgetattr (not a tty?)\n"); return 1; }
    raw = old;
    raw.c_lflag &= ~(unsigned)(ICANON | ECHO);
    if (tcsetattr(0, TCSANOW, &raw) != 0) { printf("UNAVAILABLE tcsetattr\n"); return 1; }
    char c = 0;
    ssize_t n = read(0, &c, 1);
    tcsetattr(0, TCSANOW, &old);
    if (n != 1) { printf("UNAVAILABLE read (no input)\n"); return 1; }
    printf("ok keycode %d\n", (int)(unsigned char)c);
    return 0;
#else
    printf("UNAVAILABLE raw mode is not POSIX on this target\n");
    return 1;
#endif
}
