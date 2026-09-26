int main() {
    unsigned char c;
    unsigned short s;
    unsigned char *p;
    int x;
    c = 200;
    s = 60000;
    p = &c;
    x = c + s;
    x = *p;
    *p = 7;
    c++;
    ++c;
    c += 3;
    s--;
    --s;
    s *= 2;
    x = (int) c;
    if (c != 1) return (0 - 1);
    return x;
}
