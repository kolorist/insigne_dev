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
static baker::F32Array*							s_F32Array;

%}

%union {
	f32											floatValue;
	cstr										bracketStringValue;
}

%token TK_TRANSFORM

%token TK_CAMERA
%token TK_CAMERA_PERSPECTIVE
%token TK_CAMERA_FOV

%token TK_BEGIN_WORLD
%token TK_END_WORLD

%token TK_BEGIN_TRANSFORM
%token TK_END_TRANSFORM

%token TK_SHAPE
%token TK_PLYMESH

%token TK_BEGIN_DATA
%token TK_END_DATA

%token <floatValue>								FLOAT_VALUE
%token <bracketStringValue>						STRING_VALUE

%%
pbrtv3:
	transform camera world

/* transform-----------------------------------*/
transform:
	transform_begin floats_region
	{
		floral::vec4f col0 = floral::vec4f(s_F32Array->at(0), s_F32Array->at(4), s_F32Array->at(8), s_F32Array->at(12));
		floral::vec4f col1 = floral::vec4f(s_F32Array->at(1), s_F32Array->at(5), s_F32Array->at(9), s_F32Array->at(13));
		floral::vec4f col2 = floral::vec4f(s_F32Array->at(2), s_F32Array->at(6), s_F32Array->at(10), s_F32Array->at(14));
		floral::vec4f col3 = floral::vec4f(s_F32Array->at(3), s_F32Array->at(7), s_F32Array->at(11), s_F32Array->at(15));
		floral::mat4x4f m(col0, col1, col2, col3);
		s_Callbacks.OnPushTransform(m);
	}

transform_begin:
	TK_TRANSFORM
	{
		reset_array_stacks();
	}
/*---------------------------------------------*/

/* camera--------------------------------------*/
camera:
	camera_begin camera_data
	{
		CLOVER_INFO("end camera");
	}
camera_begin:
	TK_CAMERA TK_CAMERA_PERSPECTIVE
	{
		CLOVER_INFO("begin perp camera");
	}
camera_data:
	TK_CAMERA_FOV FLOAT_VALUE
	{
		CLOVER_INFO("fov: %f", $2);
	}
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
	shape
	| transform_region

transform_region:
	transform_reg_begin transform shapes TK_END_TRANSFORM
	{
		CLOVER_INFO("end transform region");
		s_Callbacks.OnPopTransform();
	}

transform_reg_begin:
	TK_BEGIN_TRANSFORM
	{
		CLOVER_INFO("begin transform region");
	}

shapes:
	shapes shape
	| shape

shape:
	shape_ply

shape_ply:
	shape_begin TK_PLYMESH STRING_VALUE
	{
		s_Callbacks.OnNewPlyMesh($3);
	}

shape_begin:
	TK_SHAPE
	{
	}

/*---------------------------------------------*/
floats_region:
	TK_BEGIN_DATA floats TK_END_DATA

floats:
	floats FLOAT_VALUE
	{
		s_F32Array->push_back($2);
	}
	| FLOAT_VALUE
	{
		s_F32Array = baker::g_TemporalArena.allocate<baker::F32Array>(256u, &baker::g_TemporalArena);
		s_F32Array->push_back($1);
	}
%%

void yyerror(const char* i_errorStr)
{
	CLOVER_ERROR("Bison error: %s", i_errorStr);
}

void reset_array_stacks()
{
	baker::g_TemporalArena.free_all();
}

void yyparse_pbrtv3(const baker::pbrt::SceneCreationCallbacks& i_callbacks)
{
	s_Callbacks = i_callbacks;
	yyparse();
}
