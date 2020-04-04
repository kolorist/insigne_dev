#pragma once
#include <insigne/commons.h>

namespace stone
{
namespace helpers
{
// ---------------------------------------------

struct SurfaceGPU
{
	insigne::vb_handle_t						vb;
	insigne::ib_handle_t						ib;
};

// ---------------------------------------------

SurfaceGPU										CreateSurfaceGPU(voidptr i_vtxData, const u32 i_vtxCount, const size i_vtxStride,
													voidptr i_idxData, const u32 i_idxCount,
													insigne::buffer_usage_e i_usage, const bool i_makeCopy = true);

// ---------------------------------------------
}
}
