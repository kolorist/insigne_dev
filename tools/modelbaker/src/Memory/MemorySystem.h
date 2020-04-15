#pragma once

#include <floral.h>
#include <helich.h>

extern helich::memory_manager					g_MemoryManager;

namespace baker
{

typedef helich::allocator<helich::stack_scheme, helich::no_tracking_policy>		LinearAllocator;
typedef helich::allocator<helich::freelist_scheme, helich::no_tracking_policy>	FreelistAllocator;
typedef helich::allocator<helich::stack_scheme, helich::no_tracking_policy>		LinearArena;
typedef helich::allocator<helich::freelist_scheme, helich::no_tracking_policy>	FreelistArena;

extern LinearAllocator							g_PersistanceAllocator;
extern FreelistAllocator						g_TemporalAllocator;
extern FreelistAllocator						g_ParserAllocator;
extern FreelistArena							g_TemporalArena;

}
