/* ============================================================
 *  Gampil Programming Language — Recursive Descent Parser
 *  parser.c
 * ============================================================ */

#include "../include/parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Internal helpers ───────────────────────────────────────── */

static void parser_error(Parser* p, const char* msg) {
    if (!p->had_error) {
        int err_line = p->current.line;
        int err_col = p->current.col;

        snprintf(p->error_msg, sizeof(p->error_msg),
                 "Error: %s (got '%s')",
                 msg,
                 p->current.value ? p->current.value
                                  : token_type_str(p->current.type));
        
        fprintf(stderr, "%s\n", p->error_msg);
        fprintf(stderr, "  --> line %d:%d\n", err_line, err_col);
        fprintf(stderr, "   |\n");
        fprintf(stderr, "%3d| ", err_line);
        
        /* Find start of the line */
        const char* src = p->lexer->source;
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
            /* Handle tabs */
            if (line_start[i-1] == '\t') fputc('\t', stderr);
            else fputc(' ', stderr);
        }
        fprintf(stderr, "^ %s\n", msg);
        fprintf(stderr, "   |\n");
        
        p->had_error = 1;
    }
}

/* Advance to next real token */
static Token advance(Parser* p) {
    token_free(p->current);
    p->current   = p->lookahead;
    p->lookahead = lexer_next(p->lexer);
    return p->current;
}

static int check(Parser* p, TokenType t) { return p->current.type == t; }
static int check2(Parser* p, TokenType t) { return p->lookahead.type == t; }

static int consume(Parser* p, TokenType t, const char* msg) {
    if (p->current.type == t) { advance(p); return 1; }
    parser_error(p, msg);
    return 0;
}

/* Skip blank lines (newline tokens) */
static void skip_newlines(Parser* p) {
    while (check(p, TOK_NEWLINE)) advance(p);
}

/* Check if current token is a type keyword */
static int is_type_tok(TokenType t) {
    switch (t) {
        case TOK_BITOFF: case TOK_BITON:
        case TOK_ASC8:  case TOK_ASC16: case TOK_ASC32: case TOK_ASC64:
        case TOK_NUM8:  case TOK_NUM16: case TOK_NUM32: case TOK_NUM64:
        case TOK_RAT32: case TOK_RAT64: case TOK_RAT128:
        case TOK_FIELD: case TOK_LET:
            return 1;
        default: return 0;
    }
}

/* Forward declarations */
static AstNode* parse_stmt(Parser* p);
static AstNode* parse_block(Parser* p);
static AstNode* parse_expr(Parser* p);
static AstNode* parse_expr_stmt(Parser* p);
static GampilType parse_type_spec(Parser* p, int* is_pointer, int* array_size);

/* ── Parser constructor ─────────────────────────────────────── */

Parser* parser_new(Lexer* lexer) {
    Parser* p = (Parser*)calloc(1, sizeof(Parser));
    p->lexer   = lexer;
    /* Prime the two-token buffer */
    p->current   = lexer_next(lexer);
    p->lookahead = lexer_next(lexer);
    return p;
}

void parser_free(Parser* p) { free(p); }

/* ══════════════════════════════════════════════════════════════
 *  Expression parsing  (Pratt / precedence-climbing)
 * ══════════════════════════════════════════════════════════════ */

static int get_unary_prec(TokenType t) {
    switch (t) {
        case TOK_NOT: case TOK_BANG: case TOK_TILDE: case TOK_MINUS:
            return 14;
        default: return -1;
    }
}


static int get_binary_prec(TokenType t) {
    switch (t) {
        case TOK_OR:      return 1;
        case TOK_AND:     return 2;
        case TOK_PIPE:    return 3;
        case TOK_DPIPE:   return 4;
        case TOK_AMP:     return 5;
        case TOK_EQ:
        case TOK_NEQ:     return 6;
        case TOK_LT: case TOK_GT: case TOK_LTE: case TOK_GTE: return 7;
        case TOK_LSHIFT:
        case TOK_RSHIFT:  return 8;
        case TOK_PLUS:
        case TOK_MINUS:   return 9;
        case TOK_STAR:
        case TOK_SLASH:
        case TOK_PERCENT: return 10;
        case TOK_CARET:   return 11;  /* right-assoc (handle in loop) */
        default: return -1;
    }
}

