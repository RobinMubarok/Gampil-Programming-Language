/* ============================================================
 *  Gampil Programming Language — C Code Generator
 *  codegen.c
 *
 *  Strategy:
 *   - Static types (num, rat, asc, bitOff/On, field) → pure C output
 *   - Dynamic types (`let`) → call embedded Python runtime via
 *     system("python gampil_runtime.py <snippet_file>")
 * ============================================================ */

#include "../include/codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

extern char* g_python_cmd;
extern char* g_runtime_path;

/* ── Internal utilities ─────────────────────────────────────── */

static void emit(CodegenCtx* ctx, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(ctx->out, fmt, ap);
    va_end(ap);
}

static void emit_indent(CodegenCtx* ctx) {
    for (int i = 0; i < ctx->indent; i++) fprintf(ctx->out, "    ");
}

static void emit_line(CodegenCtx* ctx, const char* fmt, ...) {
    emit_indent(ctx);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(ctx->out, fmt, ap);
    va_end(ap);
    fprintf(ctx->out, "\n");
}



/* Forward declarations */
static void gen_node(CodegenCtx* ctx, AstNode* n);
static void gen_expr(CodegenCtx* ctx, AstNode* n);
static void gen_block(CodegenCtx* ctx, AstNode* block);

CodegenCtx* codegen_new(FILE* out) {
    CodegenCtx* ctx = (CodegenCtx*)calloc(1, sizeof(CodegenCtx));
    ctx->out        = out;
    ctx->symtable   = symtable_new();
    return ctx;
}

void codegen_free(CodegenCtx* ctx) {
    symtable_free(ctx->symtable);
    for (int i = 0; i < ctx->py_count; i++) free(ctx->py_snippets[i]);
    free(ctx->py_snippets);
    free(ctx);
}

/* ── Type helpers ───────────────────────────────────────────── */
/* ctype helper (available for future pointer formatting) */

static GampilType infer_type(AstNode* n) {
    if (!n) return GTYPE_VOID;
    switch (n->kind) {
        case AST_INT_LIT: return GTYPE_NUM64;
        case AST_FLOAT_LIT: return GTYPE_RAT64;
        case AST_STR_LIT: return GTYPE_ASC8;
        case AST_BOOL_LIT: return GTYPE_BITON;
        default: return GTYPE_NUM64;
    }
}

/* Generate C type for var_decl or param */
static void emit_type_decl(CodegenCtx* ctx, GampilType t, int is_pointer,
                            int array_size, const char* name, AstNode* decl_node) {
    if (t == GTYPE_DYNAMIC) { emit(ctx, "/* let */"); return; }
    if (t == GTYPE_FIELD)   {
        emit(ctx, "struct { ");
        AstNode* init = (decl_node && decl_node->kind == AST_VAR_DECL) ? decl_node->as.var_decl.initializer : NULL;
        if (init && init->kind == AST_TABLE_LIT) {
            int i = 0;
            for (AstList* e = init->as.table.elements; e; e = e->next, i++) {
                if (e->node->kind == AST_VAR_DECL) {
                    emit(ctx, "%s", gtype_to_c(e->node->as.var_decl.type));
                    for (int p=0; p < e->node->as.var_decl.is_pointer; p++) emit(ctx, "*");
                    if (e->node->as.var_decl.array_size > 0) emit(ctx, " %s[%d]; ", e->node->as.var_decl.name, e->node->as.var_decl.array_size);
                    else emit(ctx, " %s; ", e->node->as.var_decl.name);
                } else {
                    GampilType inferred = infer_type(e->node);
                    emit(ctx, "%s", gtype_to_c(inferred));
                    if (e->node->kind == AST_STR_LIT) emit(ctx, "*");
                    emit(ctx, " _%d; ", i);
                }
            }
        } else if (decl_node && decl_node->kind == AST_VAR_DECL && decl_node->as.var_decl.field_params) {
            int i = 0;
            for (AstList* f = decl_node->as.var_decl.field_params; f; f = f->next, i++) {
                AstNode* fn = f->node;
                emit(ctx, "%s", gtype_to_c(fn->as.var_decl.type));
                for (int p=0; p < fn->as.var_decl.is_pointer; p++) emit(ctx, "*");
                if (fn->as.var_decl.array_size > 0) emit(ctx, " _%d[%d]; ", i, fn->as.var_decl.array_size);
                else emit(ctx, " _%d; ", i);
            }
        } else {
            emit(ctx, "int _raw; ");
        }
        emit(ctx, "}");
        for (int i = 0; i < is_pointer; i++) emit(ctx, "*");
        if (array_size > 0) emit(ctx, " %s[%d]", name, array_size);
        else emit(ctx, " %s", name);
        return;
    }
    emit(ctx, "%s", gtype_to_c(t));
    for (int i = 0; i < is_pointer; i++) emit(ctx, "*");
    
    if (array_size > 0) {
        emit(ctx, " %s[%d]", name, array_size);
    } else {
        emit(ctx, " %s", name);
    }
}

