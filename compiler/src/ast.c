/* ============================================================
 *  Gampil Programming Language — AST Implementation
 *  ast.c
 * ============================================================ */

#include "../include/ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Type mapping helpers ───────────────────────────────────── */

GampilType tok_to_gtype(TokenType t) {
    switch (t) {
        case TOK_BITOFF: return GTYPE_BITOFF;
        case TOK_BITON:  return GTYPE_BITON;
        case TOK_ASC8:   return GTYPE_ASC8;
        case TOK_ASC16:  return GTYPE_ASC16;
        case TOK_ASC32: return GTYPE_ASC32;
        case TOK_ASC64: return GTYPE_ASC64;
        case TOK_NUM8:  return GTYPE_NUM8;
        case TOK_NUM16: return GTYPE_NUM16;
        case TOK_NUM32:  return GTYPE_NUM32;
        case TOK_NUM64:  return GTYPE_NUM64;
        case TOK_RAT32:  return GTYPE_RAT32;
        case TOK_RAT64:  return GTYPE_RAT64;
        case TOK_RAT128: return GTYPE_RAT128;
        case TOK_FIELD:  return GTYPE_FIELD;
        case TOK_LET:    return GTYPE_DYNAMIC;
        case TOK_REG:    return GTYPE_REG;
        default:         return GTYPE_UNKNOWN;
    }
}

const char* gtype_to_c(GampilType t) {
    switch (t) {
        case GTYPE_BITOFF:  return "void";
        case GTYPE_BITON:   return "int";         /* boolean */
        case GTYPE_ASC8:    return "unsigned char";
        case GTYPE_ASC16:   return "unsigned short";
        case GTYPE_ASC32:  return "unsigned int";
        case GTYPE_ASC64:  return "unsigned long long";
        case GTYPE_NUM8:   return "signed char";
        case GTYPE_NUM16:  return "short";
        case GTYPE_NUM32:   return "long";
        case GTYPE_NUM64:   return "long long";
        case GTYPE_RAT32:   return "float";
        case GTYPE_RAT64:   return "double";
        case GTYPE_RAT128:  return "long double";
        case GTYPE_FIELD:   return "struct";       /* handled specially */
        case GTYPE_DYNAMIC: return "/* dynamic */"; /* Python runtime    */
        case GTYPE_REG:     return "int";          /* register bound type */
        default:            return "int";
    }
}

const char* gtype_name(GampilType t) {
    switch (t) {
        case GTYPE_BITOFF:  return "bitOff";
        case GTYPE_BITON:   return "bitOn";
        case GTYPE_ASC8:    return "asc8";
        case GTYPE_ASC16:   return "asc16";
        case GTYPE_ASC32:   return "asc32";
        case GTYPE_NUM16:   return "num16";
        case GTYPE_NUM32:   return "num32";
        case GTYPE_NUM64:   return "num64";
        case GTYPE_RAT32:   return "rat32";
        case GTYPE_RAT64:   return "rat64";
        case GTYPE_RAT128:  return "rat128";
        case GTYPE_FIELD:   return "field";
        case GTYPE_DYNAMIC: return "let";
        case GTYPE_REG:     return "register";
        default:            return "unknown";
    }
}

int gtype_is_dynamic(GampilType t) { return t == GTYPE_DYNAMIC; }

/* ── AstNode allocation ─────────────────────────────────────── */

AstNode* ast_new(AstKind kind, int line, int col) {
    AstNode* n = (AstNode*)calloc(1, sizeof(AstNode));
    n->kind = kind;
    n->line = line;
    n->col  = col;
    return n;
}

/* ── AstList ────────────────────────────────────────────────── */

AstList* astlist_append(AstList* list, AstNode* node) {
    AstList* item = (AstList*)malloc(sizeof(AstList));
    item->node = node;
    item->next = NULL;
    if (!list) return item;
    AstList* cur = list;
    while (cur->next) cur = cur->next;
    cur->next = item;
    return list;
}

AstList* astlist_prepend(AstList* list, AstNode* node) {
    AstList* item = (AstList*)malloc(sizeof(AstList));
    item->node = node;
    item->next = list;
    return item;
}

int astlist_len(AstList* list) {
    int n = 0;
    for (AstList* c = list; c; c = c->next) n++;
    return n;
}

/* ── Debug printer ──────────────────────────────────────────── */

static void print_indent(int n) { for (int i = 0; i < n*2; i++) putchar(' '); }