/* Primary: literal, ident, call, index, address-of, parens */
static AstNode* parse_primary(Parser* p) {
    int ln = p->current.line, col = p->current.col;

    /* Integer literal */
    if (check(p, TOK_INT_LIT)) {
        AstNode* n = ast_new(AST_INT_LIT, ln, col);
        n->as.int_lit.value = strtoll(p->current.value, NULL, 0);
        advance(p);
        return n;
    }

    /* Float literal */
    if (check(p, TOK_FLOAT_LIT)) {
        AstNode* n = ast_new(AST_FLOAT_LIT, ln, col);
        n->as.float_lit.value = strtod(p->current.value, NULL);
        advance(p);
        return n;
    }

    /* String literals */
    if (check(p, TOK_STR_DOUBLE) || check(p, TOK_STR_SINGLE)) {
        AstNode* n = ast_new(AST_STR_LIT, ln, col);
        char* val = p->current.value;
        char delim = check(p, TOK_STR_DOUBLE) ? '"' : '\'';
        int is_triple = 0;
        char* prefix = NULL;
        
        char* q = strchr(val, delim);
        if (q) {
            if (q > val) {
                prefix = (char*)malloc(q - val + 1);
                memcpy(prefix, val, q - val);
                prefix[q - val] = '\0';
            }
            if (q[1] == delim && q[2] == delim) {
                is_triple = 1;
                q += 3;
            } else {
                q += 1;
            }
            
            int len = strlen(q);
            if (is_triple && len >= 3) {
                q[len-3] = '\0';
            } else if (!is_triple && len >= 1) {
                q[len-1] = '\0';
            }
            n->as.str_lit.value = strdup(q);
        } else {
            n->as.str_lit.value = strdup(val);
        }
        n->as.str_lit.prefix = prefix;
        n->as.str_lit.delim = delim;
        n->as.str_lit.is_triple = is_triple;
        
        advance(p);
        return n;
    }
    
    /* Complex literal */
    if (check(p, TOK_COMPLEX_LIT)) {
        AstNode* n = ast_new(AST_COMPLEX_LIT, ln, col);
        n->as.complex_lit.value = strdup(p->current.value);
        advance(p);
        return n;
    }

    /* Boolean */
    if (check(p, TOK_TRUE) || check(p, TOK_FALSE)) {
        AstNode* n = ast_new(AST_BOOL_LIT, ln, col);
        n->as.bool_lit.value = check(p, TOK_TRUE) ? 1 : 0;
        advance(p);
        return n;
    }

    /* nil */
    if (check(p, TOK_NIL)) {
        AstNode* n = ast_new(AST_NIL_LIT, ln, col);
        advance(p);
        return n;
    }

    /* Address-of: @varname */
    if (check(p, TOK_AT)) {
        advance(p);
        if (!check(p, TOK_IDENT)) { parser_error(p, "expected identifier after '@'"); return NULL; }
        AstNode* n = ast_new(AST_ADDR_OF, ln, col);
        n->as.addr_of.var = strdup(p->current.value);
        advance(p);
        return n;
    }

    /* malloc[expr] */
    if (check(p, TOK_MALLOC)) {
        advance(p);
        consume(p, TOK_LBRACKET, "expected '[' after 'malloc'");
        AstNode* n = ast_new(AST_MALLOC_CALL, ln, col);
        n->as.malloc_call.size_expr = parse_expr(p);
        consume(p, TOK_RBRACKET, "expected ']' after malloc argument");
        return n;
    }

    /* type cast / pointer cast */
    if (is_type_tok(p->current.type)) {
        int is_ptr = 0, arr_sz = 0;
        GampilType gt = parse_type_spec(p, &is_ptr, &arr_sz);
        if (check(p, TOK_LBRACKET)) {
            advance(p);
            AstNode* n = ast_new(AST_CAST_EXPR, ln, col);
            n->as.cast_expr.target_type = gt;
            n->as.cast_expr.is_pointer = is_ptr;
            n->as.cast_expr.expr = parse_expr(p);
            consume(p, TOK_RBRACKET, "expected ']' after cast expression");
            return n;
        } else {
            parser_error(p, "expected '[' for type cast");
            return NULL;
        }
    }

    /* Table literal: { ... } */
    if (check(p, TOK_LBRACE)) {
        advance(p);
        AstNode* n = ast_new(AST_TABLE_LIT, ln, col);
        AstList* elems = NULL;
        while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
            skip_newlines(p);
            if (check(p, TOK_RBRACE)) break;
            /* Could be a var_decl element (hash field: num16 id be 2) */
            if (is_type_tok(p->current.type)) {
                /* parse as sub-decl */
                AstNode* elem = parse_stmt(p);
                if (elem) elems = astlist_append(elems, elem);
            } else {
                elems = astlist_append(elems, parse_expr(p));
            }
            skip_newlines(p);
            if (check(p, TOK_COMMA)) advance(p);
        }
        consume(p, TOK_RBRACE, "expected '}'");
        n->as.table.elements = elems;
        return n;
    }

    /* Parenthesized expression */
    if (check(p, TOK_LPAREN)) {
        advance(p);
        AstNode* inner = parse_expr(p);
        consume(p, TOK_RPAREN, "expected ')'");
        return inner;
    }

    /* Nil literal */
    if (check(p, TOK_NIL)) {
        AstNode* n = ast_new(AST_NIL_LIT, ln, col);
        advance(p);
        return n;
    }

    /* Identifiers: may be followed by '[' (call) or '(' (index) */
    if (check(p, TOK_IDENT) || check(p, TOK_PRINTF) || check(p, TOK_PRINTN)
        || check(p, TOK_PRINT)) {
        AstKind call_kind = AST_CALL_EXPR;
        if (check(p, TOK_PRINTF)) call_kind = AST_PRINTF_CALL;
        else if (check(p, TOK_PRINTN)) call_kind = AST_PRINTN_CALL;
        else if (check(p, TOK_PRINT))  call_kind = AST_PRINT_CALL;

        char* name = p->current.value ? strdup(p->current.value)
                                      : strdup(token_type_str(p->current.type));
        advance(p);

        /* Function call: name[args] */
        if (check(p, TOK_LBRACKET)) {
            advance(p);
            AstNode* n = ast_new(call_kind, ln, col);
            n->as.call.callee = name;
            AstList* args = NULL;
            while (!check(p, TOK_RBRACKET) && !check(p, TOK_EOF)) {
                if (check(p, TOK_COMMA)) { advance(p); continue; } /* skip empty , */
                /* Keyword arg: name be value */
                if (check(p, TOK_IDENT) && check2(p, TOK_BE)) {
                    /* treat as var-assign node */
                    char* kname = strdup(p->current.value);
                    advance(p); advance(p); /* consume name and 'be' */
                    AstNode* karg = ast_new(AST_ASSIGN_STMT, ln, col);
                    karg->as.assign.target = kname;
                    karg->as.assign.op     = TOK_BE;
                    karg->as.assign.value  = parse_expr(p);
                    args = astlist_append(args, karg);
                } else {
                    args = astlist_append(args, parse_expr(p));
                }
                if (check(p, TOK_COMMA)) advance(p);
            }
            consume(p, TOK_RBRACKET, "expected ']'");
            n->as.call.args = args;
            /* Post-call: chained .field or (index) */
            while (check(p, TOK_DOT) || check(p, TOK_LPAREN)) {
                if (check(p, TOK_DOT)) {
                    advance(p);
                    AstNode* fa = ast_new(AST_FIELD_EXPR, ln, col);
                    fa->as.field_access.object = n;
                    fa->as.field_access.field  = strdup(p->current.value);
                    advance(p);
                    n = fa;
                } else {
                    advance(p); /* ( */
                    AstNode* idx = ast_new(AST_INDEX_EXPR, ln, col);
                    idx->as.index.array = n;
                    idx->as.index.index = parse_expr(p);
                    consume(p, TOK_RPAREN, "expected ')'");
                    n = idx;
                }
            }
            return n;
        }

        /* Array index: name(expr) */
        if (check(p, TOK_LPAREN)) {
            advance(p);
            AstNode* idx = ast_new(AST_INDEX_EXPR, ln, col);
            AstNode* arr = ast_new(AST_IDENT, ln, col);
            arr->as.ident.name = name;
            idx->as.index.array = arr;
            idx->as.index.index = parse_expr(p);
            consume(p, TOK_RPAREN, "expected ')'");
            /* field access after index */
            AstNode* n = idx;
            if (check(p, TOK_DOT)) {
                advance(p);
                AstNode* fa = ast_new(AST_FIELD_EXPR, ln, col);
                fa->as.field_access.object = n;
                fa->as.field_access.field  = strdup(p->current.value);
                advance(p);
                n = fa;
            }
            return n;
        }

        /* Field access: name.field */
        AstNode* n = ast_new(AST_IDENT, ln, col);
        n->as.ident.name = name;
        while (check(p, TOK_DOT)) {
            advance(p);
            AstNode* fa = ast_new(AST_FIELD_EXPR, n->line, n->col);
            fa->as.field_access.object = n;
            if (!check(p, TOK_IDENT)) { parser_error(p, "expected field name"); return n; }
            fa->as.field_access.field = strdup(p->current.value);
            advance(p);
            n = fa;
        }
        return n;
    }

    parser_error(p, "expected expression");
    advance(p); /* recover */
    return NULL;
}

