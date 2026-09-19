int main(void) {
#ifdef _WIN32
    printf("host: windows\n");
#elif defined(__APPLE__)
    printf("host: macos\n");
#else
    printf("host: linux\n");
#endif
    return 0;
}