void ast_print(AstNode* node, int indent) {
    if (!node) { print_indent(indent); printf("<null>\n"); return; }
    print_indent(indent);
    switch (node->kind) {
        case AST_PROGRAM:
            printf("PROGRAM\n");
            for (AstList* c = node->as.program.decls; c; c = c->next)
                ast_print(c->node, indent+1);
            break;
        case AST_FUNC_DECL:
            printf("FUNC_DECL %s -> %s\n", node->as.func_decl.name,
                   gtype_name(node->as.func_decl.ret_type));
            for (AstList* c = node->as.func_decl.params; c; c = c->next)
                ast_print(c->node, indent+1);
            ast_print(node->as.func_decl.body, indent+1);
            break;
        case AST_VAR_DECL:
            printf("VAR_DECL %s : %s%s%s\n",
                   node->as.var_decl.name,
                   gtype_name(node->as.var_decl.type),
                   node->as.var_decl.is_pointer ? "()" : "",
                   node->as.var_decl.array_size ? "(N)" : "");
            if (node->as.var_decl.initializer)
                ast_print(node->as.var_decl.initializer, indent+1);
            break;
        case AST_PARAM:
            printf("PARAM %s : %s\n", node->as.param.name,
                   gtype_name(node->as.param.type));
            break;
        case AST_IF_STMT:
            printf("IF_STMT\n");
            for (GuardClause* g = node->as.if_stmt.guards; g; g = g->next) {
                print_indent(indent+1);
                printf("GUARD%s\n", g->is_else_and ? " (else and)" : "");
                ast_print(g->cond, indent+2);
                ast_print(g->body, indent+2);
            }
            break;
        case AST_REDO_LOOP:
            printf("REDO_LOOP%s\n", node->as.redo_loop.quite ? " (quite)" : "");
            if (node->as.redo_loop.arrays) {
                print_indent(indent+1); printf("ITER_ARRAYS:\n");
                for (AstList* a = node->as.redo_loop.arrays; a; a = a->next)
                    ast_print(a->node, indent+2);
            }
            if (node->as.redo_loop.iters) {
                print_indent(indent+1); printf("ITERS:\n");
                for (AstList* i = node->as.redo_loop.iters; i; i = i->next)
                    ast_print(i->node, indent+2);
            }
            ast_print(node->as.redo_loop.body, indent+1);
            break;
        case AST_MALLOC_CALL:
            printf("AST_MALLOC_CALL\n");
            ast_print(node->as.malloc_call.size_expr, indent+1);
            break;
        case AST_CAST_EXPR:
            printf("AST_CAST_EXPR target=%s ptr=%d\n", gtype_to_c(node->as.cast_expr.target_type), node->as.cast_expr.is_pointer);
            ast_print(node->as.cast_expr.expr, indent+1);
            break;
        case AST_RETURN_STMT:
            printf("RETURN\n");
            ast_print(node->as.ret.value, indent+1);
            break;
        case AST_STOP_STMT:   printf("STOP\n"); break;
        case AST_BLOCK:
            printf("BLOCK\n");
            for (AstList* c = node->as.block.stmts; c; c = c->next)
                ast_print(c->node, indent+1);
            break;
        case AST_ASSIGN_STMT:
            printf("ASSIGN %s\n", node->as.assign.target);
            ast_print(node->as.assign.value, indent+1);
            break;
        case AST_MULTI_ASSIGN:
            printf("MULTI_ASSIGN\n");
            print_indent(indent+1); printf("TARGETS:\n");
            for (AstList* c = node->as.multi_assign.targets; c; c = c->next)
                ast_print(c->node, indent+2);
            print_indent(indent+1); printf("VALUES:\n");
            for (AstList* c = node->as.multi_assign.values; c; c = c->next)
                ast_print(c->node, indent+2);
            break;
        case AST_BINARY_EXPR:
            printf("BINARY_EXPR op=%s\n", token_type_str(node->as.binary.op));
            ast_print(node->as.binary.left,  indent+1);
            ast_print(node->as.binary.right, indent+1);
            break;
        case AST_UNARY_EXPR:
            printf("UNARY_EXPR op=%s\n", token_type_str(node->as.unary.op));
            ast_print(node->as.unary.operand, indent+1);
            break;
        case AST_CALL_EXPR:
        case AST_PRINTF_CALL:
        case AST_PRINTN_CALL:
        case AST_PRINT_CALL:
            printf("CALL %s\n", node->as.call.callee ? node->as.call.callee : "?");
            for (AstList* c = node->as.call.args; c; c = c->next)
                ast_print(c->node, indent+1);
            break;
        case AST_INDEX_EXPR:
            printf("INDEX\n");
            ast_print(node->as.index.array, indent+1);
            ast_print(node->as.index.index, indent+1);
            break;
        case AST_FIELD_EXPR:
            printf("FIELD .%s\n", node->as.field_access.field);
            ast_print(node->as.field_access.object, indent+1);
            break;
        case AST_IDENT:       printf("IDENT %s\n",  node->as.ident.name); break;
        case AST_INT_LIT:     printf("INT %lld\n",   node->as.int_lit.value); break;
        case AST_FLOAT_LIT:   printf("FLOAT %g\n",  node->as.float_lit.value); break;
        case AST_COMPLEX_LIT: printf("COMPLEX %s\n", node->as.complex_lit.value); break;
        case AST_STR_LIT:     
            printf("STR %s%s%c%s%c%s\n", 
                   node->as.str_lit.prefix ? node->as.str_lit.prefix : "",
                   node->as.str_lit.is_triple ? (node->as.str_lit.delim == '"' ? "\"\"" : "''") : "",
                   node->as.str_lit.delim,
                   node->as.str_lit.value,
                   node->as.str_lit.delim,
                   node->as.str_lit.is_triple ? (node->as.str_lit.delim == '"' ? "\"\"" : "''") : ""); 
            break;
        case AST_BOOL_LIT:    printf("BOOL %s\n",    node->as.bool_lit.value ? "true" : "false"); break;
        case AST_NIL_LIT:     printf("NIL\n"); break;
        case AST_TABLE_LIT:
            printf("TABLE\n");
            for (AstList* c = node->as.table.elements; c; c = c->next)
                ast_print(c->node, indent+1);
            break;
        case AST_ELSE_EXPR:   printf("ELSE\n"); break;
        case AST_ADDR_OF:     printf("ADDR_OF %s\n", node->as.addr_of.var); break;
        case AST_PYRUNTIME_STMT:
            printf("PYRUNTIME: %s\n", node->as.pyruntime.snippet); break;
        default:              printf("AST_NODE(%d)\n", node->kind); break;
    }
}