/* Unary prefix */
static AstNode* parse_unary(Parser* p) {
    int ln = p->current.line, col = p->current.col;
    TokenType t = p->current.type;
    if (get_unary_prec(t) >= 0) {
        advance(p);
        AstNode* n = ast_new(AST_UNARY_EXPR, ln, col);
        n->as.unary.op      = t;
        n->as.unary.operand = parse_unary(p);
        return n;
    }
    return parse_primary(p);
}

/* Binary infix with precedence climbing */
static AstNode* parse_binary(Parser* p, int min_prec) {
    AstNode* left = parse_unary(p);
    while (1) {
        int prec = get_binary_prec(p->current.type);
        if (prec < min_prec) break;
        int ln = p->current.line, col = p->current.col;
        TokenType op = p->current.type;
        advance(p);
        /* right-assoc for ^ */
        int next_prec = (op == TOK_CARET) ? prec : prec + 1;
        AstNode* right = parse_binary(p, next_prec);
        AstNode* n     = ast_new(AST_BINARY_EXPR, ln, col);
        n->as.binary.op    = op;
        n->as.binary.left  = left;
        n->as.binary.right = right;
        left = n;
    }
    return left;
}

static AstNode* parse_expr(Parser* p) { return parse_binary(p, 0); }

/* ══════════════════════════════════════════════════════════════
 *  Statement parsing
 * ══════════════════════════════════════════════════════════════ */

