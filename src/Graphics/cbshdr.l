%option noyywrap
%option never-interactive
%option nounistd

%%
[ \t\n]											;
[0-9]+\.[0-9]+									{ printf("float number: %s\n", yytext); }
[a-zA-Z0-9\/\.\_]+									{ printf("string: %s\n", yytext); }
%%

int yylex_cbshdr(const char* i_input) {
	YY_BUFFER_STATE buffer = yy_scan_string(i_input);
	yylex();
	yy_delete_buffer(buffer);
	return 0;
}

