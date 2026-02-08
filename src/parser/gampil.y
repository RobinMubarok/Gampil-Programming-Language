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

%token TwoContent
%token TwoSta
%token TwoEnd
%token OneContent
%token CharQuOne
%token CharQu
%token OneSta
%token OneEnd

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
   Iden Be Expr {printf("Var:\n");}

procCall:
   Iden '[' Args ']' {printf("Proc:\n");}

Args:
    | Expr
    | Expr ',' Args

Expr: Iden
    | Asc
    | Number
    | Boolean

Asc: TwoSta TwoStrContent TwoEnd
   | OneSta OneStrContent OneEnd

TwoStrContent: 
             | TwoContent TwoStrContent

OneStrContent: 
             | OneContent OneStrContent

Number: IntegerItem {printf("\tInteger\n");}
      | FloatItem {printf("\tFloat\n");}

Boolean: True {printf("\tTrue\n");}
       | False {printf("\tFalse\n");}
 

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

