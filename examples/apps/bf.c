/* bf: a Brainfuck interpreter with precomputed bracket jumps,
   running a program that prints a message. */
#include <stdio.h>
#include <string.h>

#define TAPE 30000
#define MAXPROG 4096

static unsigned char tape[TAPE];
static int jump[MAXPROG];

static int run(const char *prog) {
    int len = (int)strlen(prog), stack[256], sp = 0, pc, ptr = 0;
    long steps = 0;
    for (pc = 0; pc < len; pc++) {
        if (prog[pc] == '[') stack[sp++] = pc;
        else if (prog[pc] == ']') {
            int open;
            if (sp == 0) return -1;
            open = stack[--sp];
            jump[open] = pc;
            jump[pc] = open;
        }
    }
    if (sp != 0) return -1;
    for (pc = 0; pc < len; pc++) {
        steps++;
        switch (prog[pc]) {
        case '>': ptr++; break;
        case '<': ptr--; break;
        case '+': tape[ptr]++; break;
        case '-': tape[ptr]--; break;
        case '.': putchar(tape[ptr]); break;
        case '[': if (!tape[ptr]) pc = jump[pc]; break;
        case ']': if (tape[ptr]) pc = jump[pc]; break;
        default: steps--; break;
        }
    }
    printf("(%ld instructions executed)\n", steps);
    return 0;
}

int main(void) {
    /* prints "Hello, unisacc!" and a newline */
    const char *hello =
        "><++++++++[>+++++++++<-]>.<++++++++[>+++<-]>+++++.+++++++..+++.<++++++++"
        "[>--------<-]>---.------------.<++++++++[>++++++++++<-]>+++++.-------.--"
        "---.++++++++++.<++++++++[>--<-]>--.++..<++++++++[>--------<-]>--.<++++++"
        "++[>--<-]>-------.";
    if (run(hello) != 0) printf("unbalanced brackets\n");
    if (run("[[]") != 0) printf("unbalanced brackets rejected\n");
    return 0;
}
