#ifndef GAMPIL_SYMTABLE_H
#define GAMPIL_SYMTABLE_H

/* ============================================================
 *  Gampil Programming Language — Symbol Table
 *  symtable.h
 * ============================================================ */

#include "ast.h"

#define SYM_MAX_SCOPES 64
#define SYM_BUCKET_COUNT 128

typedef struct Symbol {
    char*       name;
    GampilType  type;
    int         is_pointer;
    int         array_size;   /* 0 = scalar, >0 = static array    */
    int         is_function;
    GampilType  func_ret_type;
    AstList*    func_params;  /* list of AST_PARAM nodes          */
    int         is_dynamic;   /* 1 = `let` type → Python runtime  */
    struct Symbol* next;      /* chaining in hash bucket          */
} Symbol;

typedef struct SymScope {
    Symbol* buckets[SYM_BUCKET_COUNT];
} SymScope;

typedef struct SymTable {
    SymScope* scopes[SYM_MAX_SCOPES];
    int       depth;          /* current scope depth (0 = global) */
} SymTable;

SymTable* symtable_new(void);
void      symtable_free(SymTable* st);

void      sym_push_scope(SymTable* st);
void      sym_pop_scope(SymTable* st);

/* Returns 0 on success, -1 if already declared in current scope */
int       sym_declare(SymTable* st, const char* name, GampilType type,
                      int is_pointer, int array_size, int is_dynamic);

/* Declare a function symbol */
int       sym_declare_func(SymTable* st, const char* name,
                           GampilType ret_type, AstList* params);

/* Lookup: searches from innermost scope outward; NULL if not found */
Symbol*   sym_lookup(SymTable* st, const char* name);

#endif /* GAMPIL_SYMTABLE_H */
