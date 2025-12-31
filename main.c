#include <stdio.h>
#include <errno.h>
#include "mystrtok.h"

#define BUFF_SIZE 1024

typedef long (*myRead)(const char* path);

typedef struct {
    myRead myread;
    long filebyte;
} StringHandler;

/* prints file to stdout and returns bytes printed */
static long printfile(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        perror("ERROR OPENING FILE");
        return -1;
    }

    unsigned char buf[BUFF_SIZE];
    size_t nread;
    long count = 0;

    while ((nread = fread(buf, 1, sizeof(buf), f)) > 0) {
        size_t nwritten = fwrite(buf, 1, nread, stdout);
        if (nwritten != nread) {
            perror("ERROR WRITING to stdout");
            fclose(f);
            return -1;
        }
        count += (long)nread;
    }

    if (ferror(f)) {
        perror("ERROR READING");
        fclose(f);
        return -1;
    }

    fclose(f);
    return count;
}

/* ---- streaming token callback ---- */
typedef struct {
    long count;
    long print_limit;   // print only first N tokens
} TokStats;

static void on_token(const char *tok, void *ctx) {
    TokStats *s = (TokStats*)ctx;
    s->count++;

    if (s->print_limit > 0 && s->count <= s->print_limit) {
        printf("TOK: %s\n", tok);
    }
}

int main(void) {
    const char *path = "example.txt";

    /* Your handler */
    StringHandler text = {
        .myread = printfile,
        .filebyte = 0
    };

    /* 1) Print file + byte count */
    text.filebyte = text.myread(path);
    if (text.filebyte < 0) return 1;
    printf("\nbytes printed = %ld\n", text.filebyte);

    /* 2) Streaming tokenize (large-file safe) */
    TokStats stats = {.count = 0, .print_limit = 20};

    if (stream_tokens(path, " ,\t\r\n", on_token, &stats) != 0) {
        perror("stream_tokens");
        return 1;
    }

    printf("Total tokens: %ld\n", stats.count);
    return 0;
}
