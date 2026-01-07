%{
#include <stdio.h>

%}

%token OP
%token ID

%%
stmt: ID OP ID {printf("found");}
	;
%%

int main() {
    printf("Please? ");
    yylex();
    return 0;
}

int yywrap() {
    return 1;
}
