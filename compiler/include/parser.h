#ifndef GAMPIL_PARSER_H
#define GAMPIL_PARSER_H

/* ============================================================
 *  Gampil Programming Language — Recursive Descent Parser
 *  parser.h
 * ============================================================ */

#include "lexer.h"
#include "ast.h"

typedef struct Parser {
    Lexer*  lexer;
    Token   current;   /* current token (already consumed)  */
    Token   lookahead; /* one token of lookahead            */
    int     had_error;
    char    error_msg[512];
    int     in_table_lit;
} Parser;

/* Create a parser from a lexer */
Parser* parser_new(Lexer* lexer);

/* Parse entire program; returns AST_PROGRAM or NULL on error */
AstNode* parser_parse(Parser* p);

/* Free parser (does NOT free lexer) */
void     parser_free(Parser* p);

#endif /* GAMPIL_PARSER_H */
