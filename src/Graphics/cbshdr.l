%{
#include <floral.h>

#include "cbshdr.tab.h"

#include "CBRenderDescs.h"
%}

%option noyywrap
%option never-interactive
%option nounistd

%%
[ \t\n]											;

_version										{ return VERSION; }
_cbshdr											{ return CBSHDR_PATH; }
_vs												{ return VS; }
_fs												{ return FS; }

_sparams										{ return SPARAMS; }
_p_tex2d										{ return P_TEX2D; }
_p_texcube										{ return P_TEXCUBE; }
_p_mat4											{ return P_MAT4; }
_p_vec3											{ return P_VEC3; }
_p_float										{ return P_FLOAT; }
_end_sparams									{ return END_SPARAMS; }

[0-9]+\.[0-9]+									{ yylval.floatValue = atof(yytext); return FLOAT_VALUE; }
[a-zA-Z0-9\/\.\_]+								{ yylval.stringValue = strdup(yytext); return STRING_VALUE; }
%%

int yylex_cbshdr(const char* i_input, cymbi::ShaderDesc& o_shaderDesc) {
	YY_BUFFER_STATE buffer = yy_scan_string(i_input);
	yyparse_shader(o_shaderDesc);
	yy_delete_buffer(buffer);
	return 0;
}

int yylex_cbmat(const char* i_input, cymbi::MaterialDesc& o_matDesc) {
	YY_BUFFER_STATE buffer = yy_scan_string(i_input);
	yyparse_material(o_matDesc);
	yy_delete_buffer(buffer);
	return 0;
}
