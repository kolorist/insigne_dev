%{
#include <floral.h>

extern int yylex();								// from flex

void yyerror(const char* i_errorStr);
%}

%union {
	float										floatValue;
	char*										stringValue;
}

%token CBMAT
%token CBSHDR_PATH

%token MPARAMS
%token P_TEX2D
%token P_TEXCUBE
%token P_MAT4
%token P_VEC3
%token P_FLOAT
%token END_MPARAMS

%token <floatValue>								FLOAT_VALUE
%token <stringValue>							STRING_VALUE

%%

cbmat:
		version cbshdr_path params_header params_body param_footer
		;

version:
		CBMAT FLOAT_VALUE
		;

cbshdr_path:
		CBSHDR_PATH STRING_VALUE				{}
		;

params_header:
		MPARAMS
		;

params_body:
		param_decls
		;

param_decls:
		param_decls param_decl
		| param_decl
		;

param_decl:
		P_TEX2D STRING_VALUE STRING_VALUE		{}
		| P_TEXCUBE STRING_VALUE STRING_VALUE	{}
		| P_FLOAT STRING_VALUE STRING_VALUE		{}
		;

param_footer:
		END_MPARAMS
		;

%%

void yyerror(const char* i_errorStr) {
	printf("bison parse error! message: %s\n", i_errorStr);
}
