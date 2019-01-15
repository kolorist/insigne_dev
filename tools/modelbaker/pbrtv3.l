%{
#include <stdio.h>
%}

%option noyywrap
%option never-interactive
%option nounistd

float_number ([\+|-])?[0-9]+(\.)[0-9]+
int_number ([\+|-])?[0-9]+
any_bracket_string \"([^\"])*\"
data_start \[
data_end \]

%%
[ \t]											;
"\n"											{ BEGIN(INITIAL); }

{data_start}									{ printf("_datastart\n"); }
{data_end}										{ printf("_dataend\n"); }

Integrator										{ printf("_integrator\n"); }
Transform										{ printf("_transform\n"); }
Sampler											{ printf("_sampler\n"); }
PixelFilter										{ printf("_pixelfilter\n"); }
Film											{ printf("_film\n"); }
Camera											{ printf("_camera\n"); }

WorldBegin										{ printf("_worldbegin\n"); }
WorldEnd										{ printf("_worldend\n"); }

MakeNamedMaterial								{ printf("_makenamedmaterial\n"); }
NamedMaterial									{ printf("_namedmaterial\n"); }
Shape											{ printf("_shape\n"); }

AttributeBegin									{ printf("_attributebegin\n"); }
AttributeEnd									{ printf("_attributeend\n"); }

AreaLightSource									{ printf("_arealightsource\n"); }

{float_number}									{ printf("float number: %s\n", yytext); }
{int_number}									{ printf("int number: %s\n", yytext); }
{any_bracket_string}							{ printf("bracket string: %s\n", yytext); }
{float_number}"e"{int_number}					{ printf("exp float number: %s\n", yytext); }

%%

int yylex_pbrtv3(const char* i_input)
{
	yy_scan_string(i_input);
	while(yylex());
	return 0;
}
