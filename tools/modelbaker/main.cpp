#include <stdio.h>

extern int yylex_pbrtv3(const char* i_input);

int main(void)
{
	FILE* f = fopen("scene.pbrt", "rb");
	char* buff = new char[8192];
	fseek(f, 0, SEEK_END);
	size_t fSize = ftell(f);
	rewind(f);
	fread(buff, 1, fSize, f);
	buff[fSize] = 0;

	yylex_pbrtv3(buff);

	return 0;
}
