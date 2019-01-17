%{
#include <clover.h>
#include "Memory/MemorySystem.h"

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

Integrator										{ return TK_INTEGRATOR; }
Transform										{ return TK_TRANSFORM; }
Sampler											{ return TK_SAMPLER; }
PixelFilter										{ return TK_PIXEL_FILTER; }
Film											{ return TK_FILM; }
Camera											{ return TK_CAMERA; }

WorldBegin										{ CLOVER_INFO("_worldbegin"); }
WorldEnd										{ CLOVER_INFO("_worldend"); }

MakeNamedMaterial								{ CLOVER_INFO("_makenamedmaterial"); }
NamedMaterial									{ CLOVER_INFO("_namedmaterial"); }
Shape											{ CLOVER_INFO("_shape"); }

AttributeBegin									{ CLOVER_INFO("_attributebegin"); }
AttributeEnd									{ CLOVER_INFO("_attributeend"); }

AreaLightSource									{ CLOVER_INFO("_arealightsource"); }

{float_number}									{ yylval.floatValue = atof(yytext); return FLOAT_VALUE; }
{float_number}"e"{int_number}					{ yylval.floatValue = atof(yytext); return FLOAT_VALUE; }
{int_number}									{ yylval.intValue = atoi(yytext); return INT_VALUE; }
{any_bracket_string}							{ yylval.bracketStringValue = strdup(yytext); return STRING_VALUE; }

%%

int yylex_pbrtv3(const char* i_input)
{
	YY_BUFFER_STATE buffer = yy_scan_string(i_input);
	yyparse_pbrtv3();
	yy_delete_buffer(buffer);
	return 0;
}
