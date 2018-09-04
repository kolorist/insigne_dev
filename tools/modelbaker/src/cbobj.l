%{
#include <floral.h>
#include <stdio.h>

//#include "cbobj.tab.h"
%}

%option noyywrap
%option never-interactive
%option nounistd

%x mtllib
%x group
%x usemtl
%x smooth_surface
%s face
%s vertex_position
%s vertex_normal
%s vertex_texcoord

junk_characters ([ \t]+)?
float_number ([\+|-])?[0-9]+(\.)[0-9]+
int_number ([\+|-])?[0-9]+
any_string .*

%%
[ \t]											;
"\n"											{ BEGIN(INITIAL); }

"#"{any_string}									{ printf("comment: %s\n", yytext); }

"mtllib"{junk_characters}						{ BEGIN(mtllib); printf("mtllib\n"); }
<mtllib>{any_string}							{ printf("%s\n", yytext); }
<mtllib>\n										{ BEGIN(INITIAL); }

[go]{junk_characters}							{ BEGIN(group); printf("group\n"); }
<group>{any_string}								{ printf("%s\n", yytext); }
<group>\n										{ BEGIN(INITIAL); }

"usemtl"{junk_characters}						{ BEGIN(usemtl); printf("usemtl\n"); }
<usemtl>{any_string}							{ printf("%s\n", yytext); }
<usemtl>\n										{ BEGIN(INITIAL); }

"/"												{ printf("separator\n"); }

"s"{junk_characters}							{ BEGIN(smooth_surface); }
<smooth_surface>"off"							{ printf("off\n"); }
<smooth_surface>[:digit:]						{ printf("ss = %s\n", yytext); }
<smooth_surface>\n								{ BEGIN(INITIAL); }

"off"											{ printf("off\n"); }

"f"{junk_characters}							{ BEGIN(face); printf("face\n"); }

{float_number}									{ printf("float number: %s\n", yytext); }
{int_number}									{ printf("int number: %s\n", yytext); }

%%

int yylex_cbobj(const char* i_input)
{
	yy_scan_string(i_input);
	while(yylex());
	return 0;
}
