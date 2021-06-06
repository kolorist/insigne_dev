#include "MemorySystem.h"

#include <clover.h>
#include <refrain2.h>
#include <calyx/memory.h>
#include <insigne/memory.h>
#include <lotus/memory.h>

#include "Graphics/FontRenderer.h"

helich::memory_manager							g_MemoryManager;

// allocators for calyx
namespace calyx {
	allocators_t								s_allocators;
}

// allocators for clover
namespace clover {
	LinearAllocator								g_LinearAllocator;
}

// allocators for refrain2
namespace refrain2 {
	FreelistAllocator							g_TaskAllocator;
	FreelistAllocator							g_TaskDataAllocator;
};

// allocators for insigne
namespace insigne {
	linear_allocator_t							g_persistance_allocator;
	arena_allocator_t							g_arena_allocator;
	freelist_allocator_t						g_stream_allocator;
};

// allocators for lotus
namespace lotus {
	linear_allocator_t							e_main_allocator;
};

// allocators for font_renderer
namespace font_renderer
{
    LinearAllocator                             g_Allocator;
}

namespace stone {
	FreelistArena								g_LoggerArena;

	LinearAllocator								g_SystemAllocator;
	LinearAllocator								g_PersistanceAllocator;
	LinearAllocator								g_PersistanceResourceAllocator;
	LinearAllocator								g_SceneResourceAllocator;
	LinearAllocator								g_StreammingAllocator;
	LinearArena									g_TemporalLinearArena;
	FreelistArena								g_TemporalFreeArena;
	FreelistArena								g_STBArena;
}

namespace helich {

void init_memory_system()
{
	using namespace helich;
	g_MemoryManager.initialize(
			memory_region<calyx::stack_allocator_t> 	{ "calyx/subsystems",			SIZE_MB(1),		&calyx::s_allocators.subsystems_allocator },

			memory_region<clover::LinearAllocator>		{ "clover/allocator",			SIZE_MB(8),		&clover::g_LinearAllocator },

			memory_region<refrain2::FreelistAllocator>	{ "refrain2/task",				SIZE_MB(4),		&refrain2::g_TaskAllocator },
			memory_region<refrain2::FreelistAllocator>	{ "refrain2/taskdata",			SIZE_MB(16),	&refrain2::g_TaskDataAllocator },

			memory_region<insigne::linear_allocator_t>	{ "insigne/persist",			SIZE_MB(256),	&insigne::g_persistance_allocator },
			memory_region<insigne::arena_allocator_t>	{ "insigne/arena",				SIZE_MB(64),	&insigne::g_arena_allocator },
			memory_region<insigne::freelist_allocator_t>{ "insigne/stream",				SIZE_MB(64),	&insigne::g_stream_allocator },

			memory_region<lotus::linear_allocator_t>	{ "lotus/main",					SIZE_MB(32),	&lotus::e_main_allocator },

            memory_region<font_renderer::LinearAllocator> { "stone/font_renderer/main", SIZE_MB(16),     &font_renderer::g_Allocator },

			memory_region<stone::FreelistArena>			{ "stone/logger_arena",			SIZE_MB(8),		&stone::g_LoggerArena },
			memory_region<stone::LinearAllocator>		{ "stone/system",				SIZE_MB(1),		&stone::g_SystemAllocator },
			memory_region<stone::LinearAllocator>		{ "stone/persist",				SIZE_MB(32),	&stone::g_PersistanceAllocator },
			memory_region<stone::LinearAllocator>		{ "stone/persistres",			SIZE_MB(64),	&stone::g_PersistanceResourceAllocator },
			memory_region<stone::LinearAllocator>		{ "stone/sceneres",				SIZE_MB(64),	&stone::g_SceneResourceAllocator },
			memory_region<stone::LinearAllocator>		{ "stone/stream",				SIZE_MB(64),	&stone::g_StreammingAllocator },
			memory_region<stone::FreelistArena>			{ "stone/stb_arena",			SIZE_MB(32),	&stone::g_STBArena }
			);
}

}

namespace stone
{
void SnapshotAllocatorInfos()
{
	u32 allocatorsCount = g_MemoryManager.p_mem_regions_count;
	CLOVER_VERBOSE("==============Memory Info==============");
	CLOVER_VERBOSE("Number of Allocators: %d", allocatorsCount);
	size totalAppMemoryBytes = 0;
	size totalUsedAppMemoryBytes = 0;
	for (u32 i = 0; i < allocatorsCount; i++)
	{
		helich::memory_region_info& inf = g_MemoryManager.p_mem_regions[i];
		CLOVER_VERBOSE(">>  %-25s [0x%x] [%d bytes]", inf.name, inf.base_address, inf.size_in_bytes);
		totalAppMemoryBytes += inf.size_in_bytes;
		helich::debug_memory_block memBlocks[512];
		u32 numAllocs = 0;
		inf.dbg_info_extractor(inf.allocator_ptr, memBlocks, 512, numAllocs);
		size totalUsedBytes = 0;
		for (u32 j = 0; j < numAllocs; j++)
		{
			CLOVER_VERBOSE("    #%4d: [0x%x] [%10d bytes] [%s]", j, memBlocks[j].frame_address, memBlocks[j].frame_size, memBlocks[j].description);
			totalUsedBytes += memBlocks[j].frame_size;
		}
		totalUsedAppMemoryBytes += totalUsedBytes;
		CLOVER_VERBOSE("    Usage: (%lld of %lld bytes) %4.2f %%", totalUsedBytes, inf.size_in_bytes,
				(f32)totalUsedBytes * 100.0f / (f32)inf.size_in_bytes);
	}
	CLOVER_VERBOSE("Total: %lld MB of %lld MB", TO_MB(totalUsedAppMemoryBytes), TO_MB(totalAppMemoryBytes));
}

}
