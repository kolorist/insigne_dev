%{
#include <stdio.h>

#include "cbshdr.tab.h"
%}

%option noyywrap
%option never-interactive
%option nounistd

%%
[ \t\n]											;

_cbshdr											{ return CBSHDR; }
_vs												{ return VS; }
_fs												{ return FS; }

_sparams										{ return SPARAMS; }
_p_tex2d										{ return P_TEX2D; }
_p_texcube										{ return P_TEXCUBE; }
_p_mat4											{ return P_MAT4; }
_p_vec3											{ return P_VEC3; }
_end_sparams									{ return END_SPARAMS; }

[0-9]+\.[0-9]+									{ yylval.floatValue = atof(yytext); return FLOAT; }
[a-zA-Z0-9\/\.\_]+								{ yylval.stringValue = strdup(yytext); return STRING; }
%%

int yylex_cbshdr(const char* i_input) {
	YY_BUFFER_STATE buffer = yy_scan_string(i_input);
	yyparse();
	yy_delete_buffer(buffer);
	return 0;
}