/* ── Free ────────────────────────────────────────────────────── */

static void astlist_free(AstList* list) {
    while (list) {
        AstList* next = list->next;
        ast_free(list->node);
        free(list);
        list = next;
    }
}

void ast_free(AstNode* node) {
    if (!node) return;
    switch (node->kind) {
        case AST_FUNC_DECL:
            free(node->as.func_decl.name);
            astlist_free(node->as.func_decl.params);
            ast_free(node->as.func_decl.body);
            break;
        case AST_VAR_DECL:
            free(node->as.var_decl.name);
            if (node->as.var_decl.reg_name) free(node->as.var_decl.reg_name);
            ast_free(node->as.var_decl.initializer);
            break;
        case AST_PARAM:
            free(node->as.param.name);
            ast_free(node->as.param.default_val);
            break;
        case AST_PROGRAM:   astlist_free(node->as.program.decls); break;
        case AST_BLOCK:     astlist_free(node->as.block.stmts);   break;
        case AST_IF_STMT: {
            GuardClause* g = node->as.if_stmt.guards;
            while (g) {
                GuardClause* next = g->next;
                ast_free(g->cond);
                ast_free(g->body);
                free(g);
                g = next;
            }
            break;
        }
        case AST_REDO_LOOP:
            astlist_free(node->as.redo_loop.arrays);
            astlist_free(node->as.redo_loop.iters);
            ast_free(node->as.redo_loop.while_cond);
            ast_free(node->as.redo_loop.body);
            break;
        case AST_RETURN_STMT:  ast_free(node->as.ret.value); break;
        case AST_ASSIGN_STMT:
            free(node->as.assign.target);
            ast_free(node->as.assign.value);
            ast_free(node->as.assign.target_index);
            break;
        case AST_MULTI_ASSIGN:
            astlist_free(node->as.multi_assign.targets);
            astlist_free(node->as.multi_assign.values);
            break;
        case AST_BINARY_EXPR:
            ast_free(node->as.binary.left);
            ast_free(node->as.binary.right);
            break;
        case AST_UNARY_EXPR:  ast_free(node->as.unary.operand); break;
        case AST_CALL_EXPR:
        case AST_PRINTF_CALL:
        case AST_PRINTN_CALL:
        case AST_PRINT_CALL:
            free(node->as.call.callee);
            astlist_free(node->as.call.args);
            break;
        case AST_INDEX_EXPR:
            ast_free(node->as.index.array);
            ast_free(node->as.index.index);
            break;
        case AST_FIELD_EXPR:
            free(node->as.field_access.field);
            ast_free(node->as.field_access.object);
            break;
        case AST_IDENT:     free(node->as.ident.name); break;
        case AST_STR_LIT:   
            free(node->as.str_lit.value); 
            if (node->as.str_lit.prefix) free(node->as.str_lit.prefix);
            break;
        case AST_COMPLEX_LIT: free(node->as.complex_lit.value); break;
        case AST_ADDR_OF:   free(node->as.addr_of.var); break;
        case AST_TABLE_LIT: astlist_free(node->as.table.elements); break;
        case AST_PYRUNTIME_STMT: free(node->as.pyruntime.snippet); break;
        case AST_MALLOC_CALL: ast_free(node->as.malloc_call.size_expr); break;
        case AST_CAST_EXPR: ast_free(node->as.cast_expr.expr); break;
        case AST_EXPR_STMT: ast_free(node->as.expr_stmt.expr); break;
        case AST_ELSE_EXPR: break;
        default: break;
    }
    free(node);
}
