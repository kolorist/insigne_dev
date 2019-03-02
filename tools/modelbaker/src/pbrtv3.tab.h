/* A Bison parser, made by GNU Bison 3.0.5.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018 Free Software Foundation, Inc.

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

#ifndef YY_YY_PBRTV3_TAB_H_INCLUDED
# define YY_YY_PBRTV3_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif
/* "%code requires" blocks.  */
#line 1 "pbrtv3.y" /* yacc.c:1910  */


#line 47 "pbrtv3.tab.h" /* yacc.c:1910  */

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    TK_TRANSFORM = 258,
    TK_CAMERA = 259,
    TK_CAMERA_PERSPECTIVE = 260,
    TK_CAMERA_FOV = 261,
    TK_BEGIN_WORLD = 262,
    TK_END_WORLD = 263,
    TK_BEGIN_TRANSFORM = 264,
    TK_END_TRANSFORM = 265,
    TK_SHAPE = 266,
    TK_PLYMESH = 267,
    TK_BEGIN_DATA = 268,
    TK_END_DATA = 269,
    FLOAT_VALUE = 270,
    STRING_VALUE = 271
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 25 "pbrtv3.y" /* yacc.c:1910  */

	f32											floatValue;
	cstr										bracketStringValue;

#line 81 "pbrtv3.tab.h" /* yacc.c:1910  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);
/* "%code provides" blocks.  */
#line 4 "pbrtv3.y" /* yacc.c:1910  */

void yyparse_pbrtv3(const baker::pbrt::SceneCreationCallbacks& i_callbacks);

#line 98 "pbrtv3.tab.h" /* yacc.c:1910  */

#endif /* !YY_YY_PBRTV3_TAB_H_INCLUDED  */