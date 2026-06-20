/* ============================================================
 *  Gampil Programming Language — Compiler Entry Point
 *  main.c
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/ast.h"
#include "../include/semantic.h"
#include "../include/codegen.h"

/* ── Usage ──────────────────────────────────────────────────── */
static void print_usage(const char* prog) {
    fprintf(stderr,
        "Gampil Compiler v1.0\n"
        "Usage: %s <input.ga> [options]\n"
        "\n"
        "Options:\n"
        "  -o <output>   Output executable name (default: a.out / a.exe)\n"
        "  -S            Output C source only, do not invoke gcc\n"
        "  -c <file>     Write C source to <file> and compile it\n"
        "  --ast         Print AST and exit (debug)\n"
        "  --tokens      Print all tokens and exit (debug)\n"
        "\n"
        "Examples:\n"
        "  %s src/example.ga -o example\n"
        "  %s src/hello.ga -S          (generates hello.ga.c)\n"
        "\n", prog, prog, prog);
}

/* ── Main ───────────────────────────────────────────────────── */
int main(int argc, char* argv[]) {
    if (argc < 2) { print_usage(argv[0]); return 1; }

    /* Parse arguments */
    const char* input_file  = NULL;
    const char* output_file = NULL;
    const char* c_file      = NULL;
    int  only_c             = 0;
    int  print_ast          = 0;
    int  print_tokens       = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i+1 < argc) {
            output_file = argv[++i];
        } else if (strcmp(argv[i], "-S") == 0) {
            only_c = 1;
        } else if (strcmp(argv[i], "-c") == 0 && i+1 < argc) {
            c_file = argv[++i];
        } else if (strcmp(argv[i], "--ast") == 0) {
            print_ast = 1;
        } else if (strcmp(argv[i], "--tokens") == 0) {
            print_tokens = 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return 1;
        } else {
            input_file = argv[i];
        }
    }

    if (!input_file) { print_usage(argv[0]); return 1; }

    /* Step 1: Read source file */
    char* source = read_file(input_file);
    if (!source) return 1;

    /* Step 2: Lex tokens (debug mode) */
    if (print_tokens) {
        Lexer* l = lexer_new(source);
        Token t;
        printf("=== TOKENS ===\n");
        while ((t = lexer_next(l)).type != TOK_EOF) {
            printf("[%3d:%2d] %-20s  %s\n",
                   t.line, t.col,
                   token_type_str(t.type),
                   t.value ? t.value : "");
            token_free(t);
        }
        token_free(t);
        lexer_free(l);
        free(source);
        return 0;
    }

    /* Step 3: Parse */
    Lexer*  lexer  = lexer_new(source);
    Parser* parser = parser_new(lexer);
    AstNode* ast   = parser_parse(parser);

    if (!ast) {
        fprintf(stderr, "Compilation failed at parse stage.\n");
        parser_free(parser); lexer_free(lexer); free(source);
        return 1;
    }

    /* Debug: print AST */
    if (print_ast) {
        ast_print(ast, 0);
        ast_free(ast);
        parser_free(parser); lexer_free(lexer); free(source);
        return 0;
    }

    /* Step 4: Semantic analysis */
    SemanticCtx* sem = semantic_new(source);
    int sem_result   = semantic_analyze(sem, ast);
    if (sem_result != 0) {
        fprintf(stderr, "Compilation failed at semantic stage.\n");
        semantic_free(sem); ast_free(ast);
        parser_free(parser); lexer_free(lexer); free(source);
        return 1;
    }
    semantic_free(sem);

    /* Step 5: Determine output C file name */
    char c_out_path[512];
    if (c_file) {
        strncpy(c_out_path, c_file, sizeof(c_out_path)-1);
    } else {
        /* Default: <inputname>.out.c */
        strncpy(c_out_path, input_file, sizeof(c_out_path)-5);
        strcat(c_out_path, ".out.c");
    }

    FILE* c_out = fopen(c_out_path, "w");
    if (!c_out) {
        fprintf(stderr, "Error: cannot open output file '%s'\n", c_out_path);
        ast_free(ast); parser_free(parser); lexer_free(lexer); free(source);
        return 1;
    }

    /* Step 6: Code generation */
    CodegenCtx* cgen = codegen_new(c_out);
    int cgen_result  = codegen_run(cgen, ast);
    fclose(c_out);
    codegen_free(cgen);
    ast_free(ast);
    parser_free(parser);
    lexer_free(lexer);
    free(source);

    if (cgen_result != 0) {
        fprintf(stderr, "Compilation failed at codegen stage.\n");
        return 1;
    }

    fprintf(stderr, "[gampil] C source written to: %s\n", c_out_path);

    /* Step 7: Invoke gcc (unless -S flag) */
    if (only_c) {
        fprintf(stderr, "[gampil] -S flag: stopping before gcc.\n");
        return 0;
    }

    /* Determine output binary name */
    char bin_out[512];
    if (output_file) {
        strncpy(bin_out, output_file, sizeof(bin_out)-1);
    } else {
        /* Default: strip .ga from input, use base name */
        strncpy(bin_out, input_file, sizeof(bin_out)-5);
        /* Remove extension if present */
        char* dot = strrchr(bin_out, '.');
        if (dot) *dot = '\0';
#ifdef _WIN32
        strncat(bin_out, ".exe", sizeof(bin_out) - strlen(bin_out) - 1);
#endif
    }

    char gcc_cmd[1024];
    snprintf(gcc_cmd, sizeof(gcc_cmd),
             "gcc -o \"%s\" \"%s\" -lm -Wall -Wextra",
             bin_out, c_out_path);

    fprintf(stderr, "[gampil] Invoking: %s\n", gcc_cmd);
    int gcc_result = system(gcc_cmd);

    if (gcc_result != 0) {
        fprintf(stderr, "[gampil] gcc failed with code %d.\n", gcc_result);
        return 1;
    }

    fprintf(stderr, "[gampil] Compiled successfully: %s\n", bin_out);
    return 0;
}
