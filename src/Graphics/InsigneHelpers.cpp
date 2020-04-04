#include "InsigneHelpers.h"

#include <insigne/system.h>
#include <insigne/ut_render.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_textures.h>

#include <math.h>

namespace stone
{
namespace helpers
{
// ---------------------------------------------

// nearest power-of-two
const size find_nearest_pot(const size i_orgSize)
{
	size retSize = 1 << (size)ceil(log2(i_orgSize));
	return retSize;
}

// ---------------------------------------------

SurfaceGPU CreateSurfaceGPU(voidptr i_vtxData, const u32 i_vtxCount, const size i_vtxStride,
		voidptr i_idxData, const u32 i_idxCount,
		insigne::buffer_usage_e i_usage, const bool i_makeCopy /* = true */)
{
	SurfaceGPU surfaceGPU;

	insigne::vbdesc_t vbDesc;
	vbDesc.region_size = find_nearest_pot(i_vtxCount * i_vtxStride);
	vbDesc.stride = i_vtxStride;
	vbDesc.data = i_vtxData;
	vbDesc.count = i_vtxCount;
	vbDesc.usage = i_usage;

	insigne::ibdesc_t ibDesc;
	ibDesc.region_size = find_nearest_pot(i_idxCount * sizeof(s32));
	ibDesc.data = i_idxData;
	ibDesc.count = i_idxCount;
	ibDesc.usage = i_usage;

	if (i_makeCopy)
	{
		surfaceGPU.vb = insigne::copy_create_vb(vbDesc);
		surfaceGPU.ib = insigne::copy_create_ib(ibDesc);
	}
	else
	{
		surfaceGPU.vb = insigne::create_vb(vbDesc);
		surfaceGPU.ib = insigne::create_ib(ibDesc);
	}

	return surfaceGPU;
}

// ---------------------------------------------
}
}
