%code requires {
}

%code provides {
void yyparse_pbrtv3();
}

%{
#include <floral.h>
#include <clover.h>

extern int yylex();

void yyerror(const char* i_errorStr);
%}

%union {
	float										floatValue;
	int											intValue;
	char*										bracketStringValue;
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
	TK_MAKE_NAMED_MATERIAL STRING_VALUE key_string_data_region_pair key_float_data_region_pair
	{
		CLOVER_INFO("> make named material");
	}

named_material_shape:
	TK_NAMED_MATERIAL STRING_VALUE shape
	{
		CLOVER_INFO("> named material %s", $2);
	}

shape:
	TK_SHAPE STRING_VALUE key_int_data_region_pair key_float_data_region_pair key_float_data_region_pair key_float_data_region_pair
	{
		CLOVER_INFO("> shape %s", $2);
	}

area_light_source:
	TK_AREA_LIGHT_SOURCE STRING_VALUE key_float_data_region_pair
	{
		CLOVER_INFO("> area light source");
	}

/*---------------------------------------------*/
key_float_data_region_pair:
	STRING_VALUE float_data_region
	{
		CLOVER_INFO("key float data pair: %s", $1);
	}

key_int_data_region_pair:
	STRING_VALUE int_data_region

key_string_data_region_pair:
	STRING_VALUE string_data_region

float_data_region:
	TK_BEGIN_DATA float_data_array TK_END_DATA
	{
		CLOVER_INFO("> float data region");
	}

int_data_region:
	TK_BEGIN_DATA int_data_array TK_END_DATA
	{
		CLOVER_INFO("> int data region");
	}

string_data_region:
	TK_BEGIN_DATA string_data_array TK_END_DATA
	{
		CLOVER_INFO("> string data region");
	}

float_data_array:
	float_data_array number_value
	| number_value

int_data_array:
	int_data_array INT_VALUE
	| INT_VALUE

string_data_array:
	string_data_array STRING_VALUE
	| STRING_VALUE

number_value:
	FLOAT_VALUE
	| INT_VALUE

%%

void yyerror(const char* i_errorStr)
{
	CLOVER_ERROR("Bison error: %s", i_errorStr);
}

void yyparse_pbrtv3()
{
	yyparse();
}
