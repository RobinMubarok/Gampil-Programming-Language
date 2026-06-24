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

static int is_numeric_type(GampilType t) {
    switch (t) {
        case GTYPE_NUM8: case GTYPE_NUM16: case GTYPE_NUM32: case GTYPE_NUM64:
        case GTYPE_ASC8: case GTYPE_ASC16: case GTYPE_ASC32: case GTYPE_ASC64:
        case GTYPE_RAT32: case GTYPE_RAT64: case GTYPE_RAT128:
        case GTYPE_BITON: case GTYPE_BITOFF:
            return 1;
        default: return 0;
    }
}

static GampilType analyze_expr(SemanticCtx* ctx, AstNode* n);
static void analyze_node(SemanticCtx* ctx, AstNode* n);
static void analyze_block(SemanticCtx* ctx, AstNode* block);

static int is_asm_instruction(const char* name) {
    static const char* insts[] = {
        "mov", "add", "sub", "mul", "div", "jmp", "cmp", "je", "jne", "jg", "jge", "jl", "jle",
        "push", "pop", "call", "ret", "int", "nop", "inc", "dec", "xor", "or", "and", "shl", "shr", "lea",
        NULL
    };
    for (int i = 0; insts[i]; i++) {
        if (strcmp(insts[i], name) == 0) return 1;
    }
    return 0;
}

