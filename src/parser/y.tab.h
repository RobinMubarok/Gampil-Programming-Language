
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     Assignment = 258,
     ArithmeticOperator = 259,
     RelationalOperator = 260,
     BitwiseOperator = 261,
     Iden = 262,
     Be = 263,
     If = 264,
     But = 265,
     Else = 266,
     Ok = 267,
     And = 268,
     Or = 269,
     Not = 270,
     Redo = 271,
     Quite = 272,
     As = 273,
     While = 274,
     Return = 275,
     Nil = 276,
     True = 277,
     False = 278,
     SingleComment = 279,
     MultiComment = 280,
     StringItem = 281,
     IntegerItem = 282,
     FloatItem = 283,
     Other = 284
   };
#endif
/* Tokens.  */
#define Assignment 258
#define ArithmeticOperator 259
#define RelationalOperator 260
#define BitwiseOperator 261
#define Iden 262
#define Be 263
#define If 264
#define But 265
#define Else 266
#define Ok 267
#define And 268
#define Or 269
#define Not 270
#define Redo 271
#define Quite 272
#define As 273
#define While 274
#define Return 275
#define Nil 276
#define True 277
#define False 278
#define SingleComment 279
#define MultiComment 280
#define StringItem 281
#define IntegerItem 282
#define FloatItem 283
#define Other 284




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


