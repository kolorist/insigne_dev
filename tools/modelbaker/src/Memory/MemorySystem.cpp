#include "MemorySystem.h"

#include <clover.h>

helich::memory_manager							g_MemoryManager;

// allocators for clover
namespace clover
{
	LinearAllocator								g_LinearAllocator;
}

namespace baker
{
	LinearAllocator								g_PersistanceAllocator;
	FreelistAllocator							g_TemporalAllocator;
	FreelistAllocator							g_ParserAllocator;
	FreelistArena								g_TemporalArena;
}

namespace helich
{
// ------------------------------------------------------------------

void init_memory_system()
{
	using namespace helich;
	g_MemoryManager.initialize(
			memory_region<clover::LinearAllocator>		{ "clover/allocator",			SIZE_MB(16),	&clover::g_LinearAllocator },
			memory_region<baker::LinearAllocator>		{ "baker/persist",				SIZE_MB(64),	&baker::g_PersistanceAllocator },
			memory_region<baker::FreelistAllocator>		{ "baker/tmpallocator",			SIZE_MB(256),	&baker::g_TemporalAllocator },
			memory_region<baker::FreelistAllocator>		{ "baker/parser",				SIZE_MB(64),	&baker::g_ParserAllocator },
			memory_region<baker::FreelistArena>			{ "baker/temporal",				SIZE_MB(64),	&baker::g_TemporalArena }
			);
}

// ------------------------------------------------------------------
}
