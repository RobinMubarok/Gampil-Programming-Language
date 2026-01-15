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

stringQuote "\'\'\'"|"\"\"\""
stringChar [^\'\"\\\n]
stringEscapeSeq [\\]{sourceCharacters}
sourceCharacters [\\\'\"abfnrtv]|"ooo"|"xhh"|"N"{iden}|"uxxxx"|"Uxxxxxxxx"
stringItem {stringEscapeSeq}|{stringChar}

integer {intPart}|[\-]{intPart} 
float {floatPart}|[\-]{floatPart}
floatPart ({digitPart}[.]{digitPart}?{exponent}?)|([.]{digitPart}{exponent}?)|({digitPart}{exponent})
exponent [Ee][+\-]{digitPart}
digitPart {whole}([_]{whole})*
intPart {decimal}|{binary}|{octal}|{hexDec}|{zeroInt}
zeroInt [0]+([_][0])*
hexDec [0][Xx]([_]{hexDigit})+
octal [0][Oo]([_]{octDigit})+
binary [0][Bb]([_]{binDigit})+
decimal {natural}([_]{whole})*
hexDigit {whole}|[a-fA-F]
octDigit [0-7]
binDigit [0-1]
natural [1-9]

%%
(([\']{stringItem}*[\'])|([\"]{stringItem}*[\"]))+ { return StringItem; }
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
{integer} {return IntegerItem; } 
{float} {return FloatItem; }
{arithmeticOperator} {return ArithmeticOperator; }
{relationalOperator} {return RelationalOperator; }
{bitwiseOperator} {return BitwiseOperator; } 
{iden}+ { return Iden; }
[\n\;]+ {return Other;}
[\t ] {;}
.  {return *yytext; };
%%

int yywrap() {
    return 1;
}
