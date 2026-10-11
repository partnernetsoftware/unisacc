/* seed/unisa_gold_reloc_label.c -- C99 reference for unisa.gold reloc_label (K6-27), finite-domain samples.
 *
 *   unisa-gold-reloc-label table         "KIND<TAB>ARCH<TAB>LABEL" for every JMPKIND x ARCH key (3 x 2 = 6 rows)
 *   unisa-gold-reloc-label call KIND ARCH one line "LABEL" for that key
 *
 * Mirrors unisa/gold.py reloc_label(k, arch): x86_64 always takes rel32; any other architecture takes arm19 for a
 * conditional jump (jz) and arm26 for everything else (jmp, call).  JMPKIND and ARCH are the tuples of gold.py and
 * catalog.py.  There is no refusal inside the function.  Point-sample equivalence over the finite domain only;
 * not the gold stage closure; not build(); keeps gold.py and catalog.py.
 */
#include <stdio.h>
#include <string.h>

static const char *JMPKIND[] = {"jmp", "jz", "call"};
#define NK ((int)(sizeof JMPKIND / sizeof JMPKIND[0]))
static const char *ARCH[] = {"x86_64", "arm64"};
#define NA ((int)(sizeof ARCH / sizeof ARCH[0]))

/* reloc_label(k, arch) */
static const char *reloc_label(const char *k, const char *arch) {
    if (!strcmp(arch, "x86_64")) return "rel32";
    return !strcmp(k, "jz") ? "arm19" : "arm26";
}

int main(int argc, char **argv) {
    int i, j;
    if (argc == 2 && !strcmp(argv[1], "table")) {
        for (i = 0; i < NK; i++)
            for (j = 0; j < NA; j++)
                printf("%s\t%s\t%s\n", JMPKIND[i], ARCH[j], reloc_label(JMPKIND[i], ARCH[j]));
        return 0;
    }
    if (argc == 4 && !strcmp(argv[1], "call")) {
        printf("%s\n", reloc_label(argv[2], argv[3]));
        return 0;
    }
    fprintf(stderr, "usage: unisa-gold-reloc-label table | call KIND ARCH\n");
    return 2;
}