/* Parse type + optional (size or empty) → returns pointer/array info */
static GampilType parse_type_spec(Parser* p, int* is_pointer, int* array_size) {
    *is_pointer = 0;
    *array_size = 0;
    GampilType gt = tok_to_gtype(p->current.type);
    advance(p);
    /* Optional (N) for array or () for pointer. Can be multiple e.g. ()() */
    while (check(p, TOK_LPAREN)) {
        advance(p);
        if (check(p, TOK_RPAREN)) {
            (*is_pointer)++; /* empty () = pointer */
            advance(p);
        } else {
            /* size expression */
            if (check(p, TOK_INT_LIT)) {
                *array_size = (int)strtol(p->current.value, NULL, 10);
                advance(p);
            }
            consume(p, TOK_RPAREN, "expected ')'");
        }
    }
    return gt;
}

/* Variable declaration tail: [be <expr>] */
static AstNode* parse_var_decl_tail(Parser* p, GampilType gt, int is_ptr, int arr_sz, char* name, int ln, int col) {
    AstNode* n = ast_new(AST_VAR_DECL, ln, col);
    n->as.var_decl.type       = gt;
    n->as.var_decl.name       = name;
    n->as.var_decl.is_pointer = is_ptr;
    n->as.var_decl.array_size = arr_sz;

    /* Optional 'be' initializer */
    if (check(p, TOK_BE)) {
        advance(p);
        n->as.var_decl.initializer = parse_expr(p);
    }
    return n;
}

