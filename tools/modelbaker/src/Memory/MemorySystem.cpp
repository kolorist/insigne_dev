#include "MemorySystem.h"

#include <clover.h>
#include <insigne/memory.h>

helich::memory_manager							g_MemoryManager;

// allocators for clover
namespace clover {
	LinearAllocator								g_LinearAllocator;
}

namespace baker {
	LinearAllocator								g_PersistanceAllocator;
}

namespace helich {

void init_memory_system()
{
	using namespace helich;
	g_MemoryManager.initialize(
			memory_region<clover::LinearAllocator>		{ "clover/allocator",			SIZE_MB(16),	&clover::g_LinearAllocator },
			memory_region<baker::LinearAllocator>		{ "baker/persist",				SIZE_MB(512),	&baker::g_PersistanceAllocator }
			);
}

}
