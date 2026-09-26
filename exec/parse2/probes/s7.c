int main(void) { int i; i = 0; do { i = i + 1; if (i == 2) continue; if (i > 5) break; } while (i < 9); while (1) { break; } for (;;) { continue; } return i; }
