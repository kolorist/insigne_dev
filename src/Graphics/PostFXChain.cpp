#include "PostFXChain.h"

namespace pfx_chain
{
// -------------------------------------------------------------------

// 8xTAA
static const floral::vec2f s_sampleLocs[8] = {
	floral::vec2f(-7.0f, 1.0f) / 8.0f,
	floral::vec2f(-5.0f, -5.0f) / 8.0f,
	floral::vec2f(-1.0f, -3.0f) / 8.0f,
	floral::vec2f(3.0f, -7.0f) / 8.0f,
	floral::vec2f(5.0f, -1.0f) / 8.0f,
	floral::vec2f(7.0f, 7.0f) / 8.0f,
	floral::vec2f(1.0f, 3.0f) / 8.0f,
	floral::vec2f(-3.0f, 5.0f) / 8.0f
};

floral::mat4x4f get_jittered_matrix(const size i_frameIdx, const f32 i_width, const f32 i_height)
{
	const size idx = i_frameIdx % 8;

	const floral::vec2f k_texelSize(1.0f / i_width, 1.0f / i_height);
	const floral::vec2f k_subSampleSize = k_texelSize * 2.0f; // That is the size of the subsample in NDC

	const floral::vec2f sloc = s_sampleLocs[idx];

	floral::vec2f subSample = sloc * k_subSampleSize * 0.5f;

	return floral::construct_translation3d(subSample.x, subSample.y, 0.0f);
}

// -------------------------------------------------------------------
}
