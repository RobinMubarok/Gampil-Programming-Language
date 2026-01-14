%{
#include <stdio.h>
#include "../parser/y.tab.h"

%}

singleComment [\\].*
multiComment [`][^`]*[`]  

/*
Source : https://stackoverflow.com/questions/25395251/detecting-and-skipping-line-comments-with-flex
*/

assignment ({arithmeticOperator}|{bitwiseOperator})"be"
arithmeticOperator [+\-*/%^]
relationalOperator [<>][=]?|"!="|"="
bitwiseOperator [&|]|"||"|">>"|"<<"
unaryOperator [~!]
iden [_A-Za-z]+[_A-Za-z0-9]*
whole [0-9]+

%%
{singleComment} {return SingleComment; }
{multiComment} {return MultiComment; }
{assignment} {return Assignment; }
"be" {return Be; }
"if" {return If; }
"but" {return But; }
"else" {return Else; }
"ok" {return Ok; }
"and" {return And; }
"or" {return Or; }
"not"|{unaryOperator} {return Not; }
"redo" {return Redo; }
"quite" {return Quite; }
"as" {return As; }
"while" {return While; }
"return" {return Return; }
"nil" {return Nil; }
"true" {return True; }
"false" {return False; }
{arithmeticOperator} {return ArithmeticOperator; }
{relationalOperator} {return RelationalOperator; }
{bitwiseOperator} {return BitwiseOperator; } 
{whole} {return Whole; } 
{iden}+ {return Iden; }
[\n\;]+ {return Other;}
[\t ] {;}
.  {return *yytext; };
%%

int yywrap() {
    return 1;
}