/* ── Python runtime bridge ──────────────────────────────────── */

static void emit_pyruntime_call(CodegenCtx* ctx, const char* snippet) {
    /* Write snippet to a temp file and call python runtime */
    emit_indent(ctx);
    emit(ctx, "{\n");
    ctx->indent++;
    /* Write the python snippet to a temp file at runtime */
    emit_line(ctx, "FILE* _pyf = fopen(\"_gampil_pysnip.py\", \"w\");");
    emit_line(ctx, "if (_pyf) {");
    ctx->indent++;
    /* Escape the snippet for C string */
    emit_indent(ctx);
    emit(ctx, "fprintf(_pyf, \"%%s\", ");
    /* Print snippet as C string literal */
    emit(ctx, "\"");
    for (const char* c = snippet; *c; c++) {
        if (*c == '"')  emit(ctx, "\\\"");
        else if (*c == '\n') emit(ctx, "\\n");
        else if (*c == '\\') emit(ctx, "\\\\");
        else fputc(*c, ctx->out);
    }
    emit(ctx, "\");\n");
    emit_line(ctx, "fclose(_pyf);");
    ctx->indent--;
    emit_line(ctx, "}");
    const char* py_cmd = g_python_cmd ? g_python_cmd : "python";
    const char* rt_path = g_runtime_path ? g_runtime_path : "../runtime/gampil_runtime.py";
    emit_line(ctx, "system(\"%s \\\"%s\\\" _gampil_pysnip.py\");", py_cmd, rt_path);
    ctx->indent--;
    emit_indent(ctx);
    emit(ctx, "}\n");
}

/* ── Expression generation ──────────────────────────────────── */

static void gen_binary_op(CodegenCtx* ctx, TokenType op) {
    switch (op) {
        case TOK_PLUS:    emit(ctx, "+");  break;
        case TOK_MINUS:   emit(ctx, "-");  break;
        case TOK_STAR:    emit(ctx, "*");  break;
        case TOK_SLASH:   emit(ctx, "/");  break;
        case TOK_PERCENT: emit(ctx, "%%"); break;
        case TOK_CARET:   /* ^ in Gampil = power; C has no native, use pow() */
                          /* we'll handle via emit_pow below */
                          emit(ctx, " /* ^ */ "); break;
        case TOK_LT:      emit(ctx, "<");  break;
        case TOK_GT:      emit(ctx, ">");  break;
        case TOK_LTE:     emit(ctx, "<="); break;
        case TOK_GTE:     emit(ctx, ">="); break;
        case TOK_EQ:      emit(ctx, "=="); break;
        case TOK_NEQ:     emit(ctx, "!="); break;
        case TOK_AND:     emit(ctx, "&&"); break;
        case TOK_OR:      emit(ctx, "||"); break;
        case TOK_AMP:     emit(ctx, "&");  break;
        case TOK_PIPE:    emit(ctx, "|");  break;
        case TOK_DPIPE:   emit(ctx, "^");  break; /* XOR */
        case TOK_LSHIFT:  emit(ctx, "<<"); break;
        case TOK_RSHIFT:  emit(ctx, ">>"); break;
        default: emit(ctx, "/*op?*/"); break;
    }
}

