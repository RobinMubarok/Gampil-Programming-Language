/* ============================================================
 *  Gampil Programming Language — Semantic Analyzer
 *  semantic.c
 * ============================================================ */

#include "../include/semantic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SemanticCtx* semantic_new(const char* source) {
    SemanticCtx* ctx = (SemanticCtx*)calloc(1, sizeof(SemanticCtx));
    ctx->symtable    = symtable_new();
    ctx->current_ret = GTYPE_VOID;
    ctx->source      = source;
    return ctx;
}

void semantic_free(SemanticCtx* ctx) {
    symtable_free(ctx->symtable);
    free(ctx);
}

static void sem_error(SemanticCtx* ctx, AstNode* n, const char* msg) {
    if (!ctx->had_error) {
        int err_line = n ? n->line : 0;
        int err_col  = n ? n->col  : 0;

        snprintf(ctx->error_msg, sizeof(ctx->error_msg),
                 "Error: %s", msg);
        
        fprintf(stderr, "%s\n", ctx->error_msg);
        
        if (err_line > 0 && ctx->source) {
            fprintf(stderr, "  --> line %d:%d\n", err_line, err_col);
            fprintf(stderr, "   |\n");
            fprintf(stderr, "%3d| ", err_line);
            
            /* Find start of the line */
            const char* src = ctx->source;
            int curr_line = 1;
            while (curr_line < err_line && *src) {
                if (*src == '\n') curr_line++;
                src++;
            }
            
            /* Print the line */
            const char* line_start = src;
            while (*src && *src != '\n') {
                fputc(*src, stderr);
                src++;
            }
            fprintf(stderr, "\n");
            
            /* Print the caret */
            fprintf(stderr, "   | ");
            for (int i = 1; i < err_col; i++) {
                if (line_start[i-1] == '\t') fputc('\t', stderr);
                else fputc(' ', stderr);
            }
            fprintf(stderr, "^ %s\n", msg);
            fprintf(stderr, "   |\n");
        }
        
        ctx->had_error = 1;
    }
}

static void analyze_node(SemanticCtx* ctx, AstNode* n);
static void analyze_block(SemanticCtx* ctx, AstNode* block);

static void analyze_expr(SemanticCtx* ctx, AstNode* n) {
    if (!n) return;
    switch (n->kind) {
        case AST_IDENT: {
            Symbol* s = sym_lookup(ctx->symtable, n->as.ident.name);
            if (!s) {
                char msg[256];
                snprintf(msg, sizeof(msg), "undefined variable '%s'", n->as.ident.name);
                sem_error(ctx, n, msg);
            }
            break;
        }
        case AST_BINARY_EXPR:
            analyze_expr(ctx, n->as.binary.left);
            analyze_expr(ctx, n->as.binary.right);
            break;
        case AST_UNARY_EXPR:
            analyze_expr(ctx, n->as.unary.operand);
            break;
        case AST_INDEX_EXPR:
            analyze_expr(ctx, n->as.index.array);
            analyze_expr(ctx, n->as.index.index);
            break;
        case AST_FIELD_EXPR:
            analyze_expr(ctx, n->as.field_access.object);
            break;
        case AST_CALL_EXPR:
        case AST_PRINTF_CALL:
        case AST_PRINTN_CALL:
        case AST_PRINT_CALL: {
            if (n->as.call.callee && n->kind == AST_CALL_EXPR) {
                Symbol* s = sym_lookup(ctx->symtable, n->as.call.callee);
                if (!s) {
                    char msg[256];
                    snprintf(msg, sizeof(msg), "undefined function '%s'", n->as.call.callee);
                    sem_error(ctx, n, msg);
                }
            }
            for (AstList* a = n->as.call.args; a; a = a->next)
                analyze_expr(ctx, a->node);
            break;
        }
        case AST_ADDR_OF: {
            Symbol* s = sym_lookup(ctx->symtable, n->as.addr_of.var);
            if (!s) {
                char msg[256];
                snprintf(msg, sizeof(msg), "undefined variable '%s' in address-of", n->as.addr_of.var);
                sem_error(ctx, n, msg);
            }
            break;
        }
        case AST_TABLE_LIT:
            for (AstList* e = n->as.table.elements; e; e = e->next)
                analyze_node(ctx, e->node);
            break;
        case AST_MALLOC_CALL:
            analyze_expr(ctx, n->as.malloc_call.size_expr);
            break;
        default: break;
    }
}

