#include "FormFactorUtils.h"

#include <random>

namespace ffutils
{

static const f32 k_PId2 = 1.570796326794896619f;	/* pi / 2 */
static const f32 k_PIt2inv = 0.159154943091895346f;	/* 1 / (2 * pi) */

const f32 compute_accurate_point2patch_form_factor(
	const floral::vec3f i_quad[], const floral::vec3f& i_point, const floral::vec3f& i_pointNormal)
{
	floral::vec3f PA = i_quad[0] - i_point;
	floral::vec3f PB = i_quad[1] - i_point;
	floral::vec3f PC = i_quad[2] - i_point;
	floral::vec3f PD = i_quad[3] - i_point;

	floral::vec3f gAB = floral::cross(PA, PB);
	floral::vec3f gBC = floral::cross(PB, PC);
	floral::vec3f gCD = floral::cross(PC, PD);
	floral::vec3f gDA = floral::cross(PD, PA);

	f32 NdotGAB = floral::dot(i_pointNormal, gAB);
	f32 NdotGBC = floral::dot(i_pointNormal, gBC);
	f32 NdotGCD = floral::dot(i_pointNormal, gCD);
	f32 NdotGDA = floral::dot(i_pointNormal, gDA);

	f32 sum = 0.0f;

	if (fabs(NdotGAB) > 0.0f)
	{
		f32 lenGAB = floral::length(gAB);
		if (lenGAB > 0.0f)
		{
			f32 gammaAB = k_PId2 - atanf(floral::dot(PA, PB) / lenGAB);
			sum += NdotGAB * gammaAB / lenGAB;
		}
	}

	if (fabs(NdotGBC) > 0.0f)
	{
		f32 lenGBC = floral::length(gBC);
		if (lenGBC > 0.0f)
		{
			f32 gammaBC = k_PId2 - atanf(floral::dot(PB, PC) / lenGBC);
			sum += NdotGBC * gammaBC / lenGBC;
		}
	}

	if (fabs(NdotGCD) > 0.0f)
	{
		f32 lenGCD = floral::length(gCD);
		if (lenGCD > 0.0f)
		{
			f32 gammaCD = k_PId2 - atanf(floral::dot(PC, PD) / lenGCD);
			sum += NdotGCD * gammaCD / lenGCD;
		}
	}

	if (fabs(NdotGDA) > 0.0f)
	{
		f32 lenGDA = floral::length(gDA);
		if (lenGDA > 0.0f)
		{
			f32 gammaDA = k_PId2 - atanf(floral::dot(PD, PA) / lenGDA);
			sum += NdotGDA * gammaDA / lenGDA;
		}
	}

	sum *= k_PIt2inv;
	return sum;
}

const floral::vec3f get_random_point_on_quad(const floral::vec3f i_quad[])
{
	static std::random_device s_rd;
	static std::mt19937 s_gen(s_rd());
	static std::uniform_real_distribution<f32> s_dis(0.0f, 1.0f);

	floral::vec3f u = i_quad[1] - i_quad[0];
	floral::vec3f v = i_quad[3] - i_quad[0];
	floral::vec3f p = i_quad[0] + u * s_dis(s_gen) + v * s_dis(s_gen);
	return p;
}

const f32 compute_patch2patch_form_factor(
	const floral::vec3f i_srcQuad[], const floral::vec3f& i_srcNorm,
	const floral::vec3f i_dstQuad[], const floral::vec3f& i_dstNorm, const s32 i_numSamples)
{
	f32 sum = 0.0f;

	// generate sample points
	for (s32 i = 0; i < i_numSamples; i++)
	{
		floral::vec3f pi = get_random_point_on_quad(i_srcQuad);
		f32 df = 0.0f, dg = 0.0f;
		for (s32 j = 0; j < i_numSamples; j++)
		{
			f32 r = 0.0f;
			floral::vec3f pj, pij;
			while (r <= 0.001f)
			{
				pj = get_random_point_on_quad(i_dstQuad);
				pij = pj - pi;
				r = floral::length(pij);
			}

			floral::vec3f nij = floral::normalize(pij);
			floral::vec3f nji = -nij;
			f32 vis = 1.0f; // hit other geometry? no for now
			f32 gVis = floral::dot(nij, i_srcNorm) * floral::dot(nji, i_dstNorm) / (3.141592f * r * r);

			if (gVis > 0.0f)
			{
				df += gVis * vis;
				dg += gVis;
			}
		}

		f32 gVisJ = compute_accurate_point2patch_form_factor(i_dstQuad, pi, i_srcNorm);
		if (gVisJ > 0.0f)
		{
			sum += (df / dg) *  (1.0f / i_numSamples) * gVisJ;
		}
	}

	return sum;
}

}
