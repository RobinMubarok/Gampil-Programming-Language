%{
#include <stdio.h>
int yylex();
%}

arithmeticOperator [+\-*/%^]
relationalOperator [<>!][= ]|"="
bitwiseOperator [&|~]|"||"|">>"|"<<"
iden [A-Za-z]+[A-Za-z0-9]*
whole [0-9]+

%%
"be" {printf("%s\t: be\n", yytext); }
"if" {printf("%s\t: if\n", yytext); }
"but" {printf("%s\t: but\n", yytext); }
"else" {printf("%s\t: else\n", yytext); }
"ok" {printf("%s\t: ok\n", yytext); }
"and" {printf("%s\t: and\n", yytext); }
"or" {printf("%s\t: or\n", yytext); }
"not" {printf("%s\t: not\n", yytext); }
"redo" {printf("%s\t: redo\n", yytext); }
"quite" {printf("%s\t: quite\n", yytext); }
"as" {printf("%s\t: as\n", yytext); }
"while" {printf("%s\t: while\n", yytext); }
"return" {printf("%s\t: return\n", yytext); }
"nil" {printf("%s\t: nil\n", yytext); }
"true" {printf("%s\t: true\n", yytext); }
"false" {printf("%s\t: false\n", yytext); }
{arithmeticOperator} {printf("%s\t: math\n", yytext); }
{relationalOperator} {printf("%s\t: relational\n", yytext); }
{bitwiseOperator} {printf("%s\t: bitwise\n", yytext); } 
{whole} {printf("%s\t: whole number\n", yytext); } 
{iden}+ {printf("%s\t: iden\n", yytext); } 
" " {;};
.  {printf("error! \"%s\" is unknown\n", yytext); };
%%

int main(){
printf("what are these?\n");
yylex();
}
int yywrap() { return 1; }