#pragma once

#include <floral.h>
#include <helich.h>

extern helich::memory_manager					g_MemoryManager;

namespace stone {
	typedef helich::allocator<helich::stack_scheme, helich::no_tracking_policy>		LinearAllocator;

	extern LinearAllocator						g_PersistanceAllocator;
}
