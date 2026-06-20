/* ============================================================
 *  Gampil Programming Language — Symbol Table
 *  symtable.c
 * ============================================================ */

#include "../include/symtable.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ── djb2 hash ──────────────────────────────────────────────── */
static unsigned int hash_str(const char* s) {
    unsigned int h = 5381;
    while (*s) h = ((h << 5) + h) + (unsigned char)(*s++);
    return h % SYM_BUCKET_COUNT;
}

SymTable* symtable_new(void) {
    SymTable* st = (SymTable*)calloc(1, sizeof(SymTable));
    st->depth = -1;
    sym_push_scope(st); /* global scope */
    return st;
}

void sym_push_scope(SymTable* st) {
    st->depth++;
    if (st->depth >= SYM_MAX_SCOPES) {
        fprintf(stderr, "Fatal: scope depth exceeded\n"); exit(1);
    }
    st->scopes[st->depth] = (SymScope*)calloc(1, sizeof(SymScope));
}

void sym_pop_scope(SymTable* st) {
    if (st->depth < 0) return;
    SymScope* scope = st->scopes[st->depth];
    for (int i = 0; i < SYM_BUCKET_COUNT; i++) {
        Symbol* s = scope->buckets[i];
        while (s) {
            Symbol* next = s->next;
            free(s->name);
            free(s);
            s = next;
        }
    }
    free(scope);
    st->scopes[st->depth] = NULL;
    st->depth--;
}

void symtable_free(SymTable* st) {
    while (st->depth >= 0) sym_pop_scope(st);
    free(st);
}

int sym_declare(SymTable* st, const char* name, GampilType type,
                int is_pointer, int array_size, int is_dynamic) {
    SymScope* scope = st->scopes[st->depth];
    unsigned int h  = hash_str(name);
    /* Check redeclaration in current scope */
    for (Symbol* s = scope->buckets[h]; s; s = s->next)
        if (strcmp(s->name, name) == 0) return -1;
    Symbol* sym    = (Symbol*)calloc(1, sizeof(Symbol));
    sym->name       = strdup(name);
    sym->type       = type;
    sym->is_pointer = is_pointer;
    sym->array_size = array_size;
    sym->is_dynamic = is_dynamic;
    sym->next       = scope->buckets[h];
    scope->buckets[h] = sym;
    return 0;
}

int sym_declare_func(SymTable* st, const char* name,
                     GampilType ret_type, AstList* params) {
    SymScope* scope = st->scopes[st->depth];
    unsigned int h  = hash_str(name);
    for (Symbol* s = scope->buckets[h]; s; s = s->next)
        if (strcmp(s->name, name) == 0) return -1;
    Symbol* sym      = (Symbol*)calloc(1, sizeof(Symbol));
    sym->name         = strdup(name);
    sym->type         = ret_type;
    sym->is_function  = 1;
    sym->func_ret_type= ret_type;
    sym->func_params  = params;
    sym->next         = scope->buckets[h];
    scope->buckets[h] = sym;
    return 0;
}

Symbol* sym_lookup(SymTable* st, const char* name) {
    unsigned int h = hash_str(name);
    for (int d = st->depth; d >= 0; d--) {
        SymScope* scope = st->scopes[d];
        if (!scope) continue;
        for (Symbol* s = scope->buckets[h]; s; s = s->next)
            if (strcmp(s->name, name) == 0) return s;
    }
    return NULL;
}
