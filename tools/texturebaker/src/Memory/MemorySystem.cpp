#include "MemorySystem.h"

#include <clover.h>

helich::memory_manager							g_MemoryManager;

namespace texbaker
{
	LinearAllocator								g_PersistanceAllocator;
	LinearArena									g_TemporalArena;
}

namespace clover
{
	LinearAllocator								g_LinearAllocator;
}

namespace helich
{
// -------------------------------------------------------------------

void init_memory_system()
{
	g_MemoryManager.initialize(
			memory_region<clover::LinearAllocator>		{ "clover/allocator",			SIZE_MB(16),		&clover::g_LinearAllocator },
			memory_region<texbaker::LinearAllocator>	{ "texbaker/persist",			SIZE_MB(32),		&texbaker::g_PersistanceAllocator },
			memory_region<texbaker::LinearArena>		{ "texbaker/arena",				SIZE_MB(128),		&texbaker::g_TemporalArena }
			);
}
// -------------------------------------------------------------------
// -------------------------------------------------------------------
}
