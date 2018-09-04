#include <floral.h>
#include <helich.h>

#include "Memory/MemorySystem.h"

extern int yylex_cbobj(const char* i_input);

int main(void)
{
	// we have to call it ourself as we do not have calyx here
	helich::init_memory_system();

	yylex_cbobj("# Blender v2.78 (sub 0) OBJ File: 'cornell_box.blend'\n"
			"# www.blender.org\n"
			"#\n"
			"mtllib cornell_box.mtl\n"
			"g Plane.004\n"
			"ss\n"
			"v 1.000000 2.000000 1.000000\n"
			"v -1.000000 2.000000 1.000000\n"
			"v 1.000000 2.000000 -1.000000\n"
			"v -1.000000 2.000000 -1.000000\n"
			"vn 0.0000 -1.0000 0.0000\n"
			"usemtl None\n"
			"s off\n"
			"f 2//1 3//1 1//1\n"
			"f 2//1 4//1 3//1\n"
			"g Plane.003\n"
			"v -1.000000 -0.000000 -1.000000\n"
			"v 1.000000 -0.000000 -1.000000\n"
			"v -1.000000 2.000000 -1.000000\n"
			"v 1.000000 2.000000 -1.000000\n"
			"vn 0.0000 -0.0000 1.0000\n"
			"usemtl None");

	/*
	baker::FreelistArena* freelistArena = baker::g_PersistanceAllocator.allocate_arena<baker::FreelistArena>(SIZE_MB(4));
	baker::LinearArena* linearArena = baker::g_PersistanceAllocator.allocate_arena<baker::LinearArena>(SIZE_MB(4));

	p8 memBlock = (p8)freelistArena->allocate(100);
	p8 lmemBlock = (p8)linearArena->allocate(100);
	memBlock[0] = 'a';
	memBlock[1] = 'b';
	memBlock[2] = 'c';
	lmemBlock[0] = 'a';
	lmemBlock[1] = 'b';
	lmemBlock[2] = 'c';

	memBlock = (p8)freelistArena->reallocate(memBlock, 200);
	lmemBlock = (p8)linearArena->reallocate(lmemBlock, 200);
	p8 memBlock2 = (p8)freelistArena->allocate(50);
	p8 lmemBlock2 = (p8)linearArena->allocate(50);

	freelistArena->free(memBlock);

	linearArena->free_all();
	*/

	return 0;
}
