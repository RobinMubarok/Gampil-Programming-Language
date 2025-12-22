#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>



/* DIDDIBLY BNF

    ! underived
    <arithmeticOperator>	        ::= '+'|'-'|'*'|'/'|'%'|'^'
    <identifierSymbol>	            ::= '_'|'?'|'@'|'`'|'\'
    <combinatorSelector>	        ::= ' '|'>'|'+'|'~'|'.'
    <simpleSelector>	            ::= '#'|'.'
    <binaryDigit>		            ::= '0'|'1'
    <octalSubSet>		            ::= %x32-37
    <decimalExtension>	            ::= '8'|'9'
    <hexSubSetUpper>	            ::= %x41-46
    <hexSubSetLower>	            ::= %x61-66
    <sp>			                ::= ' '|'\n' [<sp>]

    ! number system
    <naturalDigit>		            ::= '1'|<octalSubset>|<decimalExtension>
    <decimalDigit>		            ::= '0'|<naturalDigit>
    <octalDigit>		            ::= <binaryDigit>|<octalSubSet>
    <hexDigit>		                ::= <decimalDigit>|<hexSubSetUpper>|<hexSubSetLower>

    ! advanced operator
    <relationalOperator>	        ::= '>' ['=']|'<' ['=']|'!' ['=']|'='
    <bitwiseOperator>	            ::= '&'|'|' ['|']|'>>'|'<<'|'~'
    <symbolOperator>	            ::= <arithmeticOperator>|<relationalOperator>|<bitwiseOperator>

    ! integer representation
    <binaryNumber>		            ::= '0b' <binaryDigit>+
    <octalNumber>		            ::= '0o' <octalDigit>+
    <hexNumber>		                ::= '0x' <hexDigit>+
    <decimalNumber>		            ::= <decimalDigit>+
    <integerNumber>		            ::= <decimalNumber>|(['-' [<sp>]] <naturalDigit>+)
    <integers>		                ::= <binaryNumber>|<octalNumber>|<hexNumber>|<decimalNumber>|<integerNumber>

    ! complex number representation
    <fraction>		                ::= '.' <decimalNumber>
    <exponent>		                ::= ('E'|'e') ['+'|'-'] <decimalNumber>
    <floatNumber>		            ::= <integerNumber>|(['-']|<integerNumber>) <fraction> <exponent>
    <imaginaryNumber>	            ::= <floatNumber> ('j'|'J')
    <complexes>		                ::= <floatNumber>|<imaginaryNumber>

    ! keyword
    <and>			                ::= 'a' 'n' 'd'
    <as>			                ::= 'a' 's'
    <be>			                ::= 'b' 'e'
    <const>			                ::= 'c' 'o' 'n' 's' 't'
    <def>			                ::= 'd' 'e' 'f'
    <each>			                ::= 'e' 'a' 'c' 'h'
    <else>			                ::= 'e' 'l' 's' 'e'
    <false>			                ::= 'f' 'a' 'l' 's' 'e'
    <for>			                ::= 'f' 'o' 'r'
    <from>			                ::= 'f' 'r' 'o' 'm'
    <if>			                ::= 'i' 'f'
    <me>			                ::= 'm' 'e'
    <nil>			                ::= 'n' 'i' 'l'
    <not>			                ::= 'n' 'o' 't'
    <ok>			                ::= 'o' 'k'
    <or>			                ::= 'o' 'r'
    <private>		                ::= 'p' 'r' 'i' 'v' 'a' 't' 'e'
    <protected>		                ::= 'p' 'r' 'o' 't' 'e' 'c' 't' 'e' 'd'
    <public>		                ::= 'p' 'u' 'b' 'l' 'i' 'c'
    <range>			                ::= 'r' 'a' 'n' 'g' 'e'
    <return>		                ::= 'r' 'e' 't' 'u' 'r' 'n'
    <static>		                ::= 's' 't' 'a' 't' 'i' 'c'
    <true>			                ::= 't' 'r' 'u' 'e'
    <use>			                ::= 'u' 's' 'e'
    <while>			                ::= 'w' 'h' 'i' 'l' 'e'

    ! continuation keyword
    <elseif>		                ::= <else> <if>

    ! keyword stuff
    <assignmentOperator>	        ::= [<symbolOperator>] <be>
    <booleanOperator>	            ::= <and>|<or>|<not>
    <boolean>		                ::= <true>|<false>

    ! valid identifier
    <upperChar>		                ::= <hexSubSetUpper>|%x47–5A
    <lowerChar>		                ::= <hexSubSetLower>|%x67–7A
    <letter>		                ::= <upperChar>|<lowerChar>
    <generalIdentifier>	            ::= (<letter>+|<IdentifierSymbol>+) [<letter>+|<decimalNumbers>|<IdentifierSymbol>+]
    <labelIdentifier>	            ::= '$' <generalIdentifier>

    ! string and char
    <nonPrintable>		            ::= %x00-%x1F|%x7F
    <quineChar>		                ::= ^(<nonPrintable>|<quineMark>)
    <commentChar>		            ::= ^(<nonPrintable>|'--')
    <pureChar>		                ::= ^(<nonPrintable>|'\'|'\''|'\"')
    <escapeChar>		            ::= ^(<nonPrintable>|' ')
    <escapeSequence>	            ::= '\' <escapeChar>+
    <validChar>		                ::= <pureChar>|<escapeSequence>
    <stringPrefix>		            ::= 'f'|'F'|'b'|'B'|'r'|'R'|'u'|'U'|'t'|'T'
    <quineMark>		                ::= '\'' '\'' '\''|'\"' '\''|'\'' '\"'|'\"' '\"' '\"'
    <generalString>		            ::= '\'' {<validChar>} '\''
    <fixedString>		            ::= '\"' {<validChar>} '\"'
    <quineString>		            ::= <quineMark> {<quineChar>} <quineMark>
    <prefixedString>	            ::= <stringPrefix> '\'' {<validChar>|<set>} '\''
    <strings>		                ::= <generalString>|<fixedString>|<quineString>|<prefixedString>

    ! values
    <singleValue>		            ::= <integers>|<strings>|<complexes>|<boolean>
    <unaryOperation>	            ::= (('~'|'!' [<sp>])|(<not> <sp>)) <singleValue>
    <operators>		                ::= (<symbolOperator>|<sp> <booleanOperator>) <sp>
    <resultValue>		            ::= (<unaryOperation>|<value>) [[<sp>] <operators> <resultValue>]
    <arrCall>		                ::= <generalIdentifier> [<sp>] '[' [<sp>] <value> [<sp>] ']'
    <validValue>		            ::= <singleValue>|<resultValue>|<labelIdentifier>
    <validNaming>		            ::= <generalIdentifier>|<arrCall>|<attributes>|<procCall>
    <argumentsOf>		            ::= (<value>|<fullAssignment>) [<sp>] [',' [<sp>] <argumentsOf>]
    <procCall>		                ::= <validNaming> [<sp>] '[' [<sp>] [<argumentsOf>] [<sp>] ']' ['.' <procCall>]

    ! parentheses
    <elementOf>		                ::= (<value>|<fullDeclaration>) [<sp>] [',' [<sp>] <elementOf>]
    <list>			                ::= '[' [<sp>] {<elementOf>} [<sp>] ']'
    <tuple>			                ::= '(' [<sp>] {<elementOf>} [<sp>] ')'
    <set>			                ::= '{' [<sp>] {<elementOf>} [<sp>] '}'
    <dataSequence>		            ::= <set>|<tuple>|<list>
    <attributes>		            ::= <validNaming> ['.' <attributes>]
    <value>			                ::= <validValue>|<validNaming>|<procCall>|<dataSequence>|<nil>

    ! declaration
    <dataTypes>		                ::= <generalIdentifier> [<sp>] [<tuple>] [<sp>] ['|' [<sp>] <dataTypes>]
    <dataId>		                ::= '#' <procCall>
    <accessModifier>	            ::= <private>|<protected>|<public> [<static>]
    <typeDeclaration>	            ::= [<const>|<accessModifier>] <sp> [<dataId>] <sp> <dataTypes> <sp> <generalIdentifier>
    <halfDeclaration>	            ::= <typeDeclaration> [<sp>] [',' [<sp>] <halfDeclaration>]
    <halfAssignment>	            ::= (<generalIdentifier>|<attributes>) [<sp>] [',' [<sp>] <halfAssignment>]
    <halfValueAssign>	            ::= <value> [<sp>] [',' [<sp>] <halfValueAssign>]
    <fullAssignment>	            ::= <halfAssignment> <sp> <assignmentOperator> <sp> [<as>] <sp> <halfValueAssign>
    <fullDeclaration>	            ::= <halfDeclaration> <sp> [<assignmentOperator> <sp> [<as>] <sp> <halfValueAssign>]
    <declaration>		            ::= <fullAssignment>|<fullDeclaration>

    ! block statement
    <elseIfStatement>	            ::= <elseif> <sp> <value> [<sp>] ':' [<sp>] <block>
    <ifStatement>		            ::= <if> <sp> <value> [<sp>] ':' [<sp>] <block> [<sp>] {<elseIfStatement>} [<sp>] [<else> [<sp>] ':' [<sp>] <block>] <sp> <ok>
    <forLine>		                ::= <statements> [<sp>] [',' [<sp>] <forLine>]
    <forEach>		                ::= (<declaration>|<each>) [<sp>] ',' [<sp>] <dataTypes> <sp> <generalIdentifier> <sp> <range> <sp> <generalIdentifier>
    <forClassic>		            ::= {<forLine>} [<sp>] ';' [<sp>] {<value>} [<sp>] ';' [<sp>] {<forLine>}
    <forStatement>		            ::= <for> <sp> (<forEach>|<forClassic>|<value>) [<sp>] ':' [<sp>] <block> <sp> <ok>
    <whileStatement>	            ::= <while> <sp> [<arrCall> [<sp>] ';'] [<sp>] <value> [<sp>] ':' [<sp>] <block> <sp> <ok>
    <argumentDeclared>	            ::= <fullDeclaration> [<sp>] [',' [<sp>] <argumentDeclared>]
    <halfprocStatement>	            ::= <generalIndentifier> [<sp>] '[' [<sp>] <argumentDeclared> [<sp>] ']'
    <procStatement>		            ::= <typeDeclaration> <sp> <halfprocStatement> [<sp>] ':' [<sp>] <block> <sp> <ok>
    <classSelectors>	            ::= [<simpleSelector>] <generalIdentifier>  [[<sp>] <combinatorSelector> [<sp>] <classSelectors>]
    <classStatement>	            ::= <classSelectors> [<sp>] ':' [<sp>] {<classLine>} [<sp>] <ok>
    <constructStatement>	        ::= <def> <sp> <halfprocStatement> [<sp>] ':' [<sp>] <block> <sp> <ok>
    <statements>		            ::= <declaration>|<procCall>
    <blockStatements>	            ::= <ifStatement>|<forStatement>|<whileStatement>|<classStatement>|<procStatement>|<library>|<label>
    <line>			                ::= (<statements> [<sp>] (';'|'\n') [<sp>] [<line>])|(<blockStatements> [<sp> [<line>]])
    <classLine>		                ::= <line>|<constructStatement> [<sp> [<classLine>]]
    <block>			                ::= {<line>}
    <label>			                ::= <labelIdentifier> [<sp>] ':' [<sp>] [<block>]

    ! comments
    <comment>		                ::= '--' {<commentChar>} ('--'|'\n')|<quineString>

    ! library call
    <library>		                ::= [<from> <sp> <fixedString>] <sp> <use> <sp> <fixedString> <sp> <as> <sp> <fixedString>

*/



