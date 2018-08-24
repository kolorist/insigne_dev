%code requires {
#include "CBRenderDescs.h"
}

%code provides {
void yyparse_shader(cymbi::ShaderDesc& o_shaderDesc);
}

%{
#include <stdio.h>
#include <floral.h>

#include <insigne/commons.h>

extern int yylex();								// from flex

void yyerror(const char* i_errorStr);
%}

%union {
	float										floatValue;
	char*										stringValue;
}
%{
cymbi::ShaderDesc*								g_CurrentTarget = nullptr;
%}

%token CBSHDR
%token VS
%token FS

%token SPARAMS
%token P_TEX2D
%token P_TEXCUBE
%token P_MAT4
%token P_VEC3
%token END_SPARAMS

%token <floatValue>								FLOAT_VALUE
%token <stringValue>							STRING_VALUE

%%

cbshdr:
		version shader_paths params_header params_body param_footer
		;

version:
		CBSHDR FLOAT_VALUE						{ printf("cymbi shader version %1.1f\n", $2); }
		;

shader_paths:
		VS STRING_VALUE FS STRING_VALUE						{ g_CurrentTarget->vertexShaderPath = floral::path($2); g_CurrentTarget->fragmentShaderPath = floral::path($4); }
		| FS STRING_VALUE VS STRING_VALUE					{ g_CurrentTarget->vertexShaderPath = floral::path($4); g_CurrentTarget->fragmentShaderPath = floral::path($2); }
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
		P_TEX2D STRING_VALUE							{ g_CurrentTarget->shaderParams.push_back(insigne::shader_param_t($2, insigne::param_data_type_e::param_sampler2d)); }
		| P_TEXCUBE STRING_VALUE						{ printf("texture cube: %s\n", $2); }
		| P_MAT4 STRING_VALUE							{ printf("mat4: %s\n", $2); }
		| P_VEC3 STRING_VALUE							{ printf("vec3: %s\n", $2); }
		;

param_footer:
		END_SPARAMS								{ printf("end params list"); }
		;

%%

void yyerror(const char* i_errorStr) {
	printf("bison parse error! message: %s\n", i_errorStr);
}

void yyparse_shader(cymbi::ShaderDesc& o_shaderDesc) {
	g_CurrentTarget = &o_shaderDesc;
	yyparse();
	g_CurrentTarget = nullptr;
}
