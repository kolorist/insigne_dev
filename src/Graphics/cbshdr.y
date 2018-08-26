%code requires {
#include "CBRenderDescs.h"
}

%code provides {
void yyparse_shader(cymbi::ShaderDesc& o_shaderDesc);
}

%{
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
%token P_FLOAT
%token END_SPARAMS

%token <floatValue>								FLOAT_VALUE
%token <stringValue>							STRING_VALUE

%%

cbshdr:
		version shader_paths params_header params_body param_footer
		;

version:
		CBSHDR FLOAT_VALUE
		;

shader_paths:
		VS STRING_VALUE FS STRING_VALUE			{ g_CurrentTarget->vertexShaderPath = floral::path($2); g_CurrentTarget->fragmentShaderPath = floral::path($4); }
		| FS STRING_VALUE VS STRING_VALUE		{ g_CurrentTarget->vertexShaderPath = floral::path($4); g_CurrentTarget->fragmentShaderPath = floral::path($2); }
		;

params_header:
		SPARAMS
		;

params_body:
		param_decls
		;

param_decls:
		param_decls param_decl
		| param_decl
		;

param_decl:
		P_TEX2D STRING_VALUE					{ g_CurrentTarget->shaderParams->push_back(insigne::shader_param_t($2, insigne::param_data_type_e::param_sampler2d)); }
		| P_TEXCUBE STRING_VALUE				{ g_CurrentTarget->shaderParams->push_back(insigne::shader_param_t($2, insigne::param_data_type_e::param_sampler_cube)); }
		| P_MAT4 STRING_VALUE					{ g_CurrentTarget->shaderParams->push_back(insigne::shader_param_t($2, insigne::param_data_type_e::param_mat4)); }
		| P_VEC3 STRING_VALUE					{ g_CurrentTarget->shaderParams->push_back(insigne::shader_param_t($2, insigne::param_data_type_e::param_vec3)); }
		| P_FLOAT STRING_VALUE					{ g_CurrentTarget->shaderParams->push_back(insigne::shader_param_t($2, insigne::param_data_type_e::param_float)); }
		;

param_footer:
		END_SPARAMS
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
