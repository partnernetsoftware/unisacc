/* dijkstra: shortest paths on a weighted graph with a binary-heap
   priority queue, printing each distance and its path. */
#include <stdio.h>

#define NV 8
#define NE 32
#define INF 1000000

struct edge { int to, w, next; };

static struct edge edges[NE];
static int head[NV], nedges;
static int dist[NV], prev[NV];
static int heap[NE], hkey[NE], hn;

static void add_edge(int a, int b, int w) {
    edges[nedges].to = b; edges[nedges].w = w; edges[nedges].next = head[a]; head[a] = nedges++;
    edges[nedges].to = a; edges[nedges].w = w; edges[nedges].next = head[b]; head[b] = nedges++;
}

static void swap(int i, int j) {
    int t = heap[i]; heap[i] = heap[j]; heap[j] = t;
    t = hkey[i]; hkey[i] = hkey[j]; hkey[j] = t;
}

static void push(int v, int key) {
    int i = hn++;
    heap[i] = v; hkey[i] = key;
    while (i > 0 && hkey[(i - 1) / 2] > hkey[i]) { swap(i, (i - 1) / 2); i = (i - 1) / 2; }
}

static int pop(int *key) {
    int v = heap[0], i = 0;
    *key = hkey[0];
    hn--;
    heap[0] = heap[hn]; hkey[0] = hkey[hn];
    for (;;) {
        int l = 2 * i + 1, r = l + 1, m = i;
        if (l < hn && hkey[l] < hkey[m]) m = l;
        if (r < hn && hkey[r] < hkey[m]) m = r;
        if (m == i) break;
        swap(i, m);
        i = m;
    }
    return v;
}

static void print_path(int v) {
    if (prev[v] >= 0) { print_path(prev[v]); printf(" -> "); }
    printf("%c", 'A' + v);
}

int main(void) {
    int i, v, d, e;
    for (i = 0; i < NV; i++) { head[i] = -1; dist[i] = INF; prev[i] = -1; }
    add_edge(0, 1, 4); add_edge(0, 2, 2); add_edge(1, 2, 5); add_edge(1, 3, 10);
    add_edge(2, 4, 3); add_edge(4, 3, 4); add_edge(3, 5, 11); add_edge(4, 6, 8);
    add_edge(6, 5, 2); add_edge(5, 7, 1); add_edge(6, 7, 6);
    dist[0] = 0;
    push(0, 0);
    while (hn > 0) {
        v = pop(&d);
        if (d > dist[v]) continue;
        for (e = head[v]; e >= 0; e = edges[e].next) {
            int to = edges[e].to, nd = d + edges[e].w;
            if (nd < dist[to]) { dist[to] = nd; prev[to] = v; push(to, nd); }
        }
    }
    for (v = 0; v < NV; v++) {
        printf("%c: %2d  ", 'A' + v, dist[v]);
        print_path(v);
        printf("\n");
    }
    return 0;
}
