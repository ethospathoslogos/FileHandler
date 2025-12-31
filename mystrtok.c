#include "mystrtok.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define BUFF_SIZE 4096

/* Build a fast delimiter lookup table */
static void build_delim_table(const char *delims,
                              unsigned char table[256]) {
    memset(table, 0, 256);
    for (const unsigned char *p = (const unsigned char *)delims; *p; ++p) {
        table[*p] = 1;
    }
}

int stream_tokens(const char *path,
                  const char *delims,
                  token_cb cb,
                  void *ctx) {
    if (!path || !delims || !cb) {
        errno = EINVAL;
        return -1;
    }

    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    unsigned char delim[256];
    build_delim_table(delims, delim);

    unsigned char buf[BUFF_SIZE];

    char *tokbuf = NULL;
    size_t tokcap = 0;
    size_t toklen = 0;

    size_t nread;
    while ((nread = fread(buf, 1, sizeof(buf), f)) > 0) {
        for (size_t i = 0; i < nread; i++) {
            unsigned char c = buf[i];

            if (delim[c]) {
                if (toklen > 0) {
                    if (toklen + 1 > tokcap) {
                        size_t newcap = tokcap ? tokcap * 2 : 128;
                        while (newcap < toklen + 1) newcap *= 2;
                        char *tmp = realloc(tokbuf, newcap);
                        if (!tmp) goto oom;
                        tokbuf = tmp;
                        tokcap = newcap;
                    }
                    tokbuf[toklen] = '\0';
                    cb(tokbuf, ctx);
                    toklen = 0;
                }
            } else {
                if (toklen + 1 > tokcap) {
                    size_t newcap = tokcap ? tokcap * 2 : 128;
                    while (newcap < toklen + 1) newcap *= 2;
                    char *tmp = realloc(tokbuf, newcap);
                    if (!tmp) goto oom;
                    tokbuf = tmp;
                    tokcap = newcap;
                }
                tokbuf[toklen++] = (char)c;
            }
        }
    }

    if (ferror(f)) goto readerr;

    /* flush last token */
    if (toklen > 0) {
        if (toklen + 1 > tokcap) {
            char *tmp = realloc(tokbuf, toklen + 1);
            if (!tmp) goto oom;
            tokbuf = tmp;
            tokcap = toklen + 1;
        }
        tokbuf[toklen] = '\0';
        cb(tokbuf, ctx);
    }

    free(tokbuf);
    fclose(f);
    return 0;

oom:
    errno = ENOMEM;
readerr:
    free(tokbuf);
    fclose(f);
    return -1;
}
