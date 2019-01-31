%code requires {
}

%code provides {
void yyparse_pbrtv3(const baker::pbrt::SceneCreationCallbacks& i_callbacks);
}

%{
#include <floral.h>
#include <clover.h>

#include "Memory/MemorySystem.h"
#include "PBRTSceneDefs.h"

extern int yylex();

void yyerror(const char* i_errorStr);
void reset_array_stacks();

static baker::pbrt::SceneCreationCallbacks		s_Callbacks;

static floral::inplace_array<baker::F32Array*, 4u>	s_F32ArrayStack;
static floral::inplace_array<baker::S32Array*, 4u>	s_S32ArrayStack;
static floral::inplace_array<baker::StringArray*, 4u>	s_StringArrayStack;
static baker::F32Array*							s_CurrentF32Array;
static baker::S32Array*							s_CurrentS32Array;
static baker::StringArray*						s_CurrentStringArray;
static f32										s_TmpFloat;

%}

%union {
	f32											floatValue;
	s32											intValue;
	cstr										bracketStringValue;
}

%token TK_INTEGRATOR
%token TK_TRANSFORM
%token TK_SAMPLER
%token TK_PIXEL_FILTER
%token TK_FILM
%token TK_CAMERA

%token TK_BEGIN_WORLD
%token TK_END_WORLD

%token TK_MAKE_NAMED_MATERIAL
%token TK_NAMED_MATERIAL
%token TK_SHAPE

%token TK_BEGIN_DATA
%token TK_END_DATA

%token TK_BEGIN_ATTRIB
%token TK_END_ATTRIB

%token TK_AREA_LIGHT_SOURCE

%token <floatValue>								FLOAT_VALUE
%token <intValue>								INT_VALUE
%token <bracketStringValue>						STRING_VALUE

%right "key_float" "key_int" "key_string"

%%
pbrtv3:
	  integrator transform sampler pixel_filter film camera world

/* integrator ---------------------------------*/
integrator:
	integrator_begin int_data_region
	{
		CLOVER_INFO("end integrator");
	}

integrator_begin:
	TK_INTEGRATOR STRING_VALUE STRING_VALUE
	{
		reset_array_stacks();
		CLOVER_INFO("begin integrator - %s - %s", $2, $3);
	}
/*---------------------------------------------*/
/* transform-----------------------------------*/
transform:
	transform_begin float_data_region
	{
		CLOVER_INFO("end transform");
	}

transform_begin:
	TK_TRANSFORM
	{
		reset_array_stacks();
		CLOVER_INFO("begin transform");
	}
/*---------------------------------------------*/
/* sampler-------------------------------------*/
sampler:
	sampler_begin int_data_region
	{
		CLOVER_INFO("end sampler");
	}

sampler_begin:
	TK_SAMPLER STRING_VALUE STRING_VALUE
	{
		reset_array_stacks();
		CLOVER_INFO("begin sampler - %s - %s", $2, $3);
	}
/*---------------------------------------------*/
/* pixel_filter--------------------------------*/
pixel_filter:
	pixel_filter_begin pixel_filter_data
	{
		CLOVER_INFO("end pixel filter");
	}
pixel_filter_begin:
	TK_PIXEL_FILTER STRING_VALUE
	{
		reset_array_stacks();
	}
pixel_filter_data:
	STRING_VALUE float_data_region STRING_VALUE float_data_region
	{

	}
/*---------------------------------------------*/
/* film----------------------------------------*/
film:
	film_begin film_data
	{
		CLOVER_INFO("end film");
	}
film_begin:
	TK_FILM STRING_VALUE
	{
		reset_array_stacks();
	}
film_data:
	STRING_VALUE float_data_region STRING_VALUE float_data_region STRING_VALUE string_data_region
	{
	}
/*---------------------------------------------*/
/* camera--------------------------------------*/
camera:
	camera_begin camera_data
	{
		CLOVER_INFO("end camera");
	}
camera_begin:
	TK_CAMERA
	{
		reset_array_stacks();
	}
camera_data:
	STRING_VALUE STRING_VALUE float_data_region
/*---------------------------------------------*/
/* world---------------------------------------*/
world:
	TK_BEGIN_WORLD world_body TK_END_WORLD
	{
		CLOVER_INFO("end world");
	}

world_body:
	world_body world_body_elem
	| world_body_elem

world_body_elem:
	make_named_material
	| named_material_shape
	| attrib

attrib:
	TK_BEGIN_ATTRIB attrib_body TK_END_ATTRIB
	{
		CLOVER_INFO("end attribute");
	}

attrib_body:
	attrib_body attrib_elem
	| attrib_elem

attrib_elem:
	area_light_source
	| named_material_shape

make_named_material:
	make_named_material_begin key_data_pairs
	{
		CLOVER_INFO("> make named material");
	}

make_named_material_begin:
	TK_MAKE_NAMED_MATERIAL STRING_VALUE
	{
		reset_array_stacks();
	}

named_material_shape:
	TK_NAMED_MATERIAL STRING_VALUE shape_list
	{
		CLOVER_INFO("> named material %s", $2);
	}

shape_list:
	shape_list shape
	| shape

shape:
	shape_inplace
	| shape_ply

