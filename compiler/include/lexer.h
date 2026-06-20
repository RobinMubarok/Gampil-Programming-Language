#ifndef GAMPIL_LEXER_H
#define GAMPIL_LEXER_H

/* ============================================================
 *  Gampil Programming Language — Lexer API
 *  lexer.h
 * ============================================================ */

#include "token.h"
#include <stddef.h>

typedef struct Lexer {
    const char* source;   /* full source text (null-terminated)   */
    size_t      length;   /* total length of source               */
    size_t      pos;      /* current position                     */
    int         line;     /* current line (1-based)               */
    int         col;      /* current column (1-based)             */
} Lexer;

/* Create a new lexer from source text (not owned, must outlive Lexer) */
Lexer* lexer_new(const char* source);

/* Free lexer (does not free source) */
void   lexer_free(Lexer* l);

/* Return next token (heap-allocates token value when needed) */
Token  lexer_next(Lexer* l);

/* Peek at next token without consuming */
Token  lexer_peek(Lexer* l);

/* Free the value field of a Token (if non-NULL) */
void   token_free(Token t);

/* Read entire file into a heap-allocated string (caller frees) */
char*  read_file(const char* path);

#endif /* GAMPIL_LEXER_H */