/* Function parameter: <type>[(<size>|())] <name> [be <default>] */
static AstNode* parse_param(Parser* p) {
    int ln = p->current.line, col = p->current.col;
    int is_ptr = 0, arr_sz = 0;
    GampilType gt = parse_type_spec(p, &is_ptr, &arr_sz);
    if (!check(p, TOK_IDENT)) { parser_error(p, "expected parameter name"); return NULL; }
    char* name = strdup(p->current.value);
    advance(p);
    AstNode* n = ast_new(AST_PARAM, ln, col);
    n->as.param.type       = gt;
    n->as.param.name       = name;
    n->as.param.is_pointer = is_ptr;
    if (check(p, TOK_BE)) {
        advance(p);
        n->as.param.default_val = parse_expr(p);
    }
    return n;
}

/* Block: sequence of statements until 'ok' */
static AstNode* parse_block(Parser* p) {
    int ln = p->current.line, col = p->current.col;
    AstNode* block = ast_new(AST_BLOCK, ln, col);
    AstList* stmts = NULL;
    while (!check(p, TOK_OK) && !check(p, TOK_EOF) &&
           !check(p, TOK_BUT) && !check(p, TOK_ELSE)) {
        skip_newlines(p);
        if (check(p, TOK_OK) || check(p, TOK_EOF) ||
            check(p, TOK_BUT) || check(p, TOK_ELSE)) break;
        AstNode* s = parse_stmt(p);
        if (s) stmts = astlist_append(stmts, s);
        /* Consume statement-ending newline / semicolon */
        while (check(p, TOK_NEWLINE)) advance(p);
    }
    block->as.block.stmts = stmts;
    return block;
}

/* If-statement with guard clauses */
static AstNode* parse_if_stmt(Parser* p) {
    int ln = p->current.line, col = p->current.col;
    consume(p, TOK_IF, "expected 'if'");
    AstNode* n = ast_new(AST_IF_STMT, ln, col);
    GuardClause* guards = NULL;
    GuardClause* last   = NULL;

    /* First guard: condition */
    GuardClause* g = (GuardClause*)calloc(1, sizeof(GuardClause));
    g->is_else_and = 0;
    g->cond        = parse_expr(p);
    consume(p, TOK_COLON, "expected ':' after if condition");
    skip_newlines(p);
    g->body        = parse_block(p);
    guards = last = g;

    /* 'but' chains */
    while (check(p, TOK_BUT)) {
        advance(p); /* consume 'but' */
        skip_newlines(p);
        if (check(p, TOK_OK)) break; /* bare 'but ok' ends */

        g = (GuardClause*)calloc(1, sizeof(GuardClause));

        /* 'else and <cond>:' → exclusive guard */
        if (check(p, TOK_ELSE)) {
            advance(p);
            consume(p, TOK_AND, "expected 'and' after 'else'");
            g->is_else_and = 1;
            g->cond        = parse_expr(p);
        } else {
            /* plain guard condition */
            g->is_else_and = 0;
            g->cond        = parse_expr(p);
        }
        consume(p, TOK_COLON, "expected ':' after guard condition");
        skip_newlines(p);
        g->body = parse_block(p);
        last->next = g;
        last       = g;
    }

    consume(p, TOK_OK, "expected 'ok' to close if");
    n->as.if_stmt.guards = guards;
    return n;
}