static void gen_expr(CodegenCtx* ctx, AstNode* n) {
    if (!n) { emit(ctx, "0"); return; }
    switch (n->kind) {
        case AST_INT_LIT:
            emit(ctx, "%lldLL", n->as.int_lit.value);
            break;
        case AST_FLOAT_LIT:
            emit(ctx, "%g", n->as.float_lit.value);
            break;
        case AST_COMPLEX_LIT: {
            char buf[128];
            strncpy(buf, n->as.complex_lit.value, sizeof(buf)-1);
            buf[sizeof(buf)-1] = '\0';
            int len = strlen(buf);
            if (len > 0 && (buf[len-1] == 'j' || buf[len-1] == 'J')) {
                buf[len-1] = '\0';
            }
            emit(ctx, "(%s * _Complex_I)", buf);
            break;
        }
        case AST_STR_LIT:
            emit(ctx, "\"");
            /* Escape again for C */
            for (const char* c = n->as.str_lit.value; *c; c++) {
                if (*c == '"') emit(ctx, "\\\"");
                else if (*c == '\n') emit(ctx, "\\n");
                else if (*c == '\\') emit(ctx, "\\\\");
                else fputc(*c, ctx->out);
            }
            emit(ctx, "\"");
            break;
        case AST_BOOL_LIT:
            emit(ctx, "%d", n->as.bool_lit.value);
            break;
        case AST_NIL_LIT:
            emit(ctx, "NULL");
            break;
        case AST_IDENT:
            emit(ctx, "%s", n->as.ident.name);
            break;
        case AST_ADDR_OF:
            emit(ctx, "&%s", n->as.addr_of.var);
            break;
        case AST_UNARY_EXPR:
            switch (n->as.unary.op) {
                case TOK_NOT:   emit(ctx, "!"); break;
                case TOK_BANG:  emit(ctx, "!"); break;
                case TOK_TILDE: emit(ctx, "~"); break;
                case TOK_MINUS: emit(ctx, "-"); break;
                default: break;
            }
            emit(ctx, "(");
            gen_expr(ctx, n->as.unary.operand);
            emit(ctx, ")");
            break;
        case AST_BINARY_EXPR:
            if (n->as.binary.op == TOK_CARET) {
                /* Power: pow(left, right) */
                emit(ctx, "(long long)pow((double)(");
                gen_expr(ctx, n->as.binary.left);
                emit(ctx, "), (double)(");
                gen_expr(ctx, n->as.binary.right);
                emit(ctx, "))");
            } else {
                emit(ctx, "(");
                gen_expr(ctx, n->as.binary.left);
                emit(ctx, " ");
                gen_binary_op(ctx, n->as.binary.op);
                emit(ctx, " ");
                gen_expr(ctx, n->as.binary.right);
                emit(ctx, ")");
            }
            break;
        case AST_INDEX_EXPR: {
            int is_field = 0;
            if (n->as.index.array->kind == AST_IDENT) {
                Symbol* s = sym_lookup(ctx->symtable, n->as.index.array->as.ident.name);
                if (s && s->type == GTYPE_FIELD) is_field = 1;
            }
            if (is_field && n->as.index.index->kind == AST_INT_LIT) {
                gen_expr(ctx, n->as.index.array);
                emit(ctx, "._%lld", n->as.index.index->as.int_lit.value);
            } else {
                gen_expr(ctx, n->as.index.array);
                emit(ctx, "[");
                gen_expr(ctx, n->as.index.index);
                emit(ctx, "]");
            }
            break;
        }
        case AST_FIELD_EXPR:
            gen_expr(ctx, n->as.field_access.object);
            emit(ctx, ".%s", n->as.field_access.field);
            break;
        case AST_MALLOC_CALL:
            emit(ctx, "malloc(sizeof(int) * (");
            gen_expr(ctx, n->as.malloc_call.size_expr);
            emit(ctx, "))");
            break;
        case AST_CALL_EXPR: {
            emit(ctx, "%s(", n->as.call.callee ? n->as.call.callee : "unknown");
            
            Symbol* func_sym = NULL;
            if (n->as.call.callee) {
                func_sym = sym_lookup(ctx->symtable, n->as.call.callee);
            }
            AstList* expected_param = func_sym ? func_sym->func_params : NULL;
            AstList* provided_arg = n->as.call.args;
            
            int first = 1;
            while (provided_arg || expected_param) {
                if (!first) emit(ctx, ", ");
                first = 0;
                
                if (provided_arg) {
                    if (provided_arg->node->kind == AST_ASSIGN_STMT)
                        gen_expr(ctx, provided_arg->node->as.assign.value);
                    else
                        gen_expr(ctx, provided_arg->node);
                    provided_arg = provided_arg->next;
                } else {
                    if (expected_param->node && expected_param->node->as.param.default_val) {
                        gen_expr(ctx, expected_param->node->as.param.default_val);
                    } else {
                        emit(ctx, "0");
                    }
                }
                
                if (expected_param) expected_param = expected_param->next;
            }
            emit(ctx, ")");
            break;
        }
        case AST_PRINTF_CALL:
        case AST_PRINTN_CALL:
        case AST_PRINT_CALL: {
            int is_printn = (n->kind == AST_PRINTN_CALL);
            emit(ctx, "printf(");
            int first = 1;
            for (AstList* a = n->as.call.args; a; a = a->next) {
                if (!first) emit(ctx, ", ");
                first = 0;
                if (a->node->kind == AST_ASSIGN_STMT)
                    gen_expr(ctx, a->node->as.assign.value);
                else
                    gen_expr(ctx, a->node);
            }
            if (is_printn) emit(ctx, ", \"\\n\"");
            emit(ctx, ")");
            if (is_printn) {
                /* add newline to format if single string arg */
            }
            break;
        }
        case AST_TABLE_LIT: {
            emit(ctx, "{");
            int first = 1;
            for (AstList* e = n->as.table.elements; e; e = e->next) {
                if (!first) emit(ctx, ", ");
                first = 0;
                if (e->node->kind == AST_VAR_DECL)
                    gen_expr(ctx, e->node->as.var_decl.initializer);
                else
                    gen_expr(ctx, e->node);
            }
            emit(ctx, "}");
            break;
        }
        default:
            emit(ctx, "0 /*unsupported expr %d*/", n->kind);
            break;
    }
}

/* ── Statement generation ───────────────────────────────────── */

