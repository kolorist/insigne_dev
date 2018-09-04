#include <floral.h>

#include "Memory/MemorySystem.h"

#include "OBJModelBaker.h"

int main(int argc, char** argv)
{
	// we have to call it ourself as we do not have calyx here
	helich::init_memory_system();

	const_cstr inpFilePath = argv[1];
	const_cstr outDir = argv[2];

	baker::OBJModelBaker bakery(inpFilePath, outDir);
	bakery.BakeToFile();

	return 0;
}
