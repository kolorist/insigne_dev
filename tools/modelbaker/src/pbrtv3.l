%{
#include <clover.h>
#include "Memory/MemorySystem.h"
#include "PBRTSceneDefs.h"

#include "pbrtv3.tab.h"

// custom allocator
void* yyalloc(size_t i_bytes)
{
	return baker::g_ParserAllocator.allocate(i_bytes);
}

void* yyrealloc(void* i_ptr, size_t i_bytes)
{
	return baker::g_ParserAllocator.reallocate(i_ptr, i_bytes);
}

void yyfree(void* i_ptr)
{
	return baker::g_ParserAllocator.free(i_ptr);
}
%}

%option noyywrap
%option never-interactive
%option nounistd

%option noyyalloc
%option noyyrealloc
%option noyyfree

float_number ([\+|-])?[0-9]+(\.)[0-9]+
int_number ([\+|-])?[0-9]+
any_bracket_string \"([^\"])*\"
data_start \[
data_end \]

%%
[ \t]											;
"\n"											{ BEGIN(INITIAL); }

{data_start}									{ return TK_BEGIN_DATA; }
{data_end}										{ return TK_END_DATA; }

Transform										{ return TK_TRANSFORM; }

Camera											{ return TK_CAMERA; }
perspective										{ return TK_CAMERA_PERSPECTIVE; }
fov												{ return TK_CAMERA_FOV; }

WorldBegin										{ return TK_BEGIN_WORLD; }
WorldEnd										{ return TK_END_WORLD; }

TransformBegin									{ return TK_BEGIN_TRANSFORM; }
TransformEnd									{ return TK_END_TRANSFORM; }

Shape											{ return TK_SHAPE; }
plymesh											{ return TK_PLYMESH; }

{float_number}									{ yylval.floatValue = (f32)atof(yytext); return FLOAT_VALUE; }
{float_number}"e"{int_number}					{ yylval.floatValue = (f32)atof(yytext); return FLOAT_VALUE; }
{int_number}									{ yylval.floatValue = (f32)atof(yytext); return FLOAT_VALUE; }
{any_bracket_string}							{ yylval.bracketStringValue = strdup(yytext); return STRING_VALUE; }

%%

int yylex_pbrtv3(const char* i_input, const baker::pbrt::SceneCreationCallbacks& i_callbacks)
{
	YY_BUFFER_STATE buffer = yy_scan_string(i_input);
	yyparse_pbrtv3(i_callbacks);
	yy_delete_buffer(buffer);
	return 0;
}
