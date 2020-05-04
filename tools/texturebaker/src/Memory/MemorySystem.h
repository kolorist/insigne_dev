#pragma once

#include <floral.h>
#include <helich.h>

namespace texbaker
{
// -------------------------------------------------------------------

typedef helich::allocator<helich::stack_scheme, helich::no_tracking_policy>		LinearAllocator;
typedef helich::allocator<helich::stack_scheme, helich::no_tracking_policy>		LinearArena;

extern LinearAllocator							g_PersistanceAllocator;
extern LinearArena								g_TemporalArena;

// -------------------------------------------------------------------
}
