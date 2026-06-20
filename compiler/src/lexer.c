/* ============================================================
 *  Gampil Programming Language — Lexer Implementation
 *  lexer.c
 * ============================================================ */

#include "../include/lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ── File utilities ─────────────────────────────────────────── */
char* read_file(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "Error: cannot open '%s'\n", path); return NULL; }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);
    char* buf = (char*)malloc(len + 1);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);
    return buf;
}

/* ── Token type name (for debug) ────────────────────────────── */
const char* token_type_str(TokenType t) {
    switch (t) {
#define X(n) case n: return #n;
        X(TOK_INT_LIT)   X(TOK_FLOAT_LIT) X(TOK_STR_DOUBLE) X(TOK_STR_SINGLE)
        X(TOK_TRUE)      X(TOK_FALSE)      X(TOK_NIL)
        X(TOK_BE)        X(TOK_IF)         X(TOK_BUT)        X(TOK_ELSE)
        X(TOK_OK)        X(TOK_AND)        X(TOK_OR)         X(TOK_NOT)
        X(TOK_REDO)      X(TOK_QUITE)      X(TOK_AS)         X(TOK_WHILE)
        X(TOK_RETURN)    X(TOK_STOP)       X(TOK_LET)        X(TOK_MALLOC)
        X(TOK_BITOFF)    X(TOK_BITON)
        X(TOK_ASC8)      X(TOK_ASC16)      X(TOK_ASC32)
        X(TOK_NUM16)     X(TOK_NUM32)      X(TOK_NUM64)
        X(TOK_RAT32)     X(TOK_RAT64)      X(TOK_RAT128)
        X(TOK_FIELD)     X(TOK_PRINTF)     X(TOK_PRINTN)     X(TOK_PRINT)
        X(TOK_PLUS)      X(TOK_MINUS)      X(TOK_STAR)       X(TOK_SLASH)
        X(TOK_PERCENT)   X(TOK_CARET)
        X(TOK_LT)        X(TOK_GT)         X(TOK_EQ)         X(TOK_NEQ)
        X(TOK_LTE)       X(TOK_GTE)
        X(TOK_AMP)       X(TOK_PIPE)       X(TOK_DPIPE)      X(TOK_TILDE)
        X(TOK_LSHIFT)    X(TOK_RSHIFT)     X(TOK_BANG)
        X(TOK_PLUS_BE)   X(TOK_MINUS_BE)   X(TOK_STAR_BE)    X(TOK_SLASH_BE)
        X(TOK_PERCENT_BE) X(TOK_CARET_BE)  X(TOK_AMP_BE)     X(TOK_PIPE_BE)
        X(TOK_LSHIFT_BE) X(TOK_RSHIFT_BE)
        X(TOK_LBRACKET)  X(TOK_RBRACKET)   X(TOK_LPAREN)     X(TOK_RPAREN)
        X(TOK_LBRACE)    X(TOK_RBRACE)
        X(TOK_COLON)     X(TOK_COMMA)      X(TOK_SEMICOLON)  X(TOK_DOT)
        X(TOK_AT)        X(TOK_NEWLINE)    X(TOK_IDENT)      X(TOK_COMMENT)
        X(TOK_EOF)       X(TOK_ERROR)
#undef X
        default: return "UNKNOWN";
    }
}

void token_free(Token t) {
    if (t.value) free(t.value);
}

/* ── Lexer internal helpers ─────────────────────────────────── */

Lexer* lexer_new(const char* source) {
    Lexer* l = (Lexer*)calloc(1, sizeof(Lexer));
    l->source = source;
    l->length = strlen(source);
    l->pos    = 0;
    l->line   = 1;
    l->col    = 1;
    return l;
}

void lexer_free(Lexer* l) { free(l); }

static char peek_ch(Lexer* l) {
    if (l->pos >= l->length) return '\0';
    return l->source[l->pos];
}
static char peek2_ch(Lexer* l) {
    if (l->pos + 1 >= l->length) return '\0';
    return l->source[l->pos + 1];
}
static char advance_ch(Lexer* l) {
    char c = l->source[l->pos++];
    if (c == '\n') { l->line++; l->col = 1; }
    else            l->col++;
    return c;
}
static int  at_end(Lexer* l) { return l->pos >= l->length; }

/* Make a token with a heap-allocated value copy */
static Token make_tok(TokenType t, const char* val, int line, int col) {
    Token tok;
    tok.type  = t;
    tok.value = val ? strdup(val) : NULL;
    tok.line  = line;
    tok.col   = col;
    return tok;
}