static void gen_compound_op(CodegenCtx* ctx, TokenType op) {
    switch (op) {
        case TOK_PLUS_BE:    emit(ctx, "+="); break;
        case TOK_MINUS_BE:   emit(ctx, "-="); break;
        case TOK_STAR_BE:    emit(ctx, "*="); break;
        case TOK_SLASH_BE:   emit(ctx, "/="); break;
        case TOK_PERCENT_BE: emit(ctx, "%%="); break;
        case TOK_AMP_BE:     emit(ctx, "&="); break;
        case TOK_PIPE_BE:    emit(ctx, "|="); break;
        case TOK_LSHIFT_BE:  emit(ctx, "<<="); break;
        case TOK_RSHIFT_BE:  emit(ctx, ">>="); break;
        default:             emit(ctx, "="); break;
    }
}

/* (py_snippet list management reserved for future use) */

static void ast_to_python(AstNode* n, char* buf, size_t maxlen) {
    if (!n) { strncat(buf, "None", maxlen); return; }
    char tmp[256];
    switch (n->kind) {
        case AST_INT_LIT: snprintf(tmp, sizeof(tmp), "%lld", n->as.int_lit.value); strncat(buf, tmp, maxlen); break;
        case AST_FLOAT_LIT: snprintf(tmp, sizeof(tmp), "%g", n->as.float_lit.value); strncat(buf, tmp, maxlen); break;
        case AST_COMPLEX_LIT: snprintf(tmp, sizeof(tmp), "%s", n->as.complex_lit.value); strncat(buf, tmp, maxlen); break;
        case AST_STR_LIT:
            snprintf(tmp, sizeof(tmp), "%s%s%c%s%c%s",
                   n->as.str_lit.prefix ? n->as.str_lit.prefix : "",
                   n->as.str_lit.is_triple ? (n->as.str_lit.delim == '"' ? "\"\"" : "''") : "",
                   n->as.str_lit.delim,
                   n->as.str_lit.value,
                   n->as.str_lit.delim,
                   n->as.str_lit.is_triple ? (n->as.str_lit.delim == '"' ? "\"\"" : "''") : "");
            strncat(buf, tmp, maxlen); break;
        case AST_BOOL_LIT: strncat(buf, n->as.bool_lit.value ? "True" : "False", maxlen); break;
        case AST_NIL_LIT: strncat(buf, "None", maxlen); break;
        case AST_IDENT: strncat(buf, n->as.ident.name, maxlen); break;
        case AST_TABLE_LIT: {
            strncat(buf, "{", maxlen);
            int first = 1;
            for (AstList* e = n->as.table.elements; e; e = e->next) {
                if (!first) strncat(buf, ", ", maxlen);
                first = 0;
                if (e->node->kind == AST_VAR_DECL) {
                    snprintf(tmp, sizeof(tmp), "\"%s\": ", e->node->as.var_decl.name);
                    strncat(buf, tmp, maxlen);
                    ast_to_python(e->node->as.var_decl.initializer, buf, maxlen);
                } else {
                    ast_to_python(e->node, buf, maxlen);
                }
            }
            strncat(buf, "}", maxlen);
            break;
        }
        case AST_UNARY_EXPR:
            if (n->as.unary.op == TOK_NOT || n->as.unary.op == TOK_BANG) strncat(buf, "not ", maxlen);
            else if (n->as.unary.op == TOK_TILDE) strncat(buf, "~", maxlen);
            else if (n->as.unary.op == TOK_MINUS) strncat(buf, "-", maxlen);
            strncat(buf, "(", maxlen); ast_to_python(n->as.unary.operand, buf, maxlen); strncat(buf, ")", maxlen);
            break;
        case AST_BINARY_EXPR:
            strncat(buf, "(", maxlen);
            ast_to_python(n->as.binary.left, buf, maxlen);
            switch (n->as.binary.op) {
                case TOK_PLUS: strncat(buf, " + ", maxlen); break;
                case TOK_MINUS: strncat(buf, " - ", maxlen); break;
                case TOK_STAR: strncat(buf, " * ", maxlen); break;
                case TOK_SLASH: strncat(buf, " / ", maxlen); break;
                case TOK_PERCENT: strncat(buf, " % ", maxlen); break;
                case TOK_CARET: strncat(buf, " ** ", maxlen); break;
                case TOK_EQ: strncat(buf, " == ", maxlen); break;
                case TOK_NEQ: strncat(buf, " != ", maxlen); break;
                case TOK_LT: strncat(buf, " < ", maxlen); break;
                case TOK_GT: strncat(buf, " > ", maxlen); break;
                case TOK_LTE: strncat(buf, " <= ", maxlen); break;
                case TOK_GTE: strncat(buf, " >= ", maxlen); break;
                case TOK_AND: strncat(buf, " and ", maxlen); break;
                case TOK_OR: strncat(buf, " or ", maxlen); break;
                case TOK_AMP: strncat(buf, " & ", maxlen); break;
                case TOK_PIPE: strncat(buf, " | ", maxlen); break;
                case TOK_DPIPE: strncat(buf, " ^ ", maxlen); break;
                case TOK_LSHIFT: strncat(buf, " << ", maxlen); break;
                case TOK_RSHIFT: strncat(buf, " >> ", maxlen); break;
                default: break;
            }
            ast_to_python(n->as.binary.right, buf, maxlen);
            strncat(buf, ")", maxlen);
            break;
        case AST_CALL_EXPR:
        case AST_PRINTF_CALL:
        case AST_PRINTN_CALL:
        case AST_PRINT_CALL: {
            strncat(buf, n->as.call.callee ? n->as.call.callee : "unknown", maxlen);
            strncat(buf, "(", maxlen);
            int first = 1;
            for (AstList* a = n->as.call.args; a; a = a->next) {
                if (!first) strncat(buf, ", ", maxlen);
                first = 0;
                if (a->node->kind == AST_ASSIGN_STMT) {
                    snprintf(tmp, sizeof(tmp), "%s=", a->node->as.assign.target);
                    strncat(buf, tmp, maxlen);
                    ast_to_python(a->node->as.assign.value, buf, maxlen);
                } else {
                    ast_to_python(a->node, buf, maxlen);
                }
            }
            strncat(buf, ")", maxlen);
            break;
        }
        case AST_INDEX_EXPR:
            ast_to_python(n->as.index.array, buf, maxlen);
            strncat(buf, "[", maxlen);
            ast_to_python(n->as.index.index, buf, maxlen);
            strncat(buf, "]", maxlen);
            break;
        case AST_FIELD_EXPR:
            ast_to_python(n->as.field_access.object, buf, maxlen);
            snprintf(tmp, sizeof(tmp), ".%s", n->as.field_access.field);
            strncat(buf, tmp, maxlen);
            break;
        default: strncat(buf, "None", maxlen); break;
    }
}

