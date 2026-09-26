/* The same byte decoder serves global/local character-array initializers. */
char data[] = "\101" "2" "\x42" "C";
int main(void) {
    char local[8] = "\x1" "f" "\12" "3";
    return data[0] + data[1] + data[2] + data[3]
         + local[0] + local[1] + local[2] + local[3] + local[7];
}
