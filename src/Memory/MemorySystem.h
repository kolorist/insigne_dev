#pragma once

#include <floral.h>
#include <helich.h>

extern helich::memory_manager					g_MemoryManager;

namespace stone {
	typedef helich::allocator<helich::stack_scheme, helich::no_tracking_policy>		LinearAllocator;
	typedef helich::allocator<helich::stack_scheme, helich::no_tracking_policy>		LinearArena;

	extern LinearAllocator						g_SystemAllocator;
	extern LinearAllocator						g_PersistanceAllocator;
	extern LinearAllocator						g_PersistanceResourceAllocator;
	extern LinearAllocator						g_SceneResourceAllocator;
}
