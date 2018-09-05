#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "Memory/MemorySystem.h"

namespace stone {

struct Surface {
	insigne::surface_handle_t					surfaceHdl;
	insigne::material_handle_t					materialHdl;
};

struct Model {
	floral::fixed_array<Surface, LinearAllocator>	surfacesList;
};

}