shape_inplace:
	shape_begin key_int_data_region_pair key_float_data_region_pair key_float_data_region_pair key_float_data_region_pair
	{
		// shape
		s32 f32ArraysCount = s_F32ArrayStack.get_size();
		s32 s32ArraysCount = s_S32ArrayStack.get_size();
		baker::F32Array* uvs = s_F32ArrayStack[f32ArraysCount - 1];
		baker::F32Array* normals = s_F32ArrayStack[f32ArraysCount - 2];
		baker::F32Array* positions = s_F32ArrayStack[f32ArraysCount - 3];
		baker::S32Array* indices = s_S32ArrayStack[s32ArraysCount - 1];
		baker::Vec3Array* meshPos = baker::g_TemporalArena.allocate<baker::Vec3Array>(positions->get_size() / 3, &baker::g_TemporalArena);
		baker::Vec3Array* meshNormal = baker::g_TemporalArena.allocate<baker::Vec3Array>(normals->get_size() / 3, &baker::g_TemporalArena);
		baker::Vec2Array* meshUV = baker::g_TemporalArena.allocate<baker::Vec2Array>(uvs->get_size() / 2, &baker::g_TemporalArena);
		baker::S32Array* meshIndex = baker::g_TemporalArena.allocate<baker::S32Array>(indices->get_size(), &baker::g_TemporalArena);
		for (u32 i = 0; i < uvs->get_size() / 2; i++)
		{
			meshUV->push_back(floral::vec2f(uvs->at(i * 2), uvs->at(i * 2 + 1)));
		}
		for (u32 i = 0; i < normals->get_size() / 3; i++)
		{
			meshNormal->push_back(floral::vec3f(normals->at(i * 3), normals->at(i * 3 + 1), normals->at(i * 3 + 2)));
		}
		for (u32 i = 0; i < positions->get_size() / 3; i++)
		{
			meshPos->push_back(floral::vec3f(positions->at(i * 3), positions->at(i * 3 + 1), positions->at(i * 3 + 2)));
		}
		for (u32 i = 0; i < indices->get_size(); i++)
		{
			meshIndex->push_back(indices->at(i));
		}
		s_Callbacks.OnNewMesh(*meshIndex, *meshPos, *meshNormal, *meshUV);
	}

shape_ply:
	shape_begin key_string_data_region_pair
	{
		s32 stringArrayCount = s_StringArrayStack.get_size();
		baker::StringArray* strings = s_StringArrayStack[stringArrayCount - 1];

		s_Callbacks.OnNewPlyMesh(strings->at(strings->get_size() - 1));
	}

shape_begin:
	TK_SHAPE STRING_VALUE
	{
		reset_array_stacks();
	}

area_light_source:
	TK_AREA_LIGHT_SOURCE STRING_VALUE key_float_data_region_pair
	{
		CLOVER_INFO("> area light source");
	}

/*---------------------------------------------*/
key_data_pairs:
	key_data_pairs key_float_data_region_pair	%prec "key_float"
	| key_data_pairs key_int_data_region_pair	%prec "key_int"
	| key_data_pairs key_string_data_region_pair	%prec "key_string"

key_float_data_region_pair:
	STRING_VALUE float_data_region

key_int_data_region_pair:
	STRING_VALUE int_data_region

key_string_data_region_pair:
	STRING_VALUE string_data_region

float_data_region:
	TK_BEGIN_DATA float_data_array TK_END_DATA

int_data_region:
	TK_BEGIN_DATA int_data_array TK_END_DATA

string_data_region:
	TK_BEGIN_DATA string_data_array TK_END_DATA

float_data_array:
	float_data_array number_value
	{
		s_CurrentF32Array->push_back(s_TmpFloat);
	}
	| number_value
	{
		s_CurrentF32Array = baker::g_TemporalArena.allocate<baker::F32Array>(256u, &baker::g_TemporalArena);
		s_F32ArrayStack.push_back(s_CurrentF32Array);
		s_CurrentF32Array->push_back(s_TmpFloat);
	}

int_data_array:
	int_data_array INT_VALUE
	{
		s_CurrentS32Array->push_back($2);
	}
	| INT_VALUE
	{
		s_CurrentS32Array = baker::g_TemporalArena.allocate<baker::S32Array>(256u, &baker::g_TemporalArena);
		s_S32ArrayStack.push_back(s_CurrentS32Array);
		s_CurrentS32Array->push_back($1);
	}

string_data_array:
	string_data_array STRING_VALUE
	{
		cstr tmpStr = (cstr)baker::g_TemporalArena.allocate(256);
		strcpy(tmpStr, $2);
		s_CurrentStringArray->push_back(tmpStr);
	}
	| STRING_VALUE
	{
		s_CurrentStringArray = baker::g_TemporalArena.allocate<baker::StringArray>(32u, &baker::g_TemporalArena);
		s_StringArrayStack.push_back(s_CurrentStringArray);
		cstr tmpStr = (cstr)baker::g_TemporalArena.allocate(256);
		strcpy(tmpStr, $1);
		s_CurrentStringArray->push_back(tmpStr);
	}

number_value:
	FLOAT_VALUE
	{
		s_TmpFloat = $1;
	}
	| INT_VALUE
	{
		s_TmpFloat = (f32)$1;
	}

%%

void yyerror(const char* i_errorStr)
{
	CLOVER_ERROR("Bison error: %s", i_errorStr);
}

void reset_array_stacks()
{
	s_F32ArrayStack.empty();
	s_S32ArrayStack.empty();
	baker::g_TemporalArena.free_all();
}

void yyparse_pbrtv3(const baker::pbrt::SceneCreationCallbacks& i_callbacks)
{
	s_Callbacks = i_callbacks;
	yyparse();
}
