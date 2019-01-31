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
    TK_INTEGRATOR = 258,
    TK_TRANSFORM = 259,
    TK_SAMPLER = 260,
    TK_PIXEL_FILTER = 261,
    TK_FILM = 262,
    TK_CAMERA = 263,
    TK_BEGIN_WORLD = 264,
    TK_END_WORLD = 265,
    TK_MAKE_NAMED_MATERIAL = 266,
    TK_NAMED_MATERIAL = 267,
    TK_SHAPE = 268,
    TK_BEGIN_DATA = 269,
    TK_END_DATA = 270,
    TK_BEGIN_ATTRIB = 271,
    TK_END_ATTRIB = 272,
    TK_AREA_LIGHT_SOURCE = 273,
    FLOAT_VALUE = 274,
    INT_VALUE = 275,
    STRING_VALUE = 276
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 32 "pbrtv3.y" /* yacc.c:1910  */

	f32											floatValue;
	s32											intValue;
	cstr										bracketStringValue;

#line 87 "pbrtv3.tab.h" /* yacc.c:1910  */
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

#line 104 "pbrtv3.tab.h" /* yacc.c:1910  */

#endif /* !YY_YY_PBRTV3_TAB_H_INCLUDED  */
