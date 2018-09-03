%code requires {
// #include "CBRenderDescs.h"
}

%code provides {
//void yyparse_shader(cymbi::ShaderDesc& o_shaderDesc);
//void yyparse_material(cymbi::MaterialDesc& o_matDesc);
}

%{
#include <floral.h>

extern int yylex();								// from flex

void yyerror(const char* i_errorStr);
%}

%union {
	float										floatValue;
	char*										stringValue;
}
%{
%}

%token TK_COMMENT

%token TK_MATERIAL_LIB;
%token TK_MATERIAL_NAME;
%token TK_SURFACES_GROUP;
%token TK_SMOOTH_SHADING;

%token TK_VERTEX_POSITION;
%token TK_VERTEX_NORMAL;
%token TK_VERTEX_TEXCOORD;
%token TK_FACE;

%token TK_SEPARATOR;

%token <floatValue>								V_FLOAT;
%token <stringValue>							V_STRING;

%%

cbobj:
		version shader_paths params_header params_body param_footer
		| version cbshdr_path params_header param_defs param_footer
		;

comment:
		TK_COMMENT V_STRING
		;

shader_paths:
		VS STRING_VALUE FS STRING_VALUE			{ g_CurrentTargetShader->vertexShaderPath = floral::path($2); g_CurrentTargetShader->fragmentShaderPath = floral::path($4); }
		| FS STRING_VALUE VS STRING_VALUE		{ g_CurrentTargetShader->vertexShaderPath = floral::path($4); g_CurrentTargetShader->fragmentShaderPath = floral::path($2); }
		;

cbshdr_path:
		CBSHDR_PATH STRING_VALUE				{ g_CurrentTargetMaterial->cbShaderPath = floral::path($2); }

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
		P_TEX2D STRING_VALUE					{ g_CurrentTargetShader->shaderParams->push_back(insigne::shader_param_t($2, insigne::param_data_type_e::param_sampler2d)); }
		| P_TEXCUBE STRING_VALUE				{ g_CurrentTargetShader->shaderParams->push_back(insigne::shader_param_t($2, insigne::param_data_type_e::param_sampler_cube)); }
		| P_MAT4 STRING_VALUE					{ g_CurrentTargetShader->shaderParams->push_back(insigne::shader_param_t($2, insigne::param_data_type_e::param_mat4)); }
		| P_VEC3 STRING_VALUE					{ g_CurrentTargetShader->shaderParams->push_back(insigne::shader_param_t($2, insigne::param_data_type_e::param_vec3)); }
		| P_FLOAT STRING_VALUE					{ g_CurrentTargetShader->shaderParams->push_back(insigne::shader_param_t($2, insigne::param_data_type_e::param_float)); }
		;

param_defs:
		param_defs param_def
		| param_def
		;

param_def:
		 P_TEX2D STRING_VALUE STRING_VALUE		{ add_nvp($2, floral::path($3), g_CurrentTargetMaterial->tex2DParams); }
		 | P_TEXCUBE STRING_VALUE STRING_VALUE	{ add_nvp($2, floral::path($3), g_CurrentTargetMaterial->texCubeParams); }

param_footer:
		END_SPARAMS
		;

%%

void yyerror(const char* i_errorStr) {
	CLOVER_ERROR("bison parse error! message: %s", i_errorStr);
}

void yyparse_shader(cymbi::ShaderDesc& o_shaderDesc) {
	g_CurrentTargetShader = &o_shaderDesc;
	yyparse();
	g_CurrentTargetShader = nullptr;
}

void yyparse_material(cymbi::MaterialDesc& o_matDesc) {
	g_CurrentTargetMaterial = &o_matDesc;
	yyparse();
	g_CurrentTargetMaterial = nullptr;
}

template <typename T, typename TContainer>
void add_nvp(const_cstr i_name, const T& i_value, TContainer& o_container)
{
	cymbi::ParamNVP<T> nvp;
	strcpy(nvp.name, i_name);
	nvp.value = i_value;
	o_container.push_back(nvp);
}
