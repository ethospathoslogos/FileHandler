/*strhndl.c*/

#include <stdio.h>
#include <errno.h>
#include "mystrtok.h"
#include "strhndl.h"
#define BUFF_SIZE 1024



/* prints file to stdout and returns bytes printed */
static long byte_count(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        perror("ERROR OPENING FILE");
        return -1;
    }

    unsigned char buf[BUFF_SIZE];
    size_t nread;
    long count = 0;

    while ((nread = fread(buf, 1, sizeof(buf), f)) > 0) {
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


static void on_token(const char *tok, void *ctx) {
    TokStats *s = (TokStats*)ctx;
    s->count++;

    if (s->print_limit > 0 && s->count <= s->print_limit) {
        printf("token:%s count:%ld\n", tok,s->count);
    }
}
// In strhndl.c
StringHandler get_string_handler(void) {
    StringHandler text = {
        .myread = byte_count,
        .filebyte = 0
    };
    return text; // Returns a COPY to main
}

TokStats get_default_tok_stats(void) {
    TokStats tok = {
        .count = 0,
        .print_limit = 1000,
        .tok_cb = on_token  // This works because both are in the same file
    };
    return tok;
}

