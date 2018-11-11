#include "SHBakingUtils.h"

#include <clover.h>

#include "Memory/MemorySystem.h"

namespace stone {

#define MAX_SH_ORDER							3
#define NUM_SH_COEFF							MAX_SH_ORDER * MAX_SH_ORDER
#define CP_PI									3.14159265359

static void BuildNormalizerHStrip(const s32 i_size, f32* o_output)
{
	s32 stripWidth = i_size * 6;
	//iterate over cube faces
	for(s32 iCubeFace = 0; iCubeFace < 6; iCubeFace++)
	{
		//fast texture walk, build normalizer cube map
		f32 *texelPtr = o_output;
		for(s32 v = 0; v < i_size; v++) // scanline
		{
			for(s32 u = 0; u < i_size; u++) // pixel
			{
				// note that the captured frame buffer image is flipped upside down, thus we dont need to invert the 
				// texture v coordinate
				floral::vec3f cubeCoord = floral::texel_coord_to_cube_coord(iCubeFace, (f32)u, (f32)v, i_size);
				texelPtr[(v * stripWidth + iCubeFace * i_size + u) * 4] = cubeCoord.x;
				texelPtr[(v * stripWidth + iCubeFace * i_size + u) * 4 + 1] = cubeCoord.y;
				texelPtr[(v * stripWidth + iCubeFace * i_size + u) * 4 + 2] = cubeCoord.z;
				texelPtr[(v * stripWidth + iCubeFace * i_size + u) * 4 + 3] =
					floral::texel_coord_to_solid_angle(iCubeFace, (f32)u, (f32)(i_size - 1 - v), i_size);
			}
		}
	}
}

static f64 SHBandFactor[NUM_SH_COEFF] =
{ 1.0,
2.0 / 3.0, 2.0 / 3.0, 2.0 / 3.0,
1.0 / 4.0, 1.0 / 4.0, 1.0 / 4.0, 1.0 / 4.0, 1.0 / 4.0 };

static void EvalSHBasis(const f32* dir, f64* res )
{
	// Can be optimize by precomputing constant.
	static const f64 SqrtPi = sqrt(CP_PI);

	f64 xx = dir[0];
	f64 yy = dir[1];
	f64 zz = dir[2];

	// x[i] == pow(x, i), etc.
	f64 x[MAX_SH_ORDER+1], y[MAX_SH_ORDER+1], z[MAX_SH_ORDER+1];
	x[0] = y[0] = z[0] = 1.;
	for (s32 i = 1; i < MAX_SH_ORDER+1; ++i)
	{
		x[i] = xx * x[i-1];
		y[i] = yy * y[i-1];
		z[i] = zz * z[i-1];
	}

	res[0]  = (1/(2.*SqrtPi));

	res[1]  = -(sqrt(3/CP_PI)*yy)/2.;
	res[2]  = (sqrt(3/CP_PI)*zz)/2.;
	res[3]  = -(sqrt(3/CP_PI)*xx)/2.;

	res[4]  = (sqrt(15/CP_PI)*xx*yy)/2.;
	res[5]  = -(sqrt(15/CP_PI)*yy*zz)/2.;
	res[6]  = (sqrt(5/CP_PI)*(-1 + 3*z[2]))/4.;
	res[7]  = -(sqrt(15/CP_PI)*xx*zz)/2.;
	res[8]  = sqrt(15/CP_PI)*(x[2] - y[2])/4.;
}

void ComputeSH(f64* o_shr, f64* o_shg, f64* o_shb, f32* i_envMap)
{
	s32 width = 1536;
	s32 height = 256;
	// the stbi_loadf() will load the image with the order from top-down scanlines (y), left-right pixels (x)
	f32* data = i_envMap;
	s32 faceWidth = width / 6;
	s32 probesCount = height / faceWidth;

	f32* normalizeData = g_TemporalLinearArena.allocate_array<f32>(faceWidth * faceWidth * 4 * 6);
	BuildNormalizerHStrip(faceWidth, normalizeData);

	for (s32 pc = 0; pc < probesCount; pc++) {		
		f64 SHdir[NUM_SH_COEFF];
		memset(o_shr, 0, NUM_SH_COEFF * sizeof(f64));
		memset(o_shg, 0, NUM_SH_COEFF * sizeof(f64));
		memset(o_shb, 0, NUM_SH_COEFF * sizeof(f64));
		memset(SHdir, 0, NUM_SH_COEFF * sizeof(f64));

		f64 weightAccum = 0.0;
		f64 weight = 0.0;
		
		for (s32 iFaceIdx = 0; iFaceIdx < 6; iFaceIdx++)
		{
			for (s32 y = 0; y < faceWidth; y++) // scanline
			{
				s32 yy = y + pc * faceWidth;

				for (s32 x = 0; x < faceWidth; x++) // pixel
				{
					f32* texelVect = &normalizeData[(y * width + iFaceIdx * faceWidth + x) * 4];
					weight = *(texelVect + 3);

					EvalSHBasis(texelVect, SHdir);

					// Convert to f64
					f64 R = data[(yy * width + iFaceIdx * faceWidth + x) * 3];
					f64 G = data[(yy * width + iFaceIdx * faceWidth + x) * 3 + 1];
					f64 B = data[(yy * width + iFaceIdx * faceWidth + x) * 3 + 2];

					for (s32 i = 0; i < NUM_SH_COEFF; i++)
					{
						o_shr[i] += R * SHdir[i] * weight;
						o_shg[i] += G * SHdir[i] * weight;
						o_shb[i] += B * SHdir[i] * weight;
					}

					weightAccum += weight;
				}
			}
		}
		
		CLOVER_DEBUG("%d ---", pc);
		for (s32 i = 0; i < NUM_SH_COEFF; ++i)
		{
			o_shr[i] *= 4.0 * CP_PI / weightAccum;
			o_shg[i] *= 4.0 * CP_PI / weightAccum;
			o_shb[i] *= 4.0 * CP_PI / weightAccum;

			CLOVER_DEBUG("floral::vec4f(%ff, %ff, %ff, 0.0f);", o_shr[i], o_shg[i], o_shb[i]);
		}
	}
}

}