/* Skip whitespace (not newlines) and carriage returns */
static void skip_spaces(Lexer* l) {
    while (!at_end(l) && (peek_ch(l) == ' ' || peek_ch(l) == '\t' || peek_ch(l) == '\r'))
        advance_ch(l);
}

/* ── Keyword table ──────────────────────────────────────────── */
typedef struct { const char* word; TokenType type; } KwEntry;
static KwEntry kw_table[] = {
    {"be",     TOK_BE},     {"if",     TOK_IF},    {"but",    TOK_BUT},
    {"else",   TOK_ELSE},   {"ok",     TOK_OK},    {"and",    TOK_AND},
    {"or",     TOK_OR},     {"not",    TOK_NOT},   {"redo",   TOK_REDO},
    {"quite",  TOK_QUITE},  {"as",     TOK_AS},    {"while",  TOK_WHILE},
    {"return", TOK_RETURN}, {"stop",   TOK_STOP},  {"nil",    TOK_NIL},
    {"true",   TOK_TRUE},   {"false",  TOK_FALSE}, {"let",    TOK_LET},
    {"malloc", TOK_MALLOC},
    /* Type keywords */
    {"bitOff", TOK_BITOFF}, {"bitOn",  TOK_BITON},
    {"asc8",   TOK_ASC8},   {"asc16",  TOK_ASC16}, {"asc32",  TOK_ASC32},
    {"num16",  TOK_NUM16},  {"num32",  TOK_NUM32},  {"num64",  TOK_NUM64},
    {"rat32",  TOK_RAT32},  {"rat64",  TOK_RAT64},  {"rat128", TOK_RAT128},
    {"field",  TOK_FIELD},
    /* Builtins */
    {"printf", TOK_PRINTF}, {"printn", TOK_PRINTN}, {"print",  TOK_PRINT},
    {NULL,     TOK_ERROR}
};

static TokenType lookup_keyword(const char* word) {
    for (int i = 0; kw_table[i].word; i++)
        if (strcmp(kw_table[i].word, word) == 0)
            return kw_table[i].type;
    return TOK_IDENT;
}

/* ── String reading ─────────────────────────────────────────── */
static Token read_string(Lexer* l, char delim, int line, int col) {
    /* consume opening quote (already peeked) */
    advance_ch(l);
    char buf[4096];
    int  bi = 0;
    while (!at_end(l) && peek_ch(l) != delim) {
        char c = advance_ch(l);
        if (c == '\\') {
            if (at_end(l)) break;
            char esc = advance_ch(l);
            switch (esc) {
                case 'n':  buf[bi++] = '\n'; break;
                case 't':  buf[bi++] = '\t'; break;
                case 'r':  buf[bi++] = '\r'; break;
                case '\\': buf[bi++] = '\\'; break;
                case '"':  buf[bi++] = '"';  break;
                case '\'': buf[bi++] = '\''; break;
                default:   buf[bi++] = '\\'; buf[bi++] = esc; break;
            }
        } else {
            buf[bi++] = c;
        }
        if (bi >= 4090) break;
    }
    if (!at_end(l)) advance_ch(l); /* closing quote */
    buf[bi] = '\0';
    return make_tok(delim == '"' ? TOK_STR_DOUBLE : TOK_STR_SINGLE, buf, line, col);
}

