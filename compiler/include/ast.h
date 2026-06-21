#ifndef GAMPIL_AST_H
#define GAMPIL_AST_H

/* ============================================================
 *  Gampil Programming Language — Abstract Syntax Tree
 *  ast.h
 * ============================================================ */

#include "token.h"
#include <stddef.h>

/* ── Forward declarations ──────────────────────────────────── */
typedef struct AstNode   AstNode;
typedef struct AstList   AstList;
typedef struct GuardClause GuardClause;

/* ── Node kinds ────────────────────────────────────────────── */
typedef enum {
    /* Top-level */
    AST_PROGRAM,

    /* Declarations */
    AST_FUNC_DECL,      /* type name[params]: body ok              */
    AST_VAR_DECL,       /* type name be expr | type(size) name     */
    AST_PARAM,          /* parameter (type name [be default])      */

    /* Statements */
    AST_IF_STMT,        /* if guards ok                            */
    AST_REDO_LOOP,      /* redo [arr [quite] as T i]: body ok      */
    AST_RETURN_STMT,    /* return [expr]                           */
    AST_STOP_STMT,      /* stop                                    */
    AST_EXPR_STMT,      /* expression used as statement            */
    AST_ASSIGN_STMT,    /* name be expr  /  name +be expr  etc.    */
    AST_BLOCK,          /* sequence of statements                  */

    /* Expressions */
    AST_BINARY_EXPR,    /* left op right                           */
    AST_UNARY_EXPR,     /* op operand                              */
    AST_CALL_EXPR,      /* callee[args]                            */
    AST_INDEX_EXPR,     /* array(index)                            */
    AST_FIELD_EXPR,     /* obj.field                               */
    AST_IDENT,          /* identifier reference                    */
    AST_INT_LIT,        /* integer literal                         */
    AST_FLOAT_LIT,      /* float literal                           */
    AST_COMPLEX_LIT,    /* complex literal                         */
    AST_STR_LIT,        /* string literal                          */
    AST_BOOL_LIT,       /* true / false                            */
    AST_NIL_LIT,        /* nil                                     */
    AST_TABLE_LIT,      /* { elements }                            */
    AST_ADDR_OF,        /* @var — address-of / pointer             */

    /* Builtin calls (treated specially in codegen) */
    AST_PRINTF_CALL,    /* printf[fmt, ...]                        */
    AST_PRINTN_CALL,    /* printn[...]                             */
    AST_PRINT_CALL,     /* print[...]                              */
    AST_MALLOC_CALL,    /* malloc[n]                               */
    AST_CAST_EXPR,      /* type()[expr]                            */

    /* Python runtime call — for `let` types */
    AST_PYRUNTIME_STMT, /* any statement involving `let` vars      */
} AstKind;

/* ── Gampil type representation ──────────────────────────────*/
typedef enum {
    GTYPE_BITOFF,  GTYPE_BITON,
    GTYPE_ASC8,    GTYPE_ASC16,    GTYPE_ASC32,
    GTYPE_ASC64,
    GTYPE_NUM8,
    GTYPE_NUM16,   GTYPE_NUM32,   GTYPE_NUM64,
    GTYPE_RAT32,   GTYPE_RAT64,   GTYPE_RAT128,
    GTYPE_FIELD,   /* struct-like */
    GTYPE_VOID,
    GTYPE_DYNAMIC, /* Python `let` */
    GTYPE_UNKNOWN
} GampilType;

/* ── Linked list of AST nodes ──────────────────────────────── */
struct AstList {
    AstNode*  node;
    AstList*  next;
};

/* ── Guard clause for if-statement ─────────────────────────── */
struct GuardClause {
    int      is_else_and;  /* 1 = "else and <cond>", prevents parallel */
    AstNode* cond;         /* condition expression (NULL for else-only) */
    AstNode* body;         /* AST_BLOCK body                           */
    GuardClause* next;
};

/* ── Main AST node ──────────────────────────────────────────── */
struct AstNode {
    AstKind kind;
    int     line;
    int     col;

    union {
        /* AST_PROGRAM */
        struct { AstList* decls; } program;

        /* AST_FUNC_DECL */
        struct {
            GampilType ret_type;
            char*      name;
            int        is_pointer;   /* ret type is pointer */
            AstList*   params;       /* list of AST_PARAM   */
            AstNode*   body;         /* AST_BLOCK           */
        } func_decl;

