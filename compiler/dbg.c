#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"

int main() {
    Lexer* l = lexer_new("f\"hello\"");
    Token t = lexer_next(l);
    printf("Type: %d, Value: %s\n", t.type, t.value ? t.value : "null");
    return 0;
}
