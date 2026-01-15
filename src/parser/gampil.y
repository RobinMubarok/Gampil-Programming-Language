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

%token StringItem
%token IntegerItem
%token FloatItem

%token Other


%%
prog: 
   stmt
;

stmt: CompoundStmt
   
CompoundStmt :
             | varStmt Other CompoundStmt {printf("end");}
             | procCall Other CompoundStmt {printf("end");}

varStmt:
   Iden Be Expr {printf("Var");}

procCall:
   Iden '[' Args ']' {printf("Proc");}

Args:
    | Expr
    | Expr ',' Args

Expr: Iden
    | Asc
    | Number

Asc: StringItem {printf("String");}

Number: IntegerItem {printf("Integer");}
      | FloatItem {printf("Float");}
 

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

