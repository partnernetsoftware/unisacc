/* wordfreq: count words with a chained hash table, then sort by count. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define NBUCKET 64

struct entry {
    char *word;
    int count;
    struct entry *next;
};

static struct entry *table[NBUCKET];
static int nentries;

static unsigned hash(const char *s) {
    unsigned h = 2166136261u;
    while (*s) { h ^= (unsigned char)*s++; h *= 16777619u; }
    return h;
}

static void add(const char *w) {
    unsigned b = hash(w) % NBUCKET;
    struct entry *e;
    for (e = table[b]; e; e = e->next)
        if (strcmp(e->word, w) == 0) { e->count++; return; }
    e = malloc(sizeof *e);
    e->word = malloc(strlen(w) + 1);
    strcpy(e->word, w);
    e->count = 1;
    e->next = table[b];
    table[b] = e;
    nentries++;
}

static int cmp(const void *a, const void *b) {
    const struct entry *x = *(const struct entry *const *)a;
    const struct entry *y = *(const struct entry *const *)b;
    if (x->count != y->count) return y->count - x->count;
    return strcmp(x->word, y->word);
}

static const char text[] =
    "It was the best of times, it was the worst of times, it was the age of "
    "wisdom, it was the age of foolishness, it was the epoch of belief, it "
    "was the epoch of incredulity, it was the season of Light, it was the "
    "season of Darkness, it was the spring of hope, it was the winter of "
    "despair.";

int main(void) {
    char word[32];
    const char *p = text;
    struct entry **all;
    int i, n = 0, k;
    while (*p) {
        k = 0;
        while (*p && !isalpha((unsigned char)*p)) p++;
        while (isalpha((unsigned char)*p) && k < 31) word[k++] = (char)tolower((unsigned char)*p++);
        word[k] = 0;
        if (k) add(word);
    }
    all = malloc(nentries * sizeof *all);
    for (i = 0; i < NBUCKET; i++) {
        struct entry *e;
        for (e = table[i]; e; e = e->next) all[n++] = e;
    }
    qsort(all, n, sizeof *all, cmp);
    printf("%d distinct words\n", n);
    for (i = 0; i < n && i < 10; i++) printf("%3d  %s\n", all[i]->count, all[i]->word);
    for (i = 0; i < n; i++) { free(all[i]->word); free(all[i]); }
    free(all);
    return 0;
}
