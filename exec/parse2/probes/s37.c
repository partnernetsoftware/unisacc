/* Adjacent literal tokens span lines; escape boundaries must survive. */
char *message = "\x41\x00"
                "\102" "7" "\x1" "f" "\"" "\\";
int main(void) {
    char *p = "a" "b\n" "\x43";
    return message[0] + message[1] + message[2] + message[3]
         + message[4] + message[5] + message[6] + message[7] + p[2];
}
