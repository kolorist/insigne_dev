%{
#include <floral.h>
%}

%option noyywrap
%option never-interactive
%option nounistd

%%
[ \t\n]											;

_version										{ return CBMAT; }
_cbshdr											{ return CBSHDR_PATH; }

_mparams										{ return MPARAMS; }
_p_tex2d										{ return P_TEX2D; }
_p_texcube										{ return P_TEXCUBE; }
_p_mat4											{ return P_MAT4; }
_p_vec3											{ return P_VEC3; }
_p_float										{ return P_FLOAT; }
_end_mparams									{ return END_MPARAMS; }

[0-9]+\.[0-9]+									{ yylval.floatValue = atof(yytext); return FLOAT_VALUE; }
[a-zA-Z0-9\/\.\_]+								{ yylval.stringValue = strdup(yytext); return STRING_VALUE; }
%%
