#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"

int main(int argc, char** argv) {
    char* src = read_file("C:/Users/DELL/OneDrive/Desktop/projek gampil/hello.ga");
    Lexer* l = lexer_new(src);
    Token t;
    do {
        t = lexer_next(l);
        printf("Type: %d, Value: %s\n", t.type, t.value ? t.value : "null");
    } while (t.type != TOK_EOF && t.type != TOK_ERROR);
    return 0;
}