/* ── Number reading ─────────────────────────────────────────── */
static Token read_number(Lexer* l, int line, int col) {
    char buf[128];
    int  bi = 0;
    int  is_float = 0;

    /* Check prefix: 0x, 0b, 0o */
    if (peek_ch(l) == '0' && !at_end(l)) {
        char next = peek2_ch(l);
        if (next == 'x' || next == 'X') {
            buf[bi++] = advance_ch(l); buf[bi++] = advance_ch(l);
            while (!at_end(l) && (isxdigit(peek_ch(l)) || peek_ch(l) == '_'))
                { char c = advance_ch(l); if (c != '_') buf[bi++] = c; }
            buf[bi] = '\0';
            return make_tok(TOK_INT_LIT, buf, line, col);
        }
        if (next == 'b' || next == 'B') {
            buf[bi++] = advance_ch(l); buf[bi++] = advance_ch(l);
            while (!at_end(l) && (peek_ch(l) == '0' || peek_ch(l) == '1' || peek_ch(l) == '_'))
                { char c = advance_ch(l); if (c != '_') buf[bi++] = c; }
            buf[bi] = '\0';
            return make_tok(TOK_INT_LIT, buf, line, col);
        }
        if (next == 'o' || next == 'O') {
            buf[bi++] = advance_ch(l); buf[bi++] = advance_ch(l);
            while (!at_end(l) && ((peek_ch(l) >= '0' && peek_ch(l) <= '7') || peek_ch(l) == '_'))
                { char c = advance_ch(l); if (c != '_') buf[bi++] = c; }
            buf[bi] = '\0';
            return make_tok(TOK_INT_LIT, buf, line, col);
        }
    }

    /* Decimal / float */
    while (!at_end(l) && (isdigit(peek_ch(l)) || peek_ch(l) == '_'))
        { char c = advance_ch(l); if (c != '_') buf[bi++] = c; }

    if (!at_end(l) && peek_ch(l) == '.') {
        is_float = 1;
        buf[bi++] = advance_ch(l);
        while (!at_end(l) && (isdigit(peek_ch(l)) || peek_ch(l) == '_'))
            { char c = advance_ch(l); if (c != '_') buf[bi++] = c; }
    }
    if (!at_end(l) && (peek_ch(l) == 'e' || peek_ch(l) == 'E')) {
        is_float = 1;
        buf[bi++] = advance_ch(l);
        if (!at_end(l) && (peek_ch(l) == '+' || peek_ch(l) == '-'))
            buf[bi++] = advance_ch(l);
        while (!at_end(l) && isdigit(peek_ch(l)))
            buf[bi++] = advance_ch(l);
    }
    buf[bi] = '\0';
    return make_tok(is_float ? TOK_FLOAT_LIT : TOK_INT_LIT, buf, line, col);
}

/* ── Single-line comment  \ ... ─────────────────────────────── */
static void skip_single_comment(Lexer* l) {
    while (!at_end(l) && peek_ch(l) != '\n')
        advance_ch(l);
}

/* ── Multi-line comment  `...` ──────────────────────────────── */
static void skip_multi_comment(Lexer* l) {
    advance_ch(l); /* consume opening backtick */
    while (!at_end(l) && peek_ch(l) != '`')
        advance_ch(l);
    if (!at_end(l)) advance_ch(l); /* consume closing backtick */
}

/* ── Check if "be" follows (for compound assignment) ────────── */
static int follows_be(Lexer* l) {
    /* skip spaces */
    size_t saved = l->pos;
    int saved_line = l->line, saved_col = l->col;
    while (!at_end(l) && (peek_ch(l) == ' ' || peek_ch(l) == '\t'))
        advance_ch(l);
    int res = 0;
    if (!at_end(l) && l->pos + 1 < l->length &&
        l->source[l->pos] == 'b' && l->source[l->pos+1] == 'e' &&
        (l->pos + 2 >= l->length || !isalnum(l->source[l->pos+2]))) {
        res = 1;
        l->pos += 2; l->col += 2;
    } else {
        l->pos = saved; l->line = saved_line; l->col = saved_col;
    }
    return res;
}