/* Collect Python snippet from a `let` var_decl */
static char* build_py_let_snippet(AstNode* n) {
    char buf[4096] = {0};
    snprintf(buf, sizeof(buf), "%s = ", n->as.var_decl.name);
    ast_to_python(n->as.var_decl.initializer, buf, sizeof(buf)-1);
    strncat(buf, "\n", sizeof(buf)-1);
    return strdup(buf);
}

static void gen_node(CodegenCtx* ctx, AstNode* n) {
    if (!n) return;

    switch (n->kind) {
        /* ── Variable declaration ─────────────────────────── */
        case AST_VAR_DECL: {
            if (n->as.var_decl.type == GTYPE_DYNAMIC) {
                /* `let` → Python runtime */
                char* snip = build_py_let_snippet(n);
                emit_indent(ctx);
                emit(ctx, "/* let %s: delegated to Python runtime */\n",
                     n->as.var_decl.name);
                emit_pyruntime_call(ctx, snip);
                free(snip);
                sym_declare(ctx->symtable, n->as.var_decl.name,
                            n->as.var_decl.type, n->as.var_decl.is_pointer,
                            n->as.var_decl.array_size, 0);
                break;
            }
            emit_indent(ctx);
            emit_type_decl(ctx, n->as.var_decl.type,
                           n->as.var_decl.is_pointer,
                           n->as.var_decl.array_size,
                           n->as.var_decl.name, n);
            if (n->as.var_decl.initializer) {
                emit(ctx, " = ");
                gen_expr(ctx, n->as.var_decl.initializer);
            }
            emit(ctx, ";\n");
            /* Register in codegen symtable for redo loop size tracking */
            sym_declare(ctx->symtable, n->as.var_decl.name,
                        n->as.var_decl.type, n->as.var_decl.is_pointer,
                        n->as.var_decl.array_size, 0);
            break;
        }

        /* ── Assignment ───────────────────────────────────── */
        case AST_ASSIGN_STMT: {
            /* Check if target is a let (dynamic) variable */
            int is_dyn = 0;
            char base_name[256];
            strncpy(base_name, n->as.assign.target, sizeof(base_name)-1);
            char* dot = strchr(base_name, '.');
            if (dot) *dot = '\0';
            Symbol* target_sym = sym_lookup(ctx->symtable, base_name);
            if (target_sym && target_sym->type == GTYPE_DYNAMIC) is_dyn = 1;

            if (is_dyn) {
                char buf[4096] = {0};
                snprintf(buf, sizeof(buf), "%s ", n->as.assign.target);
                switch (n->as.assign.op) {
                    case TOK_PLUS_BE: strncat(buf, "+= ", sizeof(buf)-1); break;
                    case TOK_MINUS_BE: strncat(buf, "-= ", sizeof(buf)-1); break;
                    case TOK_STAR_BE: strncat(buf, "*= ", sizeof(buf)-1); break;
                    case TOK_SLASH_BE: strncat(buf, "/= ", sizeof(buf)-1); break;
                    case TOK_PERCENT_BE: strncat(buf, "%= ", sizeof(buf)-1); break;
                    default: strncat(buf, "= ", sizeof(buf)-1); break;
                }
                ast_to_python(n->as.assign.value, buf, sizeof(buf)-1);
                strncat(buf, "\n", sizeof(buf)-1);
                emit_pyruntime_call(ctx, buf);
                break;
            }

            emit_indent(ctx);
            if (n->as.assign.target_index) {
                int is_field = 0;
                if (target_sym && target_sym->type == GTYPE_FIELD) is_field = 1;
                
                if (is_field && n->as.assign.target_index->kind == AST_INT_LIT) {
                    emit(ctx, "%s._%lld", n->as.assign.target, n->as.assign.target_index->as.int_lit.value);
                } else {
                    emit(ctx, "%s[", n->as.assign.target);
                    gen_expr(ctx, n->as.assign.target_index);
                    emit(ctx, "]");
                }
            } else {
                emit(ctx, "%s", n->as.assign.target);
            }
            emit(ctx, " ");
            gen_compound_op(ctx, n->as.assign.op);
            emit(ctx, " ");
            gen_expr(ctx, n->as.assign.value);
            emit(ctx, ";\n");
            break;
        }

        /* ── If-statement: Parallel Guard Branching ───────── */
        case AST_IF_STMT: {
            /*
             * Strategy for parallel guard:
             *  1. Evaluate all conditions into bool variables.
             *  2. Check `else and` exclusion: if any `else and` guard is true,
             *     earlier guards with overlapping conditions are suppressed.
             *  3. Run each guard body whose condition was true.
             *
             * For a single guard (common case) → plain if-else chain.
             */
            int num_guards = 0;
            for (GuardClause* g = n->as.if_stmt.guards; g; g = g->next) num_guards++;

            if (num_guards == 1 && !n->as.if_stmt.guards->is_else_and) {
                /* Simple if */
                GuardClause* g = n->as.if_stmt.guards;
                emit_indent(ctx); emit(ctx, "if (");
                gen_expr(ctx, g->cond);
                emit(ctx, ") {\n");
                ctx->indent++;
                gen_block(ctx, g->body);
                ctx->indent--;
                emit_indent(ctx); emit(ctx, "}\n");
                break;
            }

            /* Multi-guard: evaluate conditions first */
            emit_indent(ctx); emit(ctx, "/* --- Parallel Guard Block --- */\n");
            emit_indent(ctx); emit(ctx, "{\n");
            ctx->indent++;

            /* Step 1: evaluate each condition into a temp int */
            int gi = 0;
            for (GuardClause* g = n->as.if_stmt.guards; g; g = g->next, gi++) {
                emit_line(ctx, "int _guard%d = (", gi);
                if (g->cond) gen_expr(ctx, g->cond);
                else emit(ctx, "1");
                emit(ctx, ");");
                /* Add newline after the semi on same line — fix: emit inline */
                emit(ctx, "\n");
            }

            /* Step 2: resolve `else and` exclusions */
            gi = 0;
            for (GuardClause* g = n->as.if_stmt.guards; g; g = g->next, gi++) {
                if (g->is_else_and) {
                    /* If this guard is true, suppress all earlier guards */
                    emit_indent(ctx);
                    emit(ctx, "if (_guard%d) { ", gi);
                    for (int j = 0; j < gi; j++)
                        emit(ctx, "_guard%d = 0; ", j);
                    emit(ctx, "}\n");
                }
            }

            /* Step 3: run each guard body whose flag is still set */
            gi = 0;
            for (GuardClause* g = n->as.if_stmt.guards; g; g = g->next, gi++) {
                emit_indent(ctx); emit(ctx, "if (_guard%d) {\n", gi);
                ctx->indent++;
                gen_block(ctx, g->body);
                ctx->indent--;
                emit_indent(ctx); emit(ctx, "}\n");
            }

            ctx->indent--;
            emit_indent(ctx); emit(ctx, "} /* --- end Guard Block --- */\n");
            break;
        }

        /* ── Redo loop ────────────────────────────────────── */
        case AST_REDO_LOOP: {
            /* Case 1: while-sugar  redo: while cond */
            if (n->as.redo_loop.while_cond && !n->as.redo_loop.array) {
                emit_indent(ctx); emit(ctx, "while (");
                gen_expr(ctx, n->as.redo_loop.while_cond);
                emit(ctx, ") {\n");
                ctx->indent++;
                gen_block(ctx, n->as.redo_loop.body);
                ctx->indent--;
                emit_indent(ctx); emit(ctx, "}\n");
                break;
            }

            /* Case 2: infinite  redo: ... ok */
            if (!n->as.redo_loop.array) {
                emit_indent(ctx); emit(ctx, "while (1) {\n");
                ctx->indent++;
                gen_block(ctx, n->as.redo_loop.body);
                ctx->indent--;
                emit_indent(ctx); emit(ctx, "}\n");
                break;
            }

            /* Case 3: foreach  redo arr [quite] as T i: */
            /* Determine array length:
             *  - If arr is an AST_IDENT we look up in symtable for array_size
             *  - Otherwise we use a runtime sizeof trick (only for static arrays)
             */
            char arr_name[256] = "unknown";
            int  arr_size = 0;
            if (n->as.redo_loop.array->kind == AST_IDENT) {
                strncpy(arr_name, n->as.redo_loop.array->as.ident.name,
                        sizeof(arr_name)-1);
                Symbol* s = sym_lookup(ctx->symtable, arr_name);
                if (s) arr_size = s->array_size;
            }

            const char* iter_ctype = gtype_to_c(n->as.redo_loop.iter_type);
            const char* iter_name  = n->as.redo_loop.iter_name
                                     ? n->as.redo_loop.iter_name : "_it";

            emit_indent(ctx);
            if (arr_size > 0) {
                emit(ctx, "for (int _idx_%s = 0; _idx_%s < %d; _idx_%s++) {\n",
                     iter_name, iter_name, arr_size, iter_name);
            } else {
                /* Dynamic — use sizeof/sizeof element trick */
                emit(ctx, "for (int _idx_%s = 0; _idx_%s < (int)(sizeof(%s)/sizeof(%s[0])); _idx_%s++) {\n",
                     iter_name, iter_name, arr_name, arr_name, iter_name);
            }
            ctx->indent++;
            emit_line(ctx, "%s %s = %s[_idx_%s];",
                      iter_ctype, iter_name, arr_name, iter_name);
            gen_block(ctx, n->as.redo_loop.body);
            ctx->indent--;
            emit_indent(ctx); emit(ctx, "}\n");
            break;
        }

        /* ── Return ───────────────────────────────────────── */
        case AST_RETURN_STMT:
            emit_indent(ctx);
            if (n->as.ret.value) {
                emit(ctx, "return ");
                gen_expr(ctx, n->as.ret.value);
                emit(ctx, ";\n");
            } else {
                emit(ctx, "return;\n");
            }
            break;

        /* ── Stop (break) ─────────────────────────────────── */
        case AST_STOP_STMT:
            emit_line(ctx, "break;");
            break;

        /* ── Expression statement ─────────────────────────── */
        case AST_EXPR_STMT: {
            int is_dyn = 0;
            if (n->as.expr_stmt.expr->kind == AST_CALL_EXPR && n->as.expr_stmt.expr->as.call.callee) {
                Symbol* target_sym = sym_lookup(ctx->symtable, n->as.expr_stmt.expr->as.call.callee);
                if (target_sym && target_sym->type == GTYPE_DYNAMIC) is_dyn = 1;
            }
            // For general function calls that are not in symtable, we just emit normally.
            if (is_dyn) {
                char buf[4096] = {0};
                ast_to_python(n->as.expr_stmt.expr, buf, sizeof(buf)-1);
                strncat(buf, "\n", sizeof(buf)-1);
                emit_pyruntime_call(ctx, buf);
            } else {
                emit_indent(ctx);
                gen_expr(ctx, n->as.expr_stmt.expr);
                emit(ctx, ";\n");
            }
            break;
        }

        /* ── Nested block ─────────────────────────────────── */
        case AST_BLOCK:
            gen_block(ctx, n);
            break;

        /* ── Python runtime statement ─────────────────────── */
        case AST_PYRUNTIME_STMT:
            emit_pyruntime_call(ctx, n->as.pyruntime.snippet);
            break;

        /* ── Function declaration ─────────────────────────── */
        case AST_FUNC_DECL: {
            GampilType ret = n->as.func_decl.ret_type;
            emit(ctx, "\n");
            emit_indent(ctx);
            if (ret == GTYPE_BITOFF) emit(ctx, "void");
            else emit(ctx, "%s", gtype_to_c(ret));
            emit(ctx, " %s(", n->as.func_decl.name);

            /* Parameters */
            int first = 1;
            for (AstList* pa = n->as.func_decl.params; pa; pa = pa->next) {
                AstNode* pm = pa->node;
                if (!first) emit(ctx, ", ");
                first = 0;
                emit_type_decl(ctx, pm->as.param.type,
                               pm->as.param.is_pointer, 0,
                               pm->as.param.name, pm);
            }
            emit(ctx, ") {\n");

            /* Emit default values as assignments at top of body */
            sym_push_scope(ctx->symtable);
            for (AstList* pa = n->as.func_decl.params; pa; pa = pa->next) {
                AstNode* pm = pa->node;
                sym_declare(ctx->symtable, pm->as.param.name,
                            pm->as.param.type, pm->as.param.is_pointer, 0, 0);
                /* Suppress unused parameter warning */
                emit_line(ctx, "(void)%s;", pm->as.param.name);
            }

            ctx->indent++;
            gen_block(ctx, n->as.func_decl.body);
            ctx->indent--;
            sym_pop_scope(ctx->symtable);

            emit_indent(ctx); emit(ctx, "}\n");
            break;
        }

        default:
            emit_indent(ctx);
            emit(ctx, "/* unhandled AST node %d */\n", n->kind);
            break;
    }
}

