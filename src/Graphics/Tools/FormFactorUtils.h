#pragma once

#include <floral/stdaliases.h>
#include <floral/gpds/vec.h>

namespace ffutils
{

/*
 * returns
 *  - positive IF i_quad vertices are declared in clockwise order
 *  - negative IF i_quad vertices are declared in counter-clockwise order
 */
const f32 compute_accurate_point2patch_form_factor(
	const floral::vec3f i_quad[], const floral::vec3f& i_point, const floral::vec3f& i_pointNormal);

const floral::vec3f get_random_point_on_quad(const floral::vec3f i_quad[]);

const f32 compute_patch2patch_form_factor(
	const floral::vec3f i_srcQuad[], const floral::vec3f& i_srcNorm,
	const floral::vec3f i_dstQuad[], const floral::vec3f& i_dstNorm, const s32 i_numSamples);
}
