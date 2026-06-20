#ifndef GAMPIL_TOKEN_H
#define GAMPIL_TOKEN_H

/* ============================================================
 *  Gampil Programming Language — Token Definitions
 *  token.h
 * ============================================================ */

typedef enum {
    /* ── Literals ─────────────────────────────────────────── */
    TOK_INT_LIT,        /* 42, 0x1F, 0b101, 0o77              */
    TOK_FLOAT_LIT,      /* 3.14, 1e10                          */
    TOK_STR_DOUBLE,     /* "hello"                             */
    TOK_STR_SINGLE,     /* 'hello'                             */
    TOK_TRUE,           /* true                                */
    TOK_FALSE,          /* false                               */
    TOK_NIL,            /* nil                                 */

    /* ── Keywords ─────────────────────────────────────────── */
    TOK_BE,             /* be                                  */
    TOK_IF,             /* if                                  */
    TOK_BUT,            /* but                                 */
    TOK_ELSE,           /* else                                */
    TOK_OK,             /* ok                                  */
    TOK_AND,            /* and                                 */
    TOK_OR,             /* or                                  */
    TOK_NOT,            /* not                                 */
    TOK_REDO,           /* redo                                */
    TOK_QUITE,          /* quite                               */
    TOK_AS,             /* as                                  */
    TOK_WHILE,          /* while                               */
    TOK_RETURN,         /* return                              */
    TOK_STOP,           /* stop                                */
    TOK_LET,            /* let  — Python dynamic type          */
    TOK_MALLOC,         /* malloc                              */

    /* ── Type keywords ───────────────────────────────────── */
    TOK_BITOFF,         /* bitOff  (void / no return)          */
    TOK_BITON,          /* bitOn   (boolean)                   */
    TOK_ASC8,           /* asc8    (unsigned char)             */
    TOK_ASC16,          /* asc16   (unsigned short)            */
    TOK_ASC32,          /* asc32   (unsigned int)              */
    TOK_NUM16,          /* num16   (signed int)                */
    TOK_NUM32,          /* num32   (long)                      */
    TOK_NUM64,          /* num64   (long long)                 */
    TOK_RAT32,          /* rat32   (float)                     */
    TOK_RAT64,          /* rat64   (double)                    */
    TOK_RAT128,         /* rat128  (long double)               */
    TOK_FIELD,          /* field   (struct / table)            */

    /* ── Builtin functions ──────────────────────────────── */
    TOK_PRINTF,         /* printf  — formatted output          */
    TOK_PRINTN,         /* printn  — println                   */
    TOK_PRINT,          /* print   — basic print               */

    /* ── Operators ──────────────────────────────────────── */
    TOK_PLUS,           /* +                                   */
    TOK_MINUS,          /* -                                   */
    TOK_STAR,           /* *                                   */
    TOK_SLASH,          /* /                                   */
    TOK_PERCENT,        /* %                                   */
    TOK_CARET,          /* ^   (power)                         */

    TOK_LT,             /* <                                   */
    TOK_GT,             /* >                                   */
    TOK_EQ,             /* =   (equality)                      */
    TOK_NEQ,            /* !=                                  */
    TOK_LTE,            /* <=                                  */
    TOK_GTE,            /* >=                                  */

    TOK_AMP,            /* &   (bitwise AND)                   */
    TOK_PIPE,           /* |   (bitwise OR)                    */
    TOK_DPIPE,          /* ||  (bitwise XOR)                   */
    TOK_TILDE,          /* ~   (bitwise NOT)                   */
    TOK_LSHIFT,         /* <<                                  */
    TOK_RSHIFT,         /* >>                                  */
    TOK_BANG,           /* !   (logical NOT prefix)            */

    /* ── Compound assignment operators  (op + "be") ─────── */
    TOK_PLUS_BE,        /* +be                                 */
    TOK_MINUS_BE,       /* -be                                 */
    TOK_STAR_BE,        /* *be                                 */
    TOK_SLASH_BE,       /* /be                                 */
    TOK_PERCENT_BE,     /* %be                                 */
    TOK_CARET_BE,       /* ^be                                 */
    TOK_AMP_BE,         /* &be                                 */
    TOK_PIPE_BE,        /* |be                                 */
    TOK_LSHIFT_BE,      /* <<be                                */
    TOK_RSHIFT_BE,      /* >>be                                */

    /* ── Punctuation ─────────────────────────────────────── */
    TOK_LBRACKET,       /* [                                   */
    TOK_RBRACKET,       /* ]                                   */
    TOK_LPAREN,         /* (                                   */
    TOK_RPAREN,         /* )                                   */
    TOK_LBRACE,         /* {                                   */
    TOK_RBRACE,         /* }                                   */
    TOK_COLON,          /* :                                   */
    TOK_COMMA,          /* ,                                   */
    TOK_SEMICOLON,      /* ;   (same as newline)               */
    TOK_DOT,            /* .   (field access)                  */
    TOK_AT,             /* @   (address-of / pointer)          */

    /* ── Structural ──────────────────────────────────────── */
    TOK_NEWLINE,        /* \n   (statement separator)          */
    TOK_IDENT,          /* identifier                          */
    TOK_COMMENT,        /* comment (skipped by parser)         */
    TOK_EOF,            /* end of file                         */
    TOK_ERROR           /* unknown / error token               */
} TokenType;

/* String representation of token type (for debug) */
const char* token_type_str(TokenType t);

/* ── Token struct ─────────────────────────────────────────── */
typedef struct Token {
    TokenType   type;
    char*       value;   /* heap-allocated, NULL for punctuation */
    int         line;
    int         col;
} Token;

#endif /* GAMPIL_TOKEN_H */
