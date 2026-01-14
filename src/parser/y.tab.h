
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
     Whole = 263,
     Be = 264,
     If = 265,
     But = 266,
     Else = 267,
     Ok = 268,
     And = 269,
     Or = 270,
     Not = 271,
     Redo = 272,
     Quite = 273,
     As = 274,
     While = 275,
     Return = 276,
     Nil = 277,
     True = 278,
     False = 279,
     SingleComment = 280,
     MultiComment = 281,
     Other = 282
   };
#endif
/* Tokens.  */
#define Assignment 258
#define ArithmeticOperator 259
#define RelationalOperator 260
#define BitwiseOperator 261
#define Iden 262
#define Whole 263
#define Be 264
#define If 265
#define But 266
#define Else 267
#define Ok 268
#define And 269
#define Or 270
#define Not 271
#define Redo 272
#define Quite 273
#define As 274
#define While 275
#define Return 276
#define Nil 277
#define True 278
#define False 279
#define SingleComment 280
#define MultiComment 281
#define Other 282




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


