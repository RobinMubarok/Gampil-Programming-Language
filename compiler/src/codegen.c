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
static void emit(CodegenCtx* ctx, const char* fmt, ...);
static void gen_node(CodegenCtx* ctx, AstNode* n);
static void gen_expr(CodegenCtx* ctx, AstNode* n);
static int count_list(AstList* lst) {
    int c = 0;
    while (lst) { c++; lst = lst->next; }
    return c;
}
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
        case AST_INT_LIT:
            if (n->as.int_lit.value >= -2147483648LL && n->as.int_lit.value <= 2147483647LL)
                return GTYPE_NUM32;
            return GTYPE_NUM64;
        case AST_FLOAT_LIT: return GTYPE_RAT64;
        case AST_STR_LIT: return GTYPE_ASC8;
        case AST_BOOL_LIT: return GTYPE_BITON;
        default: return GTYPE_NUM64;
    }
}

/* Generate C type for var_decl or param */
static void emit_type_decl(CodegenCtx* ctx, GampilType t, int is_pointer,
                            int array_size, const char* name, AstNode* decl_node) {
    if (t == GTYPE_DYNAMIC) t = GTYPE_NUM64; /* Treat dynamic as 64-bit int natively */
    if (t == GTYPE_REG) {
        emit(ctx, "register int %s __asm__(\"%s\")", name, (decl_node && decl_node->kind == AST_VAR_DECL && decl_node->as.var_decl.reg_name) ? decl_node->as.var_decl.reg_name : "eax");
        return;
    }
    if (t == GTYPE_FIELD)   {
        emit(ctx, "struct { ");
        AstNode* init = (decl_node && decl_node->kind == AST_VAR_DECL) ? decl_node->as.var_decl.initializer : NULL;
        if (init && init->kind == AST_TABLE_LIT) {
            int i = 0;
            for (AstList* e = init->as.table.elements; e; e = e->next, i++) {
                AstNode* elem = e->node;
                if (elem->kind == AST_MULTI_ASSIGN && count_list(elem->as.multi_assign.targets) == 1) {
                    elem = elem->as.multi_assign.targets->node;
                }
                if (elem->kind == AST_VAR_DECL) {
                    emit(ctx, "%s", gtype_to_c(elem->as.var_decl.type));
                    for (int p=0; p < elem->as.var_decl.is_pointer; p++) emit(ctx, "*");
                    if (elem->as.var_decl.array_size > 0) emit(ctx, " %s[%d]; ", elem->as.var_decl.name, elem->as.var_decl.array_size);
                    else emit(ctx, " %s; ", elem->as.var_decl.name);
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

/* ── Python runtime bridge ──────────────────────────────────── */

static void emit_pyruntime_call(CodegenCtx* ctx, const char* snippet) {
    /* Write snippet to a temp file and call python runtime */
    emit_indent(ctx);
    emit(ctx, "{\n");
    ctx->indent++;
    emit_line(ctx, "FILE* _pyf = fopen(\"_gampil_pysnip.py\", \"w\");");
    emit_line(ctx, "if (_pyf) {");
    ctx->indent++;
    emit_indent(ctx);
    emit(ctx, "fprintf(_pyf, \"%%s\", ");
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
    const char* rt_path = g_runtime_path ? g_runtime_path : "gampil_runtime.py";
    
    /* Escape backslashes for C string literal */
    char esc_rt[2048] = {0};
    int j = 0;
    for (int i = 0; rt_path[i] && j < 2046; i++) {
        if (rt_path[i] == '\\') {
            esc_rt[j++] = '\\';
            esc_rt[j++] = '\\';
        } else {
            esc_rt[j++] = rt_path[i];
        }
    }
    esc_rt[j] = '\0';
    
    emit_indent(ctx);
    emit(ctx, "const char* _env_py = getenv(\"GAMPIL_PYTHON\");\n");
    emit_indent(ctx);
    emit(ctx, "const char* _py = _env_py ? _env_py : \"%s\";\n", py_cmd);
    emit_indent(ctx);
    emit(ctx, "const char* _env_rt = getenv(\"GAMPIL_RUNTIME\");\n");
    emit_indent(ctx);
    emit(ctx, "const char* _rt = _env_rt ? _env_rt : \"%s\";\n", esc_rt);
    emit_indent(ctx);
    emit(ctx, "char _cmd[2048];\n");
    emit_indent(ctx);
    emit(ctx, "snprintf(_cmd, sizeof(_cmd), \"%%s \\\"%%s\\\" _gampil_pysnip.py\", _py, _rt);\n");
    emit_indent(ctx);
    emit(ctx, "system(_cmd);\n");
    ctx->indent--;
    emit_line(ctx, "}");
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

/* Check if an identifier is a Python builtin cast name */
static int is_python_cast_name(const char* name) {
    static const char* py_casts[] = {
        "int", "float", "str", "list", "set", "tuple", "dict",
        "frozenset", "bool", "complex", "bytes", "bytearray",
        "range", "type", NULL
    };
    for (int i = 0; py_casts[i]; i++) {
        if (strcmp(name, py_casts[i]) == 0) return 1;
    }
    return 0;
}

/* Returns 1 only when an explicit Python cast is present (e.g. int[x], list[{...}]) */
static int expr_has_python_cast(CodegenCtx* ctx, AstNode* n) {
    if (!n) return 0;
    switch (n->kind) {
        case AST_INDEX_EXPR:
            /* Check if this is python_type[expr] — the trigger for Python bridge */
            if (n->as.index.array->kind == AST_IDENT) {
                const char* name = n->as.index.array->as.ident.name;
                /* Only if it's NOT a declared variable (i.e. truly a Python cast) */
                Symbol* s = sym_lookup(ctx->symtable, name);
                if (!s && is_python_cast_name(name)) return 1;
            }
            /* Recurse into sub-expressions */
            return expr_has_python_cast(ctx, n->as.index.array)
                || expr_has_python_cast(ctx, n->as.index.index);
        case AST_UNARY_EXPR:
            return expr_has_python_cast(ctx, n->as.unary.operand);
        case AST_BINARY_EXPR:
            return expr_has_python_cast(ctx, n->as.binary.left)
                || expr_has_python_cast(ctx, n->as.binary.right);
        case AST_CALL_EXPR:
        case AST_PRINTF_CALL:
        case AST_PRINT_CALL:
        case AST_PRINTN_CALL:
            for (AstList* arg = n->as.call.args; arg; arg = arg->next) {
                if (expr_has_python_cast(ctx, arg->node)) return 1;
            }
            return 0;
        case AST_TABLE_LIT:
            for (AstList* el = n->as.table.elements; el; el = el->next) {
                if (expr_has_python_cast(ctx, el->node)) return 1;
            }
            return 0;
        default: return 0;
    }
}

static void gen_expr(CodegenCtx* ctx, AstNode* n) {
    if (!n) { emit(ctx, "0"); return; }
    switch (n->kind) {
        case AST_INT_LIT:
            if (n->as.int_lit.value >= -2147483648LL && n->as.int_lit.value <= 2147483647LL)
                emit(ctx, "%lld", n->as.int_lit.value);
            else
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
        case AST_STR_LIT: {
            /* Check if this is an f-string */
            int is_fstring = 0;
            if (n->as.str_lit.prefix) {
                for (int i = 0; n->as.str_lit.prefix[i]; i++) {
                    if (n->as.str_lit.prefix[i] == 'f' || n->as.str_lit.prefix[i] == 'F') is_fstring = 1;
                }
            }
            if (is_fstring) {
                /* Native C f-string using GCC statement expression:
                 * ({ char* _fb = malloc(1024); snprintf(_fb, 1024, "fmt", args...); _fb; })
                 * Parse the string value to find {var} interpolations.
                 */
                emit(ctx, "({ char* _fb = (char*)malloc(1024); snprintf(_fb, 1024, \"");
                /* Build format string and collect args */
                const char* src = n->as.str_lit.value;
                char args_buf[2048] = {0};
                int args_len = 0;
                while (*src) {
                    if (*src == '{') {
                        src++;
                        /* Extract variable name */
                        char varname[128];
                        int vi = 0;
                        while (*src && *src != '}' && vi < 126) {
                            varname[vi++] = *src++;
                        }
                        varname[vi] = '\0';
                        if (*src == '}') src++;
                        /* Look up type in symtable to choose format specifier */
                        Symbol* vs = sym_lookup(ctx->symtable, varname);
                        if (vs) {
                            switch (vs->type) {
                                case GTYPE_RAT32: case GTYPE_RAT64: case GTYPE_RAT128:
                                    emit(ctx, "%%g"); break;
                                case GTYPE_ASC8: case GTYPE_ASC16: case GTYPE_ASC32: case GTYPE_ASC64:
                                    if (vs->is_pointer) emit(ctx, "%%s");
                                    else emit(ctx, "%%c");
                                    break;
                                case GTYPE_BITON: case GTYPE_BITOFF:
                                    emit(ctx, "%%lld"); break;
                                case GTYPE_NUM8: case GTYPE_NUM16: case GTYPE_NUM32: case GTYPE_NUM64:
                                    emit(ctx, "%%lld"); break;
                                default:
                                    emit(ctx, "%%lld"); break;
                            }
                        } else {
                            /* Unknown var — default to %lld */
                            emit(ctx, "%%lld");
                        }
                        
                        /* Append variable as argument, cast integers to long long to prevent warnings */
                        if (vs && (vs->type == GTYPE_NUM8 || vs->type == GTYPE_NUM16 || vs->type == GTYPE_NUM32 || vs->type == GTYPE_NUM64 || vs->type == GTYPE_BITON || vs->type == GTYPE_BITOFF)) {
                            args_len += snprintf(args_buf + args_len, sizeof(args_buf) - args_len, ", (long long)%s", varname);
                        } else {
                            args_len += snprintf(args_buf + args_len, sizeof(args_buf) - args_len, ", %s", varname);
                        }
                    } else if (*src == '"') {
                        emit(ctx, "\\\"");
                        src++;
                    } else if (*src == '\n') {
                        emit(ctx, "\\n");
                        src++;
                    } else if (*src == '\\') {
                        /* Check for \n escape sequence */
                        if (*(src+1) == 'n') {
                            emit(ctx, "\\n");
                            src += 2;
                        } else {
                            emit(ctx, "\\\\");
                            src++;
                        }
                    } else if (*src == '%') {
                        emit(ctx, "%%%%");
                        src++;
                    } else {
                        fputc(*src, ctx->out);
                        src++;
                    }
                }
                emit(ctx, "\"%s); (void*)_fb; })", args_buf);
            } else {
                /* Normal string literal */
                emit(ctx, "\"");
                for (const char* c = n->as.str_lit.value; *c; c++) {
                    if (*c == '"') emit(ctx, "\\\"");
                    else if (*c == '\n') emit(ctx, "\\n");
                    else if (*c == '\\') emit(ctx, "\\\\");
                    else fputc(*c, ctx->out);
                }
                emit(ctx, "\"");
            }
            break;
        }
        case AST_BOOL_LIT:
            emit(ctx, "%d", n->as.bool_lit.value);
            break;
        case AST_NIL_LIT:
            emit(ctx, "NULL");
            break;
        case AST_ELSE_EXPR:
            emit(ctx, "_else_flag");
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
            Symbol* func_sym = NULL;
            if (n->as.call.callee) {
                func_sym = sym_lookup(ctx->symtable, n->as.call.callee);
            }

            if (n->as.call.callee && !func_sym && is_asm_instruction(n->as.call.callee)) {
                int argc = 0;
                for (AstList* a = n->as.call.args; a; a = a->next) argc++;
                
                if (argc == 0) {
                    emit(ctx, "__asm__ (\"%s\")", n->as.call.callee);
                } else if (argc == 1) {
                    int is_write = (strcmp(n->as.call.callee, "pop") == 0 || strcmp(n->as.call.callee, "inc") == 0 || strcmp(n->as.call.callee, "dec") == 0);
                    if (is_write) {
                        emit(ctx, "__asm__ (\"%s %%0\" : \"=g\"(", n->as.call.callee);
                        gen_expr(ctx, n->as.call.args->node);
                        emit(ctx, "))");
                    } else {
                        emit(ctx, "__asm__ (\"%s %%0\" : : \"g\"(", n->as.call.callee);
                        gen_expr(ctx, n->as.call.args->node);
                        emit(ctx, "))");
                    }
                } else if (argc == 2) {
                    emit(ctx, "__asm__ (\"%s %%1, %%0\" : \"=g\"(", n->as.call.callee);
                    gen_expr(ctx, n->as.call.args->node);
                    emit(ctx, ") : \"g\"(");
                    gen_expr(ctx, n->as.call.args->next->node);
                    emit(ctx, "))");
                } else {
                    emit(ctx, "__asm__ (\"%s", n->as.call.callee);
                    for (int i = 0; i < argc; i++) {
                        emit(ctx, " %%%d%c", i, (i == argc - 1) ? '"' : ',');
                    }
                    emit(ctx, " : \"=g\"(");
                    gen_expr(ctx, n->as.call.args->node);
                    emit(ctx, ")");
                    if (argc > 1) {
                        emit(ctx, " : ");
                        int first = 1;
                        for (AstList* a = n->as.call.args->next; a; a = a->next) {
                            if (!first) emit(ctx, ", ");
                            first = 0;
                            emit(ctx, "\"g\"(");
                            gen_expr(ctx, a->node);
                            emit(ctx, ")");
                        }
                    }
                    emit(ctx, ")");
                }
                break;
            }

            emit(ctx, "%s(", n->as.call.callee ? n->as.call.callee : "unknown");
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
                AstNode* elem = e->node;
                if (elem->kind == AST_MULTI_ASSIGN && count_list(elem->as.multi_assign.targets) == 1) {
                    elem = elem->as.multi_assign.values->node;
                }
                if (elem->kind == AST_VAR_DECL)
                    gen_expr(ctx, elem->as.var_decl.initializer);
                else
                    gen_expr(ctx, elem);
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

/* ── Python runtime bridge ──────────────────────────────────── */

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
            strncat(buf, "GampilTable(", maxlen);
            int first = 1;
            for (AstList* e = n->as.table.elements; e; e = e->next) {
                if (!first) strncat(buf, ", ", maxlen);
                first = 0;
                if (e->node->kind == AST_VAR_DECL) {
                    snprintf(tmp, sizeof(tmp), "%s=", e->node->as.var_decl.name);
                    strncat(buf, tmp, maxlen);
                    ast_to_python(e->node->as.var_decl.initializer, buf, maxlen);
                } else {
                    ast_to_python(e->node, buf, maxlen);
                }
            }
            strncat(buf, ")", maxlen);
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
            if (n->kind == AST_PRINTF_CALL) strncat(buf, "printf", maxlen);
            else if (n->kind == AST_PRINTN_CALL) strncat(buf, "printn", maxlen);
            else if (n->kind == AST_PRINT_CALL) strncat(buf, "print", maxlen);
            else strncat(buf, n->as.call.callee ? n->as.call.callee : "unknown", maxlen);
            
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
            if (n->as.index.array->kind == AST_IDENT) {
                char* name = n->as.index.array->as.ident.name;
                if (strcmp(name, "set") == 0 || strcmp(name, "list") == 0 || strcmp(name, "tuple") == 0 || strcmp(name, "frozenset") == 0) {
                    strncat(buf, name, maxlen);
                    strncat(buf, "(", maxlen);
                    ast_to_python(n->as.index.index, buf, maxlen);
                    strncat(buf, ")", maxlen);
                    break;
                }
            }
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
            emit_indent(ctx);
            emit_type_decl(ctx, n->as.var_decl.type, n->as.var_decl.is_pointer,
                           n->as.var_decl.array_size, n->as.var_decl.name, n);
            
            if (n->as.var_decl.initializer) {
                emit(ctx, " = ");
                gen_expr(ctx, n->as.var_decl.initializer);
            } else if (n->as.var_decl.array_size > 0) {
                emit(ctx, " = {0}");
            }
            emit(ctx, ";\n");
            
            /* Python bridge only for explicit Python casts in initializer */
            if (n->as.var_decl.initializer && expr_has_python_cast(ctx, n->as.var_decl.initializer)) {
                char buf[4096] = {0};
                snprintf(buf, sizeof(buf), "%s = ", n->as.var_decl.name);
                ast_to_python(n->as.var_decl.initializer, buf, sizeof(buf)-1);
                strncat(buf, "\n", sizeof(buf)-1);
                emit_pyruntime_call(ctx, buf);
            }
            
            sym_declare(ctx->symtable, n->as.var_decl.name,
                        n->as.var_decl.type, n->as.var_decl.is_pointer,
                        n->as.var_decl.array_size, 0);
            break;
        }

        /* ── Assignment ───────────────────────────────────── */
        case AST_ASSIGN_STMT: {
            Symbol* target_sym = sym_lookup(ctx->symtable, n->as.assign.target);

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
            
            /* Python bridge only for explicit Python casts in value */
            if (n->as.assign.value && expr_has_python_cast(ctx, n->as.assign.value)) {
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
            }
            break;
        }

        /* ── Multi-Assignment (varmult) ───────────────────── */
        case AST_MULTI_ASSIGN: {
            char unique[64];
            snprintf(unique, sizeof(unique), "_varmult_%d_%d", n->line, n->col);
            
            int val_count = 0;
            for (AstList* v = n->as.multi_assign.values; v; v = v->next) val_count++;
            
            int target_count = 0;
            for (AstList* t = n->as.multi_assign.targets; t; t = t->next) target_count++;
            
            if (target_count == 1) {
                AstNode* target = n->as.multi_assign.targets->node;
                AstNode* val = (val_count == 1) ? n->as.multi_assign.values->node : NULL;
                
                if (target->kind == AST_VAR_DECL) {
                    target->as.var_decl.initializer = val;
                    gen_node(ctx, target);
                    target->as.var_decl.initializer = NULL; /* Prevent double-free in ast_free */
                } else if (target->kind == AST_IDENT) {
                    AstNode assign = {0};
                    assign.kind = AST_ASSIGN_STMT;
                    assign.line = target->line;
                    assign.col = target->col;
                    assign.as.assign.target = target->as.ident.name;
                    assign.as.assign.op = TOK_BE;
                    assign.as.assign.value = val;
                    gen_node(ctx, &assign);
                }
                break;
            }
            
            if (val_count == 1 && target_count > 1) {
                emit_indent(ctx);
                emit(ctx, "%s %s_shared = ", gtype_to_c(infer_type(n->as.multi_assign.values->node)), unique);
                gen_expr(ctx, n->as.multi_assign.values->node);
                emit(ctx, ";\n");
            } else {
                int vi = 0;
                for (AstList* v = n->as.multi_assign.values; v; v = v->next, vi++) {
                    emit_indent(ctx);
                    emit(ctx, "%s %s_%d = ", gtype_to_c(infer_type(v->node)), unique, vi);
                    gen_expr(ctx, v->node);
                    emit(ctx, ";\n");
                }
            }
            
            int ti = 0;
            for (AstList* t = n->as.multi_assign.targets; t; t = t->next, ti++) {
                AstNode* target = t->node;
                char* tmp_name = (val_count == 1 && target_count > 1) ? "_shared" : NULL;
                char tmp_buf[32];
                if (!tmp_name) {
                    snprintf(tmp_buf, sizeof(tmp_buf), "_%d", (ti < val_count) ? ti : (val_count - 1));
                    tmp_name = tmp_buf;
                }
                
                if (target->kind == AST_VAR_DECL) {
                    emit_indent(ctx);
                    emit_type_decl(ctx, target->as.var_decl.type, target->as.var_decl.is_pointer, target->as.var_decl.array_size, target->as.var_decl.name, target);
                    if (val_count > 0) {
                        emit(ctx, " = %s%s;\n", unique, tmp_name);
                    } else {
                        emit(ctx, ";\n");
                    }
                    sym_declare(ctx->symtable, target->as.var_decl.name, target->as.var_decl.type, target->as.var_decl.is_pointer, target->as.var_decl.array_size, 0);
                } else if (target->kind == AST_IDENT) {
                    if (val_count > 0) {
                        emit_indent(ctx);
                        emit(ctx, "%s = %s%s;\n", target->as.ident.name, unique, tmp_name);
                    }
                }
            }
            break;
        }

        /* ── If-statement: Parallel Guard Branching ───────── */
        case AST_IF_STMT: {
            emit_indent(ctx); emit(ctx, "/* --- Parallel Guard Block --- */\n");
            emit_indent(ctx); emit(ctx, "{\n");
            ctx->indent++;
            emit_line(ctx, "int _else_flag = 1;");
            
            int gi = 0;
            for (GuardClause* g = n->as.if_stmt.guards; g; g = g->next, gi++) {
                emit_indent(ctx);
                emit(ctx, "int _guard%d = (", gi);
                if (g->cond) gen_expr(ctx, g->cond);
                else emit(ctx, "1");
                emit(ctx, ");\n");
                
                emit_line(ctx, "if (_guard%d) _else_flag = 0;", gi);
            }
            
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
            if (n->as.redo_loop.while_cond && !n->as.redo_loop.arrays) {
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
            if (!n->as.redo_loop.arrays) {
                emit_indent(ctx); emit(ctx, "while (1) {\n");
                ctx->indent++;
                gen_block(ctx, n->as.redo_loop.body);
                ctx->indent--;
                emit_indent(ctx); emit(ctx, "}\n");
                break;
            }

            /* Case 3: foreach  redo arrs [quite] as iters: */
            emit_indent(ctx);
            emit(ctx, "{\n");
            ctx->indent++;
            
            int ai = 0;
            for (AstList* al = n->as.redo_loop.arrays; al; al = al->next, ai++) {
                AstNode* arr_expr = al->node;
                if (arr_expr->kind == AST_TABLE_LIT) {
                    int n_elems = 0;
                    for (AstList* el = arr_expr->as.table.elements; el; el = el->next) n_elems++;
                    
                    GampilType elem_type = GTYPE_NUM64;
                    if (arr_expr->as.table.elements)
                        elem_type = infer_type(arr_expr->as.table.elements->node);
                        
                    emit_indent(ctx);
                    emit(ctx, "%s _tmp_arr_%d_%d[%d] = ", gtype_to_c(elem_type), n->line, ai, n_elems);
                    gen_expr(ctx, arr_expr);
                    emit(ctx, ";\n");
                }
            }
            
            AstNode* first_arr = n->as.redo_loop.arrays ? n->as.redo_loop.arrays->node : NULL;
            char first_arr_name[256] = "unknown";
            int first_arr_size = 0;
            if (first_arr) {
                if (first_arr->kind == AST_TABLE_LIT) {
                    int n_elems = 0;
                    for (AstList* el = first_arr->as.table.elements; el; el = el->next) n_elems++;
                    first_arr_size = n_elems;
                    sprintf(first_arr_name, "_tmp_arr_%d_0", n->line);
                } else if (first_arr->kind == AST_IDENT) {
                    strncpy(first_arr_name, first_arr->as.ident.name, sizeof(first_arr_name)-1);
                    Symbol* s = sym_lookup(ctx->symtable, first_arr_name);
                    if (s) first_arr_size = s->array_size;
                }
            }
            
            char idx_name[64];
            sprintf(idx_name, "_idx_%d", n->line);
            
            emit_indent(ctx);
            if (first_arr_size > 0) {
                emit(ctx, "for (int %s = 0; %s < %d; %s++) {\n", idx_name, idx_name, first_arr_size, idx_name);
            } else {
                emit(ctx, "for (int %s = 0; %s < (int)(sizeof(%s)/sizeof(%s[0])); %s++) {\n", idx_name, idx_name, first_arr_name, first_arr_name, idx_name);
            }
            ctx->indent++;
            
            int i_idx = 0;
            AstList* curr_arr = n->as.redo_loop.arrays;
            for (AstList* it = n->as.redo_loop.iters; it && curr_arr; it = it->next, curr_arr = curr_arr->next, i_idx++) {
                AstNode* iter_var = it->node;
                AstNode* arr_node = curr_arr->node;
                
                char arr_expr_str[256];
                if (arr_node->kind == AST_TABLE_LIT) {
                    sprintf(arr_expr_str, "_tmp_arr_%d_%d", n->line, i_idx);
                } else if (arr_node->kind == AST_IDENT) {
                    strcpy(arr_expr_str, arr_node->as.ident.name);
                } else {
                    strcpy(arr_expr_str, "unknown");
                }
                
                if (iter_var->as.var_decl.type == GTYPE_DYNAMIC) {
                    GampilType elem_type = GTYPE_NUM64;
                    if (arr_node->kind == AST_TABLE_LIT && arr_node->as.table.elements) {
                        elem_type = infer_type(arr_node->as.table.elements->node);
                    } else if (arr_node->kind == AST_IDENT) {
                        Symbol* s = sym_lookup(ctx->symtable, arr_node->as.ident.name);
                        if (s) elem_type = s->type;
                    }
                    
                    /* Declare local C variable for the dynamic iterator */
                    emit_indent(ctx);
                    emit_type_decl(ctx, elem_type, 0, 0, iter_var->as.var_decl.name, iter_var);
                    emit(ctx, " = %s[%s];\n", arr_expr_str, idx_name);
                } else {
                    emit_indent(ctx);
                    emit_type_decl(ctx, iter_var->as.var_decl.type,
                                   iter_var->as.var_decl.is_pointer,
                                   iter_var->as.var_decl.array_size,
                                   iter_var->as.var_decl.name, iter_var);
                    emit(ctx, " = %s[%s];\n", arr_expr_str, idx_name);
                }
            }
            
            gen_block(ctx, n->as.redo_loop.body);
            ctx->indent--;
            emit_indent(ctx); emit(ctx, "}\n");
            
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
            int has_pycast = expr_has_python_cast(ctx, n->as.expr_stmt.expr);
            
            if (has_pycast) {
                char buf[4096] = {0};
                ast_to_python(n->as.expr_stmt.expr, buf, sizeof(buf)-1);
                strncat(buf, "\n", sizeof(buf)-1);
                emit_pyruntime_call(ctx, buf);
            } else {
                /* Execute purely native expression */
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
