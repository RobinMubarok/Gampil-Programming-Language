#ifndef GAMPIL_CODEGEN_H
#define GAMPIL_CODEGEN_H

/* ============================================================
 *  Gampil Programming Language — C Code Generator
 *  codegen.h
 * ============================================================ */

#include "ast.h"
#include "symtable.h"
#include <stdio.h>

typedef struct CodegenCtx {
    FILE*     out;            /* output C file                     */
    SymTable* symtable;
    int       indent;         /* current indentation level         */
    int       tmp_counter;    /* for generating unique temp names  */
    int       in_loop;
    int       had_error;
    char      error_msg[512];
    /* Runtime bridge: collect Python snippets */
    char**    py_snippets;
    int       py_count;
    int       py_cap;
} CodegenCtx;

CodegenCtx* codegen_new(FILE* out);
void        codegen_free(CodegenCtx* ctx);

/* Generate C code from AST; returns 0 on success */
int         codegen_run(CodegenCtx* ctx, AstNode* program);

#endif /* GAMPIL_CODEGEN_H */
