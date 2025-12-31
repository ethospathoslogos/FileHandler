/*mystrtok.h*/
#ifndef MYSTRTOK_H
#define MYSTRTOK_H

#include <stddef.h>

/* Callback invoked once per token */
typedef void (*token_cb)(const char *tok, void *ctx);

/*
 * stream_tokens
 *  - Reads file incrementally (large-file safe)
 *  - Splits on any character in `delims`
 *  - Calls cb(token, ctx) for each token
 *
 * Returns:
 *   0  on success
 *  -1  on error (errno set)
 */
int stream_tokens(const char *path,
                  const char *delims,
                  token_cb cb,
                  void *ctx);

#endif /* TOKENIZER_H */
