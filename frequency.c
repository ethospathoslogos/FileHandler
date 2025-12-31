#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "mystrtok.h"

#ifdef _MSC_VER
#define strdup _strdup
#endif

/* ---------- hash map: word -> count (separate chaining) ---------- */

typedef struct Entry {
    char *key;
    long count;
    struct Entry *next;
} Entry;

typedef struct {
    Entry **buckets;
    size_t nbuckets;
    size_t nitems;
} FreqMap;

static unsigned long hash_str(const char *s) {
    // djb2
    unsigned long h = 5381;
    unsigned char c;
    while ((c = (unsigned char)*s++)) {
        h = ((h << 5) + h) + c;
    }
    return h;
}

static int map_init(FreqMap *m, size_t nbuckets) {
    m->buckets = (Entry**)calloc(nbuckets, sizeof(Entry*));
    if (!m->buckets) return 0;
    m->nbuckets = nbuckets;
    m->nitems = 0;
    return 1;
}

static void map_free(FreqMap *m) {
    if (!m || !m->buckets) return;
    for (size_t i = 0; i < m->nbuckets; i++) {
        Entry *e = m->buckets[i];
        while (e) {
            Entry *next = e->next;
            free(e->key);
            free(e);
            e = next;
        }
    }
    free(m->buckets);
    m->buckets = NULL;
    m->nbuckets = 0;
    m->nitems = 0;
}

static int map_rehash(FreqMap *m, size_t new_nbuckets) {
    Entry **newb = (Entry**)calloc(new_nbuckets, sizeof(Entry*));
    if (!newb) return 0;

    for (size_t i = 0; i < m->nbuckets; i++) {
        Entry *e = m->buckets[i];
        while (e) {
            Entry *next = e->next;
            unsigned long h = hash_str(e->key);
            size_t idx = h % new_nbuckets;
            e->next = newb[idx];
            newb[idx] = e;
            e = next;
        }
    }

    free(m->buckets);
    m->buckets = newb;
    m->nbuckets = new_nbuckets;
    return 1;
}

static int map_inc(FreqMap *m, const char *key) {
    // Resize when load factor > ~0.75
    if (m->nitems * 4 >= m->nbuckets * 3) {
        if (!map_rehash(m, m->nbuckets ? m->nbuckets * 2 : 1024)) return 0;
    }

    unsigned long h = hash_str(key);
    size_t idx = h % m->nbuckets;

    for (Entry *e = m->buckets[idx]; e; e = e->next) {
        if (strcmp(e->key, key) == 0) {
            e->count++;
            return 1;
        }
    }

    Entry *ne = (Entry*)malloc(sizeof(*ne));
    if (!ne) return 0;

    ne->key = strdup(key);
    if (!ne->key) { free(ne); return 0; }

    ne->count = 1;
    ne->next = m->buckets[idx];
    m->buckets[idx] = ne;
    m->nitems++;
    return 1;
}

/* ---------- token normalization + callback ---------- */

typedef struct {
    FreqMap map;
    int oom;
} FreqCtx;

static void normalize_word(const char *in, char *out, size_t outsz) {
    // lowercase + keep only [a-z0-9'] (tweak if you want)
    size_t j = 0;
    for (size_t i = 0; in[i] && j + 1 < outsz; i++) {
        unsigned char c = (unsigned char)in[i];
        c = (unsigned char)tolower(c);
        if (isalnum(c) || c == '\'') out[j++] = (char)c;
    }
    out[j] = '\0';
}

static void freq_cb(const char *tok, void *ctx) {
    FreqCtx *fc = (FreqCtx*)ctx;
    if (fc->oom) return;

    char word[256];
    normalize_word(tok, word, sizeof(word));
    if (word[0] == '\0') return;

    if (!map_inc(&fc->map, word)) {
        fc->oom = 1;
    }
}

/* ---------- top-10 printing ---------- */

typedef struct {
    const char *key;
    long count;
} Pair;

static int cmp_pair_desc(const void *a, const void *b) {
    const Pair *pa = (const Pair*)a;
    const Pair *pb = (const Pair*)b;
    if (pb->count > pa->count) return 1;
    if (pb->count < pa->count) return -1;
    return strcmp(pa->key, pb->key); // tie-break alphabetical
}

int main(int argc, char **argv) {
    const char *path = (argc >= 2) ? argv[1] : "example.txt";

    FreqCtx fc;
    fc.oom = 0;
    if (!map_init(&fc.map, 2048)) {
        fprintf(stderr, "Failed to init map\n");
        return 1;
    }

    // delimiters: split on whitespace + common punctuation
    const char *delims = " \t\r\n*.()[]/!,;:\"?<>-|\\_+=~`{}";

    if (stream_tokens(path, delims, freq_cb, &fc) != 0) {
        perror("stream_tokens");
        map_free(&fc.map);
        return 1;
    }

    if (fc.oom) {
        fprintf(stderr, "Out of memory while counting frequencies\n");
        map_free(&fc.map);
        return 1;
    }

    // Collect pairs
    Pair *pairs = (Pair*)malloc(fc.map.nitems * sizeof(*pairs));
    if (!pairs) {
        fprintf(stderr, "Out of memory building results\n");
        map_free(&fc.map);
        return 1;
    }

    size_t k = 0;
    for (size_t i = 0; i < fc.map.nbuckets; i++) {
        for (Entry *e = fc.map.buckets[i]; e; e = e->next) {
            pairs[k].key = e->key;
            pairs[k].count = e->count;
            k++;
        }
    }

    qsort(pairs, k, sizeof(*pairs), cmp_pair_desc);

    printf("\nTop 10 words in %s:\n", path);
    size_t top = (k < 10) ? k : 10;
    for (size_t i = 0; i < top; i++) {
        printf("%2zu) %-20s %ld\n", i + 1, pairs[i].key, pairs[i].count);
    }

    free(pairs);
    map_free(&fc.map);
    return 0;
}
