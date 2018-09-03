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

%%
[ \t\n]											;

"#".*											{ printf("comment: %s\n", yytext); }

"mtllib"[ \t]+									{ BEGIN(mtllib); printf("mtllib\n"); }
<mtllib>.*										{ printf("%s\n", yytext); }
<mtllib>\n										{ BEGIN(INITIAL); }

[go][ \t]+										{ BEGIN(group); printf("group\n"); }
<group>.*										{ printf("%s\n", yytext); }
<group>\n										{ BEGIN(INITIAL); }

"usemtl"[ \t]+									{ BEGIN(usemtl); printf("usemtl\n"); }
<usemtl>.*										{ printf("%s\n", yytext); }
<usemtl>\n										{ BEGIN(INITIAL); }

"/"												{ printf("separator\n"); }
"s"												{ printf("smooth surface\n"); }

"off"											{ printf("off\n"); }

([\+|-])?[0-9]+.[0-9]+							{ printf("number: %s\n", yytext); }

%%

int yylex_cbobj(const char* i_input)
{
	yy_scan_string(i_input);
	while(yylex());
	return 0;
}
