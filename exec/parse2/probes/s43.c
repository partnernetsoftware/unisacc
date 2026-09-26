/* Both integer arms use the common type, irrespective of which arm is chosen. */
int pick(int k) {
    char c = 3; unsigned int u = 2;
    int x = (k ? 1 : c) + (k ? c : 1);
    return (x != 4) || ((k ? -1 : u) < 0) || ((!k ? u : -1) < 0);
}
int main(void) { return pick(0) + pick(1); }
