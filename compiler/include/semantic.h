#ifndef GAMPIL_SEMANTIC_H
#define GAMPIL_SEMANTIC_H

/* ============================================================
 *  Gampil Programming Language — Semantic Analyzer
 *  semantic.h
 * ============================================================ */

#include "ast.h"
#include "symtable.h"

typedef struct SemanticCtx {
    SymTable* symtable;
    int       in_loop;         /* nesting depth of redo loops      */
    GampilType current_ret;   /* return type of current function  */
    int       had_error;
    char      error_msg[512];
    const char* source;
} SemanticCtx;

SemanticCtx* semantic_new(const char* source);
void         semantic_free(SemanticCtx* ctx);

/* Run semantic analysis; returns 0 on success, -1 on error */
int          semantic_analyze(SemanticCtx* ctx, AstNode* program);

#endif /* GAMPIL_SEMANTIC_H */