/* ── Core: produce one token ────────────────────────────────── */
static Token scan_one(Lexer* l) {
    skip_spaces(l);
    if (at_end(l)) return make_tok(TOK_EOF, NULL, l->line, l->col);

    int line = l->line, col = l->col;
    char c = peek_ch(l);

    /* Newline / semicolon → statement separator */
    if (c == '\n' || c == ';') {
        advance_ch(l);
        return make_tok(TOK_NEWLINE, NULL, line, col);
    }

    /* Single-line comment: \ */
    if (c == '\\') {
        advance_ch(l);
        skip_single_comment(l);
        return make_tok(TOK_COMMENT, NULL, line, col);
    }

    /* Multi-line comment: ` ... ` */
    if (c == '`') {
        skip_multi_comment(l);
        return make_tok(TOK_COMMENT, NULL, line, col);
    }

    /* String literals */
    if (c == '"') return read_string(l, '"', line, col);
    if (c == '\'') return read_string(l, '\'', line, col);

    /* Numbers */
    if (isdigit(c)) return read_number(l, line, col);

    /* Minus sign: could be negative number or operator */
    if (c == '-') {
        advance_ch(l);
        /* check compound assignment: -be */
        if (follows_be(l)) return make_tok(TOK_MINUS_BE, NULL, line, col);
        return make_tok(TOK_MINUS, NULL, line, col);
    }

    /* Identifiers and keywords */
    if (isalpha(c) || c == '_') {
        char buf[256]; int bi = 0;
        while (!at_end(l) && (isalnum(peek_ch(l)) || peek_ch(l) == '_'))
            buf[bi++] = advance_ch(l);
        buf[bi] = '\0';
        TokenType kw = lookup_keyword(buf);
        return make_tok(kw, kw == TOK_IDENT ? buf : NULL, line, col);
    }

    /* Address-of / pointer dereference */
    if (c == '@') { advance_ch(l); return make_tok(TOK_AT, NULL, line, col); }

    /* Two-character operators */
    if (c == '<') {
        advance_ch(l);
        if (!at_end(l) && peek_ch(l) == '<') {
            advance_ch(l);
            if (follows_be(l)) return make_tok(TOK_LSHIFT_BE, NULL, line, col);
            return make_tok(TOK_LSHIFT, NULL, line, col);
        }
        if (!at_end(l) && peek_ch(l) == '=') { advance_ch(l); return make_tok(TOK_LTE, NULL, line, col); }
        return make_tok(TOK_LT, NULL, line, col);
    }
    if (c == '>') {
        advance_ch(l);
        if (!at_end(l) && peek_ch(l) == '>') {
            advance_ch(l);
            if (follows_be(l)) return make_tok(TOK_RSHIFT_BE, NULL, line, col);
            return make_tok(TOK_RSHIFT, NULL, line, col);
        }
        if (!at_end(l) && peek_ch(l) == '=') { advance_ch(l); return make_tok(TOK_GTE, NULL, line, col); }
        return make_tok(TOK_GT, NULL, line, col);
    }
    if (c == '!') {
        advance_ch(l);
        if (!at_end(l) && peek_ch(l) == '=') { advance_ch(l); return make_tok(TOK_NEQ, NULL, line, col); }
        return make_tok(TOK_BANG, NULL, line, col);
    }
    if (c == '|') {
        advance_ch(l);
        if (!at_end(l) && peek_ch(l) == '|') {
            advance_ch(l);
            if (follows_be(l)) return make_tok(TOK_PIPE_BE, NULL, line, col);
            return make_tok(TOK_DPIPE, NULL, line, col);
        }
        if (follows_be(l)) return make_tok(TOK_PIPE_BE, NULL, line, col);
        return make_tok(TOK_PIPE, NULL, line, col);
    }
    if (c == '&') {
        advance_ch(l);
        if (follows_be(l)) return make_tok(TOK_AMP_BE, NULL, line, col);
        return make_tok(TOK_AMP, NULL, line, col);
    }

    /* Single-char operators with optional compound assignment */
#define MAYBE_COMPOUND(ch, plain, compound) \
    if (c == (ch)) { advance_ch(l); \
        if (follows_be(l)) return make_tok(compound, NULL, line, col); \
        return make_tok(plain, NULL, line, col); }

    MAYBE_COMPOUND('+', TOK_PLUS,    TOK_PLUS_BE)
    MAYBE_COMPOUND('*', TOK_STAR,    TOK_STAR_BE)
    MAYBE_COMPOUND('/', TOK_SLASH,   TOK_SLASH_BE)
    MAYBE_COMPOUND('%', TOK_PERCENT, TOK_PERCENT_BE)
    MAYBE_COMPOUND('^', TOK_CARET,   TOK_CARET_BE)
#undef MAYBE_COMPOUND

    /* Pure single-char */
    advance_ch(l);
    switch (c) {
        case '=': return make_tok(TOK_EQ,        NULL, line, col);
        case '~': return make_tok(TOK_TILDE,      NULL, line, col);
        case '[': return make_tok(TOK_LBRACKET,   NULL, line, col);
        case ']': return make_tok(TOK_RBRACKET,   NULL, line, col);
        case '(': return make_tok(TOK_LPAREN,     NULL, line, col);
        case ')': return make_tok(TOK_RPAREN,     NULL, line, col);
        case '{': return make_tok(TOK_LBRACE,     NULL, line, col);
        case '}': return make_tok(TOK_RBRACE,     NULL, line, col);
        case ':': return make_tok(TOK_COLON,      NULL, line, col);
        case ',': return make_tok(TOK_COMMA,      NULL, line, col);
        case '.': return make_tok(TOK_DOT,        NULL, line, col);
        default: {
            char err[4] = {c, '\0'};
            return make_tok(TOK_ERROR, err, line, col);
        }
    }
}

/* ── Public: lexer_next — skip comments, return real tokens ─── */
Token lexer_next(Lexer* l) {
    Token t;
    do {
        t = scan_one(l);
    } while (t.type == TOK_COMMENT);
    return t;
}

/* ── Public: lexer_peek ─────────────────────────────────────── */
Token lexer_peek(Lexer* l) {
    size_t saved_pos  = l->pos;
    int    saved_line = l->line;
    int    saved_col  = l->col;
    Token t = lexer_next(l);
    /* restore (but t.value is heap — must free caller-side) */
    l->pos  = saved_pos;
    l->line = saved_line;
    l->col  = saved_col;
    return t;
}
