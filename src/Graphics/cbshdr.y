%{
#include <stdio.h>

extern int yylex();
extern int yyparse();

void yyerror(const char* i_errorStr);
%}

%union {
	float										floatValue;
	char*										stringValue;
}

%token CBSHDR
%token VS
%token FS

%token SPARAMS
%token P_TEX2D
%token P_TEXCUBE
%token P_MAT4
%token P_VEC3
%token END_SPARAMS

%token <floatValue>								FLOAT
%token <stringValue>							STRING

%%

cbshdr:
		version shader_paths params_header params_body param_footer
		;

version:
		CBSHDR FLOAT							{ printf("cymbi shader version %1.1f\n", $2); }
		;

shader_paths:
		VS STRING FS STRING						{ printf("vs: '%s' | fs: '%s'\n", $2, $4); }
		| FS STRING VS STRING					{ printf("fs: '%s' | vs: '%s'\n", $2, $4); }
		;

params_header:
		SPARAMS									{ printf("begin params list:\n"); }
		;

params_body:
		param_decls
		;

param_decls:
		param_decls param_decl
		| param_decl
		;

param_decl:
		P_TEX2D STRING							{ printf("texture 2d: %s\n", $2); }
		| P_TEXCUBE STRING						{ printf("texture cube: %s\n", $2); }
		| P_MAT4 STRING							{ printf("mat4: %s\n", $2); }
		| P_VEC3 STRING							{ printf("vec3: %s\n", $2); }
		;

param_footer:
		END_SPARAMS								{ printf("end params list"); }
		;

%%

void yyerror(const char* i_errorStr) {
	printf("bison parse error! message: %s\n", i_errorStr);
}
