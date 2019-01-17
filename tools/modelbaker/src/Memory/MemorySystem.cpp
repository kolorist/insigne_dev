#include "MemorySystem.h"

#include <clover.h>

helich::memory_manager							g_MemoryManager;

// allocators for clover
namespace clover {
	LinearAllocator								g_LinearAllocator;
}

namespace baker {
	LinearAllocator								g_PersistanceAllocator;
	FreelistAllocator							g_ParserAllocator;
}

namespace helich {

void init_memory_system()
{
	using namespace helich;
	g_MemoryManager.initialize(
			memory_region<clover::LinearAllocator>		{ "clover/allocator",			SIZE_MB(16),	&clover::g_LinearAllocator },
			memory_region<baker::LinearAllocator>		{ "baker/persist",				SIZE_MB(512),	&baker::g_PersistanceAllocator },
			memory_region<baker::FreelistAllocator>		{ "baker/parser",				SIZE_MB(64),	&baker::g_ParserAllocator }
			);
}

}
