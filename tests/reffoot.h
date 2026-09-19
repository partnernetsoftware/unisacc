#undef main
int main(int argc, char **argv) {
    G_argc = argc; G_argv = argv;
    return unisacc_main();
}