        /* AST_VAR_DECL */
        struct {
            GampilType type;
            char*      name;
            int        is_pointer;   /* num32() → pointer   */
            int        array_size;   /* num16(3) → 3; 0=dynamic */
            AstNode*   initializer;  /* NULL if uninitialized   */
            AstList*   field_params; /* for field({types}) var  */
        } var_decl;

        /* AST_PARAM */
        struct {
            GampilType type;
            char*      name;
            int        is_pointer;
            AstNode*   default_val; /* NULL if no default       */
        } param;

        /* AST_IF_STMT */
        struct {
            GuardClause* guards;   /* linked list of guard clauses */
        } if_stmt;

        /* AST_REDO_LOOP */
        struct {
            AstNode*   array;      /* NULL for infinite loop       */
            int        quite;      /* 1 = redo arr quite as T i    */
            char*      iter_name;  /* iterator variable name       */
            GampilType iter_type;  /* iterator type                */
            AstNode*   while_cond; /* "while" sugar condition      */
            AstNode*   body;       /* AST_BLOCK                    */
        } redo_loop;

        /* AST_RETURN_STMT */
        struct { AstNode* value; } ret;  /* NULL → void return     */

        /* AST_ASSIGN_STMT */
        struct {
            char*      target;     /* variable name                */
            TokenType  op;         /* TOK_BE, TOK_PLUS_BE, etc.   */
            AstNode*   value;
            AstNode*   target_index; /* for array assign: arr(i)  */
        } assign;

        /* AST_BLOCK */
        struct { AstList* stmts; } block;

        /* AST_BINARY_EXPR */
        struct {
            TokenType op;
            AstNode*  left;
            AstNode*  right;
        } binary;

        /* AST_UNARY_EXPR */
        struct {
            TokenType op;
            AstNode*  operand;
        } unary;

        /* AST_CALL_EXPR / AST_PRINTF_CALL / AST_PRINTN_CALL / etc. */
        struct {
            char*    callee;
            AstList* args;     /* each arg is an AstNode* (expr or assign) */
        } call;

        /* AST_INDEX_EXPR */
        struct {
            AstNode* array;
            AstNode* index;
        } index;

        /* AST_FIELD_EXPR */
        struct {
            AstNode* object;
            char*    field;
        } field_access;

        /* AST_IDENT */
        struct { char* name; } ident;

        /* AST_INT_LIT */
        struct { long long value; } int_lit;

        /* AST_FLOAT_LIT */
        struct { double value; } float_lit;

        /* AST_STR_LIT */
        struct {
            char* value;
            char* prefix;
            char  delim;
            int   is_triple;
        } str_lit;

        /* AST_COMPLEX_LIT */
        struct { char* value; } complex_lit;

        /* AST_BOOL_LIT */
        struct { int value; } bool_lit; /* 1=true, 0=false */

        /* AST_TABLE_LIT: { expr, expr, ... } or { varDecl, ... } */
        struct { AstList* elements; } table;

        /* AST_ADDR_OF */
        struct { char* var; } addr_of;

        /* AST_PYRUNTIME_STMT: raw source snippet to delegate */
        struct { char* snippet; } pyruntime;

        /* AST_MALLOC_CALL */
        struct { AstNode* size_expr; } malloc_call;
        
        /* AST_CAST_EXPR */
        struct {
            GampilType target_type;
            int        is_pointer;
            AstNode*   expr;
        } cast_expr;

        /* AST_EXPR_STMT */
        struct { AstNode* expr; } expr_stmt;

    } as;
};

/* ── AST helpers ────────────────────────────────────────────── */
AstNode*  ast_new(AstKind kind, int line, int col);
AstList*  astlist_append(AstList* list, AstNode* node);
AstList*  astlist_prepend(AstList* list, AstNode* node);
int       astlist_len(AstList* list);
void      ast_print(AstNode* node, int indent); /* debug pretty-print */
void      ast_free(AstNode* node);

/* Type helpers */
const char* gtype_to_c(GampilType t);   /* "int", "long", etc.   */
const char* gtype_name(GampilType t);   /* "num16", "rat32", etc. */
GampilType  tok_to_gtype(TokenType t);  /* token → GampilType    */
int         gtype_is_dynamic(GampilType t); /* 1 if `let`         */

#endif /* GAMPIL_AST_H */