static void gen_block(CodegenCtx* ctx, AstNode* block) {
    if (!block) return;
    sym_push_scope(ctx->symtable);
    for (AstList* s = block->as.block.stmts; s; s = s->next)
        gen_node(ctx, s->node);
    sym_pop_scope(ctx->symtable);
}

/* ── Forward-declare all functions (for mutual recursion) ────── */
static void emit_forward_decls(CodegenCtx* ctx, AstNode* program) {
    for (AstList* d = program->as.program.decls; d; d = d->next) {
        AstNode* n = d->node;
        if (!n || n->kind != AST_FUNC_DECL) continue;
        GampilType ret = n->as.func_decl.ret_type;
        if (ret == GTYPE_BITOFF) emit(ctx, "void");
        else emit(ctx, "%s", gtype_to_c(ret));
        if (strcmp(n->as.func_decl.name, "main") == 0) {
            emit(ctx, " _gampil_main(");
        } else {
            emit(ctx, " %s(", n->as.func_decl.name);
        }
        int first = 1;
        for (AstList* pa = n->as.func_decl.params; pa; pa = pa->next) {
            AstNode* pm = pa->node;
            if (!first) emit(ctx, ", ");
            first = 0;
            emit_type_decl(ctx, pm->as.param.type,
                           pm->as.param.is_pointer, 0,
                           pm->as.param.name, pm);
        }
        emit(ctx, ");\n");
        sym_declare_func(ctx->symtable, n->as.func_decl.name, ret, n->as.func_decl.params);
    }
}