static GampilType analyze_expr(SemanticCtx* ctx, AstNode* n) {
    if (!n) return GTYPE_VOID;
    switch (n->kind) {
        case AST_IDENT: {
            Symbol* s = sym_lookup(ctx->symtable, n->as.ident.name);
            if (!s) {
                char msg[256];
                snprintf(msg, sizeof(msg), "undefined variable '%s'", n->as.ident.name);
                sem_error(ctx, n, msg);
                return GTYPE_UNKNOWN;
            }
            if (!s->is_initialized && !s->is_function) {
                char msg[256];
                snprintf(msg, sizeof(msg), "use of uninitialized variable '%s'", n->as.ident.name);
                sem_error(ctx, n, msg);
            }
            return s->type;
        }
        case AST_INT_LIT:
            if (n->as.int_lit.value >= -2147483648LL && n->as.int_lit.value <= 2147483647LL)
                return GTYPE_NUM32;
            return GTYPE_NUM64;
        case AST_FLOAT_LIT: return GTYPE_RAT64;
        case AST_COMPLEX_LIT: return GTYPE_UNKNOWN;
        case AST_STR_LIT: return GTYPE_ASC8;
        case AST_BOOL_LIT: return GTYPE_BITON;
        case AST_NIL_LIT: return GTYPE_VOID;
        case AST_ELSE_EXPR: return GTYPE_BITON;
        case AST_BINARY_EXPR: {
            GampilType lt = analyze_expr(ctx, n->as.binary.left);
            GampilType rt = analyze_expr(ctx, n->as.binary.right);
            if (lt != rt && lt != GTYPE_DYNAMIC && rt != GTYPE_DYNAMIC && !(is_numeric_type(lt) && is_numeric_type(rt))) {
                char msg[256];
                snprintf(msg, sizeof(msg), "type mismatch in binary expression: %s and %s", gtype_name(lt), gtype_name(rt));
                sem_error(ctx, n, msg);
            }
            return lt;
        }
        case AST_UNARY_EXPR:
            return analyze_expr(ctx, n->as.unary.operand);
        case AST_INDEX_EXPR:
            analyze_expr(ctx, n->as.index.array);
            analyze_expr(ctx, n->as.index.index);
            return GTYPE_DYNAMIC; /* simplification */
        case AST_FIELD_EXPR:
            analyze_expr(ctx, n->as.field_access.object);
            return GTYPE_DYNAMIC;
        case AST_CALL_EXPR:
        case AST_PRINTF_CALL:
        case AST_PRINTN_CALL:
        case AST_PRINT_CALL: {
            GampilType ret = GTYPE_VOID;
            if (n->as.call.callee) {
                if (n->as.call.callee->kind == AST_IDENT) {
                    char* cname = n->as.call.callee->as.ident.name;
                    Symbol* s = sym_lookup(ctx->symtable, cname);
                    if (!s && !is_asm_instruction(cname)) {
                        ret = GTYPE_DYNAMIC;
                    } else if (s) {
                        ret = s->func_ret_type;
                    }
                } else {
                    analyze_expr(ctx, n->as.call.callee);
                    ret = GTYPE_DYNAMIC; /* Higher-order call returns dynamic for now */
                }
            }
            for (AstList* a = n->as.call.args; a; a = a->next)
                analyze_expr(ctx, a->node);
            return ret;
        }
        case AST_GACAST_EXPR:
            analyze_expr(ctx, n->as.py_cast.expr);
            return n->as.py_cast.target_type;
        case AST_PYCAST_EXPR:
            analyze_expr(ctx, n->as.py_cast.expr);
            return GTYPE_DYNAMIC;
        case AST_ADDR_OF: {
            GampilType tgt = analyze_expr(ctx, n->as.addr_of.target);
            /* Addr of should return a pointer type of the target, currently we don't have strict pointer types 
               in analyze_expr return so we just return the base type */
            return tgt;
        }
        case AST_TABLE_LIT:
            for (AstList* e = n->as.table.elements; e; e = e->next)
                analyze_node(ctx, e->node);
            return GTYPE_FIELD;
        case AST_MALLOC_CALL:
            analyze_expr(ctx, n->as.malloc_call.size_expr);
            return GTYPE_VOID;
        default: return GTYPE_UNKNOWN;
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
                            pm->as.param.is_pointer, NULL, 0, gtype_is_dynamic(pm->as.param.type));
                Symbol* s = sym_lookup(ctx->symtable, pm->as.param.name);
                if (s) s->is_initialized = 1;
            }
            analyze_block(ctx, n->as.func_decl.body);
            ctx->current_ret = prev_ret;
            sym_pop_scope(ctx->symtable);
            break;
        }
        case AST_VAR_DECL: {
            if (n->as.var_decl.type == GTYPE_DYNAMIC) {
                /* Let variable — mark dynamic, no static type check */
                sym_declare(ctx->symtable, n->as.var_decl.name, GTYPE_DYNAMIC, 0, NULL, 0, 1);
            } else {
                int r = sym_declare(ctx->symtable, n->as.var_decl.name,
                                    n->as.var_decl.type,
                                    n->as.var_decl.is_pointer,
                                    n->as.var_decl.array_sizes, n->as.var_decl.num_dims, 0);
                if (r < 0) {
                    char msg[256];
                    snprintf(msg, sizeof(msg), "redeclaration of '%s'", n->as.var_decl.name);
                    sem_error(ctx, n, msg);
                }
            }
            if (n->as.var_decl.initializer) {
                GampilType init_type = analyze_expr(ctx, n->as.var_decl.initializer);
                if (n->as.var_decl.type != GTYPE_DYNAMIC && init_type != GTYPE_DYNAMIC && n->as.var_decl.type != init_type && init_type != GTYPE_UNKNOWN && n->as.var_decl.type != GTYPE_FIELD && !(is_numeric_type(n->as.var_decl.type) && is_numeric_type(init_type))) {
                    char msg[256];
                    snprintf(msg, sizeof(msg), "type mismatch in initialization of '%s': expected %s, got %s", n->as.var_decl.name, gtype_name(n->as.var_decl.type), gtype_name(init_type));
                    sem_error(ctx, n, msg);
                }
                Symbol* s = sym_lookup(ctx->symtable, n->as.var_decl.name);
                if (s) s->is_initialized = 1;
            } else if (n->as.var_decl.num_dims > 0 || n->as.var_decl.type == GTYPE_FIELD) {
                Symbol* s = sym_lookup(ctx->symtable, n->as.var_decl.name);
                if (s) s->is_initialized = 1;
            }
            break;
        }
        case AST_ASSIGN_STMT: {
            GampilType target_type = analyze_expr(ctx, n->as.assign.target_expr);
            GampilType val_type = analyze_expr(ctx, n->as.assign.value);

            /* Check if target_expr is a valid l-value */
            int is_lvalue = 0;
            AstNode* t = n->as.assign.target_expr;
            if (t->kind == AST_IDENT) is_lvalue = 1;
            else if (t->kind == AST_INDEX_EXPR) is_lvalue = 1;
            else if (t->kind == AST_FIELD_EXPR) is_lvalue = 1;
            else if (t->kind == AST_CALL_EXPR) is_lvalue = 1; /* In C, macros or returning pointers could act as l-values, we'll allow for flexibility */

            if (!is_lvalue) {
                sem_error(ctx, n, "invalid left value in assignment");
            }

            if (t->kind == AST_IDENT) {
                Symbol* s = sym_lookup(ctx->symtable, t->as.ident.name);
                if (s) s->is_initialized = 1;
            }

            if (target_type != GTYPE_DYNAMIC && val_type != GTYPE_DYNAMIC && target_type != val_type && val_type != GTYPE_UNKNOWN && target_type != GTYPE_FIELD && !(is_numeric_type(target_type) && is_numeric_type(val_type))) {
                char msg[256];
                snprintf(msg, sizeof(msg), "type mismatch in assignment: expected %s, got %s", gtype_name(target_type), gtype_name(val_type));
                sem_error(ctx, n, msg);
            }
            break;
        }
        case AST_MULTI_ASSIGN: {
            for (AstList* v = n->as.multi_assign.values; v; v = v->next)
                analyze_expr(ctx, v->node);
                
            for (AstList* t = n->as.multi_assign.targets; t; t = t->next) {
                AstNode* target = t->node;
                if (target->kind == AST_VAR_DECL) {
                    analyze_node(ctx, target);
                    Symbol* s = sym_lookup(ctx->symtable, target->as.var_decl.name);
                    if (s) s->is_initialized = 1;
                } else if (target->kind == AST_IDENT) {
                    Symbol* s = sym_lookup(ctx->symtable, target->as.ident.name);
                    if (!s) {
                        char msg[256];
                        snprintf(msg, sizeof(msg), "undefined variable '%s'", target->as.ident.name);
                        sem_error(ctx, n, msg);
                    } else {
                        s->is_initialized = 1;
                    }
                }
            }
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
            for (AstList* a = n->as.redo_loop.arrays; a; a = a->next)
                analyze_expr(ctx, a->node);
            analyze_expr(ctx, n->as.redo_loop.while_cond);
            for (AstList* it = n->as.redo_loop.iters; it; it = it->next) {
                AstNode* iter_node = it->node;
                sym_declare(ctx->symtable, iter_node->as.var_decl.name,
                            iter_node->as.var_decl.type,
                            iter_node->as.var_decl.is_pointer,
                            iter_node->as.var_decl.array_sizes, iter_node->as.var_decl.num_dims,
                            gtype_is_dynamic(iter_node->as.var_decl.type));
                Symbol* s = sym_lookup(ctx->symtable, iter_node->as.var_decl.name);
                if (s) s->is_initialized = 1;
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
        case AST_RETURN_STMT: {
            GampilType ret = analyze_expr(ctx, n->as.ret.value);
            if (ctx->current_ret != GTYPE_DYNAMIC && ret != GTYPE_DYNAMIC && ctx->current_ret != ret && ret != GTYPE_UNKNOWN && ctx->current_ret != GTYPE_VOID && !(is_numeric_type(ctx->current_ret) && is_numeric_type(ret))) {
                char msg[256];
                snprintf(msg, sizeof(msg), "type mismatch in return: expected %s, got %s", gtype_name(ctx->current_ret), gtype_name(ret));
                sem_error(ctx, n, msg);
            }
            break;
        }
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
        
    /* Enforce entry function */
    Symbol* algo_sym = sym_lookup(ctx->symtable, "algo");
    if (!algo_sym || !algo_sym->is_function) {
        fprintf(stderr, "Error: missing entry function 'algo'\n");
        ctx->had_error = 1;
    }
    
    return ctx->had_error ? -1 : 0;
}