static void analyze_node(SemanticCtx* ctx, AstNode* n) {
    if (!n) return;
    switch (n->kind) {
        case AST_FUNC_DECL: {
            /* Register function */
            sym_declare_func(ctx->symtable, n->as.func_decl.name,
                             n->as.func_decl.ret_type, n->as.func_decl.params);
            sym_push_scope(ctx->symtable);
            GampilType prev_ret = ctx->current_ret;
            ctx->current_ret    = n->as.func_decl.ret_type;
            /* Register parameters */
            for (AstList* pa = n->as.func_decl.params; pa; pa = pa->next) {
                AstNode* pm = pa->node;
                sym_declare(ctx->symtable, pm->as.param.name, pm->as.param.type,
                            pm->as.param.is_pointer, 0, gtype_is_dynamic(pm->as.param.type));
            }
            analyze_block(ctx, n->as.func_decl.body);
            ctx->current_ret = prev_ret;
            sym_pop_scope(ctx->symtable);
            break;
        }
        case AST_VAR_DECL: {
            if (n->as.var_decl.type == GTYPE_DYNAMIC) {
                /* Let variable — mark dynamic, no static type check */
                sym_declare(ctx->symtable, n->as.var_decl.name, GTYPE_DYNAMIC, 0, 0, 1);
            } else {
                int r = sym_declare(ctx->symtable, n->as.var_decl.name,
                                    n->as.var_decl.type,
                                    n->as.var_decl.is_pointer,
                                    n->as.var_decl.array_size, 0);
                if (r < 0) {
                    char msg[256];
                    snprintf(msg, sizeof(msg), "redeclaration of '%s'", n->as.var_decl.name);
                    sem_error(ctx, n, msg);
                }
            }
            if (n->as.var_decl.initializer)
                analyze_expr(ctx, n->as.var_decl.initializer);
            break;
        }
        case AST_ASSIGN_STMT: {
            /* For dotted targets (obj.field), just check the base name */
            char base[256]; strncpy(base, n->as.assign.target, sizeof(base)-1);
            char* dot = strchr(base, '.'); if (dot) *dot = '\0';
            Symbol* s = sym_lookup(ctx->symtable, base);
            if (!s) {
                char msg[256];
                snprintf(msg, sizeof(msg), "assignment to undeclared '%s'", n->as.assign.target);
                sem_error(ctx, n, msg);
            }
            analyze_expr(ctx, n->as.assign.value);
            analyze_expr(ctx, n->as.assign.target_index);
            break;
        }
        case AST_IF_STMT:
            for (GuardClause* g = n->as.if_stmt.guards; g; g = g->next) {
                analyze_expr(ctx, g->cond);
                analyze_block(ctx, g->body);
            }
            break;
        case AST_REDO_LOOP: {
            int prev_in = ctx->in_loop;
            ctx->in_loop++;
            sym_push_scope(ctx->symtable);
            analyze_expr(ctx, n->as.redo_loop.array);
            analyze_expr(ctx, n->as.redo_loop.while_cond);
            if (n->as.redo_loop.iter_name) {
                sym_declare(ctx->symtable, n->as.redo_loop.iter_name,
                            n->as.redo_loop.iter_type, 0, 0, 0);
            }
            analyze_block(ctx, n->as.redo_loop.body);
            sym_pop_scope(ctx->symtable);
            ctx->in_loop = prev_in;
            break;
        }
        case AST_STOP_STMT:
            if (ctx->in_loop == 0)
                sem_error(ctx, n, "'stop' used outside of loop");
            break;
        case AST_RETURN_STMT:
            analyze_expr(ctx, n->as.ret.value);
            break;
        case AST_EXPR_STMT:
            analyze_expr(ctx, n->as.expr_stmt.expr);
            break;
        case AST_BLOCK:
            analyze_block(ctx, n);
            break;
        default: break;
    }
}

static void analyze_block(SemanticCtx* ctx, AstNode* block) {
    if (!block) return;
    sym_push_scope(ctx->symtable);
    for (AstList* s = block->as.block.stmts; s; s = s->next)
        analyze_node(ctx, s->node);
    sym_pop_scope(ctx->symtable);
}

int semantic_analyze(SemanticCtx* ctx, AstNode* program) {
    if (!program || program->kind != AST_PROGRAM) return -1;
    /* First pass: register all top-level functions */
    for (AstList* d = program->as.program.decls; d; d = d->next) {
        AstNode* n = d->node;
        if (n && n->kind == AST_FUNC_DECL)
            sym_declare_func(ctx->symtable, n->as.func_decl.name,
                             n->as.func_decl.ret_type, n->as.func_decl.params);
    }
    /* Second pass: full analysis */
    for (AstList* d = program->as.program.decls; d; d = d->next)
        analyze_node(ctx, d->node);
    return ctx->had_error ? -1 : 0;
}
