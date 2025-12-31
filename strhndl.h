/*strhndl.h*/
#ifndef STRHNDL_H
#define STRHNDL_H
#include <stddef.h>

typedef long (*myRead)(const char* path);
typedef void (*token_cb) (const char *tok, void *ctx);


typedef struct {
    myRead myread;
    long filebyte;
} StringHandler;

/* ---- streaming token callback ---- */
typedef struct {
    long count;
    long print_limit;   // print only first N tokens
    token_cb tok_cb;
} TokStats;

typedef struct{
    char** strs;
    long len; //number of  tokens
    size_t cap; //capacity
}vec_str;


/* File operations */
StringHandler get_string_handler(void);

/* Token statistics */
TokStats get_default_tok_stats(void);

/* Dynamic token vector */
void vec_init(vec_str *v);
void vec_free(vec_str *v);

/* Token collection callback */
void collect_token_cb(const char *tok, void *ctx);

#endif 