/* Redo loop: 3 forms */
static AstNode* parse_redo_loop(Parser* p) {
    int ln = p->current.line, col = p->current.col;
    consume(p, TOK_REDO, "expected 'redo'");
    AstNode* n = ast_new(AST_REDO_LOOP, ln, col);

    /* redo: ... ok  → infinite / while-sugar */
    if (check(p, TOK_COLON)) {
        advance(p);
        skip_newlines(p);
        /* Check for 'while cond' sugar */
        if (check(p, TOK_WHILE)) {
            advance(p);
            n->as.redo_loop.while_cond = parse_expr(p);
            skip_newlines(p);
        }
        n->as.redo_loop.body = parse_block(p);
        consume(p, TOK_OK, "expected 'ok' to close redo");
        return n;
    }

    /* redo <array> [quite] as <type> <iter>: body ok */
    n->as.redo_loop.array = parse_expr(p);

    if (check(p, TOK_QUITE)) {
        advance(p);
        n->as.redo_loop.quite = 1;
    }
    consume(p, TOK_AS, "expected 'as'");

    /* iterator type + name */
    int is_ptr = 0, arr_sz = 0;
    n->as.redo_loop.iter_type = parse_type_spec(p, &is_ptr, &arr_sz);
    if (!check(p, TOK_IDENT)) { parser_error(p, "expected iterator name"); return n; }
    n->as.redo_loop.iter_name = strdup(p->current.value);
    advance(p);

    consume(p, TOK_COLON, "expected ':' after redo header");
    skip_newlines(p);
    n->as.redo_loop.body = parse_block(p);
    consume(p, TOK_OK, "expected 'ok' to close redo");
    return n;
}

/* Function declaration tail: [<params>]: body ok */
static AstNode* parse_func_decl_tail(Parser* p, GampilType ret, int is_ptr, int arr_sz, char* name, int ln, int col) {
    (void)arr_sz; /* functions don't return static arrays in Gampil currently */
    consume(p, TOK_LBRACKET, "expected '[' in function declaration");

    AstList* params = NULL;
    while (!check(p, TOK_RBRACKET) && !check(p, TOK_EOF)) {
        if (check(p, TOK_COMMA)) { advance(p); continue; }
        if (is_type_tok(p->current.type))
            params = astlist_append(params, parse_param(p));
        else
            break;
    }
    consume(p, TOK_RBRACKET, "expected ']'");
    consume(p, TOK_COLON, "expected ':' after function signature");
    skip_newlines(p);
    AstNode* body = parse_block(p);
    consume(p, TOK_OK, "expected 'ok' to close function");

    AstNode* n = ast_new(AST_FUNC_DECL, ln, col);
    n->as.func_decl.ret_type   = ret;
    n->as.func_decl.name       = name;
    n->as.func_decl.is_pointer = is_ptr;
    n->as.func_decl.params     = params;
    n->as.func_decl.body       = body;
    return n;
}

/* Assignment / compound-assignment statement */
static AstNode* parse_assignment(Parser* p, char* target_name, int ln, int col) {
    AstNode* n = ast_new(AST_ASSIGN_STMT, ln, col);
    n->as.assign.target = target_name;
    n->as.assign.op     = p->current.type; /* be / +be / -be / … */
    advance(p);
    n->as.assign.value  = parse_expr(p);
    return n;
}