/* ── Main entry: generate full C file ──────────────────────── */
int codegen_run(CodegenCtx* ctx, AstNode* program) {
    if (!program) return -1;

    /* C file header */
    emit(ctx, "/* Auto-generated by Gampil Compiler */\n");
    emit(ctx, "#include <stdio.h>\n");
    emit(ctx, "#include <stdlib.h>\n");
    emit(ctx, "#include <string.h>\n");
    emit(ctx, "#include <math.h>\n");
    emit(ctx, "#include <complex.h>\n\n");

    /* Forward declare all functions */
    emit(ctx, "/* --- Forward Declarations --- */\n");
    emit_forward_decls(ctx, program);
    emit(ctx, "\n");

    /* Find main function — it must be named 'main' in Gampil */
    AstNode* main_func = NULL;
    AstList* others    = NULL;

    for (AstList* d = program->as.program.decls; d; d = d->next) {
        AstNode* n = d->node;
        if (!n) continue;
        if (n->kind == AST_FUNC_DECL &&
            strcmp(n->as.func_decl.name, "main") == 0) {
            main_func = n;
        } else {
            others = astlist_append(others, n);
        }
    }

    /* Generate non-main functions first */
    for (AstList* d = others; d; d = d->next) {
        AstNode* n = d->node;
        if (n) gen_node(ctx, n);
    }

    /* Generate main function */
    if (main_func) {
        /* Gampil main becomes _gampil_main */
        GampilType ret = main_func->as.func_decl.ret_type;
        if (ret == GTYPE_BITOFF) emit(ctx, "\nvoid _gampil_main(");
        else emit(ctx, "\n%s _gampil_main(", gtype_to_c(ret));
        
        int first = 1;
        for (AstList* pa = main_func->as.func_decl.params; pa; pa = pa->next) {
            AstNode* pm = pa->node;
            if (!first) emit(ctx, ", ");
            first = 0;
            emit_type_decl(ctx, pm->as.param.type,
                           pm->as.param.is_pointer, 0,
                           pm->as.param.name, pm);
        }
        emit(ctx, ") {\n");
        ctx->indent++;
        for (AstList* pa = main_func->as.func_decl.params; pa; pa = pa->next) {
            AstNode* pm = pa->node;
            emit_line(ctx, "(void)%s;", pm->as.param.name);
        }
        gen_block(ctx, main_func->as.func_decl.body);
        ctx->indent--;
        emit(ctx, "}\n");
        
        /* C main */
        emit(ctx, "\nint main(int argc, char** argv) {\n");
        ctx->indent++;
        emit_line(ctx, "(void)argc;");
        emit_line(ctx, "_gampil_main((unsigned char**)argv);");
        emit_line(ctx, "return 0;");
        ctx->indent--;
        emit(ctx, "}\n");
    } else {
        /* No explicit main — wrap top-level statements */
        emit(ctx, "\nint main(int argc, char** argv) {\n");
        ctx->indent++;
        emit_line(ctx, "(void)argc;");
        emit_line(ctx, "(void)argv;");
        for (AstList* d = program->as.program.decls; d; d = d->next) {
            AstNode* n = d->node;
            if (n && n->kind != AST_FUNC_DECL)
                gen_node(ctx, n);
        }
        emit_line(ctx, "return 0;");
        ctx->indent--;
        emit(ctx, "}\n");
    }

    /* Cleanup temp lists */
    AstList* cur = others;
    while (cur) { AstList* nx = cur->next; free(cur); cur = nx; }

    return ctx->had_error ? -1 : 0;
}