// jenis token
typedef enum type {

    nonToken = '\0',
    arithmeticOperator,
    identifierSymbol, combinatorSelector,
    simpleSelector, binaryDigit,
    octalSubSet, decimalExtension,
    hexSubSetUpper, hexSubSetLower,
    sp, naturalDigit,
    decimalDigit, octalDigit,
    hexDigit, relationalOperator,
    bitwiseOperator,
    symbolOperator,
    binaryNumber,
    octalNumber,
    hexNumber, decimalNumber,
    integerNumber, integers,
    fraction, exponent,
    floatNumber,
    imaginaryNumber,
    complexes, and, as, be,
    consts, def, each,
    elses, falses,
    fors, from, ifs,
    me, nil, not, ok, or,
    privates, protecteds, publics,
    range, returns,
    statics, trues, use, whiles,
    elseif, assignmentOperator,
    booleanOperator, booleans,
    upperChar, lowerChar,
    letter, generalIdentifier,
    labelIdentifier, nonPrintable,
    quineChar, commentChar, pureChar,
    escapeChar, escapeSequence,
    validChar, stringPrefix, quineMark,
    generalString, fixedString,
    quineString, prefixedString,
    strings, singleValue,
    unaryOperation, operators,
    resultValue, arrCall,
    validValue, validNaming,
    argumentsOf, procCall, elementOf,
    lists, tuples, sets,
    dataSequence, attributes, value,
    dataTypes, dataId,
    accessModifier, typeDeclaration,
    halfDeclaration, halfAssignment,
    halfValueAssign, fullAssignment,
    fullDeclaration, declaration,
    elseIfStatement, ifStatement,
    forLine, forEach, forClassic,
    forStatement, whileStatement,
    argumentDeclared, halfprocStatement,
    procStatement, classSelectors,
    classStatement, constructStatement,
    statements, blockStatements,
    line, classLine, block, label,
    comment, library

} typeNames;

// pemeriksa token
bool checkToken[124];

// tipe penyimpan token
typedef struct Token {

    typeNames tokenType;
    int tokenRecurred;
    char* tokenVal;
    struct Token* recursions;

} tokenCatcher;




