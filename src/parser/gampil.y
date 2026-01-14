%{
#include <stdio.h>
#include <string.h>

int yylex(void);
int yyerror(char* s);
%}

%token Assignment 
%token ArithmeticOperator 
%token RelationalOperator 
%token BitwiseOperator
%token Iden
%token Whole
%token Be
%token If
%token But
%token Else
%token Ok
%token And
%token Or
%token Not
%token Redo
%token Quite
%token As
%token While
%token Return
%token Nil
%token True
%token False
%token SingleComment 
%token MultiComment 
%token Other


%%
prog: 
   stmt
;

stmt: 
   | varStmt Other stmt {printf("end");}
   
   | procCall Other stmt {printf("end");}

varStmt:
   Iden Be Whole {printf("var");}

procCall:
   Iden '[' Iden ']' {printf("proc");}

%%

int main() {
    printf("Please? ");
    yyparse();
    return 0;
}

int yyerror(char *s){
    fprintf(stderr,"%s\n",s);
    return 0;
}