/* Generic statement dispatcher */
static AstNode* parse_stmt(Parser* p) {
    int ln = p->current.line, col = p->current.col;

    skip_newlines(p);
    if (check(p, TOK_EOF)) return NULL;

    /* Function / variable declaration: starts with type keyword */
    if (is_type_tok(p->current.type)) {
        int ln_decl = p->current.line, col_decl = p->current.col;

        /* Special check for field({types}) */
        if (check(p, TOK_FIELD) && check2(p, TOK_LPAREN)) {
            advance(p); advance(p); /* consume 'field' and '(' */
            AstList* fparams = NULL;
            if (check(p, TOK_LBRACE)) {
                advance(p);
                while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
                    if (is_type_tok(p->current.type)) {
                        int p_is_ptr = 0, p_arr_sz = 0;
                        GampilType pt = parse_type_spec(p, &p_is_ptr, &p_arr_sz);
                        AstNode* fn = ast_new(AST_VAR_DECL, p->current.line, p->current.col);
                        fn->as.var_decl.type = pt;
                        fn->as.var_decl.is_pointer = p_is_ptr;
                        fn->as.var_decl.array_size = p_arr_sz;
                        fn->as.var_decl.name = strdup("");
                        fparams = astlist_append(fparams, fn);
                    } else if (check(p, TOK_IDENT)) {
                        advance(p);
                    }
                    if (check(p, TOK_COMMA)) advance(p);
                }
                consume(p, TOK_RBRACE, "expected '}'");
            }
            consume(p, TOK_RPAREN, "expected ')'");
            AstNode* n = ast_new(AST_VAR_DECL, ln_decl, col_decl);
            n->as.var_decl.type = GTYPE_FIELD;
            if (!check(p, TOK_IDENT)) { parser_error(p, "expected identifier after field(...)"); return n; }
            n->as.var_decl.name = strdup(p->current.value);
            n->as.var_decl.field_params = fparams;
            advance(p);
            
            /* Optional 'be' initializer */
            if (check(p, TOK_BE)) {
                advance(p);
                n->as.var_decl.initializer = parse_expr(p);
            }
            return n;
        }

        int is_ptr2 = 0, arr_sz2 = 0;
        GampilType gt2 = parse_type_spec(p, &is_ptr2, &arr_sz2);
        
        if (!check(p, TOK_IDENT)) { parser_error(p, "expected identifier after type"); return NULL; }
        char* name = p->current.value ? strdup(p->current.value) : strdup("unknown");
        advance(p);

        /* If next token is '[' → function declaration */
        if (check(p, TOK_LBRACKET)) {
            return parse_func_decl_tail(p, gt2, is_ptr2, arr_sz2, name, ln_decl, col_decl);
        } else {
            return parse_var_decl_tail(p, gt2, is_ptr2, arr_sz2, name, ln_decl, col_decl);
        }
    }

    /* if-statement */
    if (check(p, TOK_IF)) return parse_if_stmt(p);

    /* redo loop */
    if (check(p, TOK_REDO)) return parse_redo_loop(p);

    /* return */
    if (check(p, TOK_RETURN)) {
        advance(p);
        AstNode* n = ast_new(AST_RETURN_STMT, ln, col);
        if (!check(p, TOK_NEWLINE) && !check(p, TOK_EOF) && !check(p, TOK_OK))
            n->as.ret.value = parse_expr(p);
        return n;
    }

    /* stop */
    if (check(p, TOK_STOP)) {
        advance(p);
        return ast_new(AST_STOP_STMT, ln, col);
    }

    /* Identifier: could be assignment or expression/call */
    if (check(p, TOK_IDENT)) {
        char* name = strdup(p->current.value);
        int   iln  = p->current.line, icol = p->current.col;
        advance(p);

        /* Assignment: name (be | +be | -be | …) expr */
        TokenType ct = p->current.type;
        if (ct == TOK_BE || ct == TOK_PLUS_BE  || ct == TOK_MINUS_BE ||
            ct == TOK_STAR_BE || ct == TOK_SLASH_BE || ct == TOK_PERCENT_BE ||
            ct == TOK_CARET_BE || ct == TOK_AMP_BE  || ct == TOK_PIPE_BE   ||
            ct == TOK_LSHIFT_BE || ct == TOK_RSHIFT_BE) {
            return parse_assignment(p, name, iln, icol);
        }

        /* Array element assignment: name(index) be expr */
        if (check(p, TOK_LPAREN)) {
            /* peek – could be index access used as lvalue */
            advance(p);
            AstNode* idx_expr = parse_expr(p);
            consume(p, TOK_RPAREN, "expected ')'");
            if (check(p, TOK_BE)) {
                advance(p);
                AstNode* n = ast_new(AST_ASSIGN_STMT, iln, icol);
                n->as.assign.target       = name;
                n->as.assign.op           = TOK_BE;
                n->as.assign.value        = parse_expr(p);
                n->as.assign.target_index = idx_expr;
                return n;
            }
            /* Not assignment — reconstruct as index expression */
            AstNode* arr = ast_new(AST_IDENT, iln, icol);
            arr->as.ident.name = name;
            AstNode* idx = ast_new(AST_INDEX_EXPR, iln, icol);
            idx->as.index.array = arr;
            idx->as.index.index = idx_expr;
            AstNode* es = ast_new(AST_EXPR_STMT, iln, icol);
            es->as.expr_stmt.expr = idx;
            return es;
        }

        /* Call: name[args] — already consumed name, put it back via ident */
        if (check(p, TOK_LBRACKET)) {
            /* Rebuild as call — reuse parse_primary logic via a fake ident node */
            advance(p); /* consume '[' */
            AstNode* n = ast_new(AST_CALL_EXPR, iln, icol);
            n->as.call.callee = name;
            AstList* args = NULL;
            while (!check(p, TOK_RBRACKET) && !check(p, TOK_EOF)) {
                if (check(p, TOK_COMMA)) { advance(p); continue; }
                if (check(p, TOK_IDENT) && check2(p, TOK_BE)) {
                    char* kn = strdup(p->current.value);
                    advance(p); advance(p);
                    AstNode* ka = ast_new(AST_ASSIGN_STMT, ln, col);
                    ka->as.assign.target = kn;
                    ka->as.assign.op     = TOK_BE;
                    ka->as.assign.value  = parse_expr(p);
                    args = astlist_append(args, ka);
                } else {
                    args = astlist_append(args, parse_expr(p));
                }
                if (check(p, TOK_COMMA)) advance(p);
            }
            consume(p, TOK_RBRACKET, "expected ']'");
            n->as.call.args = args;
            AstNode* es = ast_new(AST_EXPR_STMT, iln, icol);
            es->as.expr_stmt.expr = n;
            return es;
        }

        /* Field access then maybe assignment */
        AstNode* id = ast_new(AST_IDENT, iln, icol);
        id->as.ident.name = name;
        if (check(p, TOK_DOT)) {
            /* field access */
            advance(p);
            if (!check(p, TOK_IDENT)) { parser_error(p, "expected field name"); return id; }
            char* fname = strdup(p->current.value);
            advance(p);
            /* Could be assignment: obj.field be value */
            if (check(p, TOK_BE)) {
                advance(p);
                AstNode* n = ast_new(AST_ASSIGN_STMT, iln, icol);
                /* encode target as "name.field" */
                char tgt[256];
                snprintf(tgt, sizeof(tgt), "%s.%s", name, fname);
                free(id->as.ident.name); free(id); free(fname);
                n->as.assign.target = strdup(tgt);
                n->as.assign.op     = TOK_BE;
                n->as.assign.value  = parse_expr(p);
                return n;
            }
            AstNode* fa = ast_new(AST_FIELD_EXPR, iln, icol);
            fa->as.field_access.object = id;
            fa->as.field_access.field  = fname;
            AstNode* es = ast_new(AST_EXPR_STMT, iln, icol);
            es->as.expr_stmt.expr = fa;
            return es;
        }

        /* Just an expression */
        AstNode* es = ast_new(AST_EXPR_STMT, iln, icol);
        es->as.expr_stmt.expr = id;
        return es;
    }

    /* printf / printn / print as statements */
    if (check(p, TOK_PRINTF) || check(p, TOK_PRINTN) || check(p, TOK_PRINT)) {
        AstNode* n = parse_expr(p);
        AstNode* es = ast_new(AST_EXPR_STMT, ln, col);
        es->as.expr_stmt.expr = n;
        return es;
    }

    /* Fallback: expression statement */
    return parse_expr_stmt(p);
}

static AstNode* parse_expr_stmt(Parser* p) {
    int ln = p->current.line, col = p->current.col;
    AstNode* expr = parse_expr(p);
    if (!expr) return NULL;
    AstNode* n = ast_new(AST_EXPR_STMT, ln, col);
    n->as.expr_stmt.expr = expr;
    return n;
}

/* ── Top-level program ──────────────────────────────────────── */

AstNode* parser_parse(Parser* p) {
    AstNode* prog = ast_new(AST_PROGRAM, 1, 1);
    AstList* decls = NULL;

    /* Skip leading header line (the `GAMPIL PROGRAMMING LANGUAGE` backtick string) */
    skip_newlines(p);

    while (!check(p, TOK_EOF)) {
        skip_newlines(p);
        if (check(p, TOK_EOF)) break;
        AstNode* s = parse_stmt(p);
        if (s) decls = astlist_append(decls, s);
        while (check(p, TOK_NEWLINE)) advance(p);
    }

    prog->as.program.decls = decls;
    if (p->had_error) { ast_free(prog); return NULL; }
    return prog;
}
