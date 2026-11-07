const int selector = 0;
int plain(void) { return 0; }
int qualified(void) { return 1; }
int main(void)
{
    return _Generic(selector, int: plain, const int: qualified)();
}
