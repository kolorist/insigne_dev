#include "MemorySystem.h"

#include <clover.h>
#include <insigne/memory.h>

helich::memory_manager							g_MemoryManager;

// allocators for clover
namespace clover {
	LinearAllocator								g_LinearAllocator;
}

// allocators for insigne
namespace insigne {
	linear_allocator_t							g_persistance_allocator;
	arena_allocator_t							g_arena_allocator;
	freelist_allocator_t						g_stream_allocator;
};

namespace baker {
	LinearAllocator								g_PersistanceAllocator;
}

namespace helich {

void init_memory_system()
{
	using namespace helich;
	g_MemoryManager.initialize(
			memory_region<clover::LinearAllocator>		{ "clover/allocator",			SIZE_MB(16),	&clover::g_LinearAllocator },
			memory_region<insigne::linear_allocator_t>	{ "insigne/persist",			SIZE_MB(512),	&insigne::g_persistance_allocator },
			memory_region<insigne::arena_allocator_t>	{ "insigne/arena",				SIZE_MB(64),	&insigne::g_arena_allocator },
			memory_region<insigne::freelist_allocator_t>{ "insigne/stream",				SIZE_MB(64),	&insigne::g_stream_allocator },
			memory_region<baker::LinearAllocator>		{ "baker/persist",				SIZE_MB(64),	&baker::g_PersistanceAllocator }
			);
}

}
