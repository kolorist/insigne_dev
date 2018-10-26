#include "Baker2D.h"

#include <clover.h>

#include "Memory/MemorySystem.h"
#include "CBFormats.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

namespace texbaker {

void ConvertTexture2D(const_cstr i_inputTexPath, const_cstr i_outputTexPath, const s32 i_maxMips)
{
	CLOVER_INFO("Input texture:  %s", i_inputTexPath);
	CLOVER_INFO("Output texture: %s", i_outputTexPath);
	CLOVER_INFO(
			"Conversion settings:\n"
			"  - Max mips: %d",
			i_maxMips);

	CLOVER_INFO("Begin conversion...");
	CLOVER_DEBUG("Loading original image '%s'...", i_inputTexPath);
	s32 x, y, n;
	p8 data = stbi_load(i_inputTexPath, &x, &y, &n, 0);
	CLOVER_DEBUG(
			"Original image loaded:\n"
			"  - Resolution: %d x %d\n"
			"  - Color Channels: %d",
			x, y, n);

	FILE* fp = fopen(i_outputTexPath, "wb");

	cymbi::CBTexture2DHeader header;
	memcpy(header.magicCharacters, "CBFM", 4);
	header.colorRange = cymbi::ColorRange::HDR;
	header.colorSpace = cymbi::ColorSpace::Linear;
	header.colorChannel = cymbi::ColorChannel::RGB;
	header.encodedGamma = 1.0f;

	// mips count?
	s32 mipsCount = (s32)log2(x) + 1;
	s32 maxMips = 0;
	s32 mipOffset = 0;

	if (i_maxMips > 0) {
		maxMips = i_maxMips;
		mipOffset = 0;
		if (mipsCount > maxMips) {
			mipOffset = mipsCount - maxMips;
			mipsCount = maxMips;
		}
	} else {
		maxMips = mipsCount;
	}

	header.mipsCount = mipsCount;

	CLOVER_DEBUG("Settled at mips count = %d", header.mipsCount);

	fwrite((p8)&header, sizeof(cymbi::CBTexture2DHeader), 1, fp);

	for (s32 i = 1 + mipOffset; i <= mipsCount + mipOffset; i++) {
		s32 nx = x >> (i - 1);
		s32 ny = y >> (i - 1);
		CLOVER_DEBUG("Creating mip #%d at size %dx%d...", i - mipOffset, nx, ny);
		if (i == 1) {
			fwrite(data, sizeof(u8), nx * ny * n, fp);
		} else {
			p8 rzdata = g_PersistanceAllocator.allocate_array<u8>(nx * ny * n);
			stbir_resize_uint8_srgb(
					data, x, y, 0,
					rzdata, nx, ny, 0,
					n, STBIR_ALPHA_CHANNEL_NONE, 0);
			fwrite(rzdata, sizeof(u8), nx * ny * n, fp);
			g_PersistanceAllocator.free(rzdata);
		}
	}

	fclose(fp);
	CLOVER_INFO("All done. Enjoy :D");
}

//----------------------------------------------
#define MAX_SH_ORDER							3
#define NUM_SH_COEFF							MAX_SH_ORDER * MAX_SH_ORDER
#define CP_PI									3.14159265359

void BuildNormalizerHStrip(const s32 i_size, f32* o_output)
{
	s32 stripWidth = i_size * 6;
	//iterate over cube faces
	for(s32 iCubeFace = 0; iCubeFace < 6; iCubeFace++)
	{
		//fast texture walk, build normalizer cube map
		f32 *texelPtr = o_output;
		for(s32 v = 0; v < i_size; v++) // scanline
		{
			for(s32 u=0; u < i_size; u++) // pixel
			{
				floral::vec3f cubeCoord = floral::texel_coord_to_cube_coord(iCubeFace, (f32)u, (f32)v, i_size);
				texelPtr[(v * stripWidth + iCubeFace * i_size + u) * 4] = cubeCoord.x;
				texelPtr[(v * stripWidth + iCubeFace * i_size + u) * 4 + 1] = cubeCoord.y;
				texelPtr[(v * stripWidth + iCubeFace * i_size + u) * 4 + 2] = cubeCoord.z;
				texelPtr[(v * stripWidth + iCubeFace * i_size + u) * 4 + 3] =
					floral::texel_coord_to_solid_angle(iCubeFace, (f32)u, (f32)v, i_size);
			}
		}
	}
}

static f64 SHBandFactor[NUM_SH_COEFF] =
{ 1.0,
2.0 / 3.0, 2.0 / 3.0, 2.0 / 3.0,
1.0 / 4.0, 1.0 / 4.0, 1.0 / 4.0, 1.0 / 4.0, 1.0 / 4.0 };
#if 0
	0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, // The 4 band will be zeroed
	-1.0 / 24.0, -1.0 / 24.0, -1.0 / 24.0, -1.0 / 24.0, -1.0 / 24.0, -1.0 / 24.0, -1.0 / 24.0, -1.0 / 24.0, -1.0 / 24.0 };
#endif

void EvalSHBasis(const f32* dir, f64* res )
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
#if 0
	res[9]  = (sqrt(35/(2.*CP_PI))*(-3*x[2]*yy + y[3]))/4.;
	res[10] = (sqrt(105/CP_PI)*xx*yy*zz)/2.;
	res[11] = -(sqrt(21/(2.*CP_PI))*yy*(-1 + 5*z[2]))/4.;
	res[12] = (sqrt(7/CP_PI)*zz*(-3 + 5*z[2]))/4.;
	res[13] = -(sqrt(21/(2.*CP_PI))*xx*(-1 + 5*z[2]))/4.;
	res[14] = (sqrt(105/CP_PI)*(x[2] - y[2])*zz)/4.;
	res[15] = -(sqrt(35/(2.*CP_PI))*(x[3] - 3*xx*y[2]))/4.;

	res[16] = (3*sqrt(35/CP_PI)*xx*yy*(x[2] - y[2]))/4.;
	res[17] = (-3*sqrt(35/(2.*CP_PI))*(3*x[2]*yy - y[3])*zz)/4.;
	res[18] = (3*sqrt(5/CP_PI)*xx*yy*(-1 + 7*z[2]))/4.;
	res[19] = (-3*sqrt(5/(2.*CP_PI))*yy*zz*(-3 + 7*z[2]))/4.;
	res[20] = (3*(3 - 30*z[2] + 35*z[4]))/(16.*SqrtPi);
	res[21] = (-3*sqrt(5/(2.*CP_PI))*xx*zz*(-3 + 7*z[2]))/4.;
	res[22] = (3*sqrt(5/CP_PI)*(x[2] - y[2])*(-1 + 7*z[2]))/8.;
	res[23] = (-3*sqrt(35/(2.*CP_PI))*(x[3] - 3*xx*y[2])*zz)/4.;
	res[24] = (3*sqrt(35/CP_PI)*(x[4] - 6*x[2]*y[2] + y[4]))/16.;
#endif
}

void ComputeSH(const_cstr i_inputTexPath, const_cstr i_outputPath)
{
	CLOVER_INFO("Computing Spherial Harmonics Co-efficients...");
	CLOVER_INFO("Input Texture: %s", i_inputTexPath);

	g_TemporalArena.free_all();

	s32 width, height, numChannels;
	// the stbi_loadf() will load the image with the order from top-down scanlines (y), left-right pixels (x)
	f32* data = stbi_loadf(i_inputTexPath, &width, &height, &numChannels, 0);
	CLOVER_DEBUG(
			"Original image loaded:\n"
			"  - Resolution: %d x %d\n"
			"  - Color Channels: %d",
			width, height, numChannels);
	s32 faceWidth = width / 6;
	s32 probesCount = height / faceWidth;

	FILE* fp = fopen(i_outputPath, "wb");

	f32* normalizeData = g_TemporalArena.allocate_array<f32>(faceWidth * faceWidth * 4 * 6);
	BuildNormalizerHStrip(faceWidth, normalizeData);

	for (s32 pc = 0; pc < probesCount; pc++) {		
		f64 SHr[NUM_SH_COEFF];
		f64 SHg[NUM_SH_COEFF];
		f64 SHb[NUM_SH_COEFF];
		f64 SHdir[NUM_SH_COEFF];
		memset(SHr, 0, NUM_SH_COEFF * sizeof(f64));
		memset(SHg, 0, NUM_SH_COEFF * sizeof(f64));
		memset(SHb, 0, NUM_SH_COEFF * sizeof(f64));
		memset(SHdir, 0, NUM_SH_COEFF * sizeof(f64));

		f64 weightAccum = 0.0;
		f64 weight = 0.0;
		
		for (s32 iFaceIdx = 0; iFaceIdx < 6; iFaceIdx++)
		{
			for (s32 y = 0; y < faceWidth; y++) // scanline
			{
				s32 yy = y + pc * faceWidth;
				//normCubeRowStartPtr = &m_NormCubeMap[iFaceIdx].m_ImgData[NormCubeMapNumChannels * (y * faceWidth)];
				//srcCubeRowStartPtr	= &SrcCubeImage[iFaceIdx].m_ImgData[SrcCubeMapNumChannels * (y * faceWidth)];

				for (s32 x = 0; x < faceWidth; x++) // pixel
				{
					//pointer to direction and solid angle in cube map associated with texel
					//texelVect = &normCubeRowStartPtr[NormCubeMapNumChannels * x];
					f32* texelVect = &normalizeData[(y * width + iFaceIdx * faceWidth + x) * 4];
					weight = *(texelVect + 3);

					EvalSHBasis(texelVect, SHdir);

					// Convert to f64
					f64 R = data[(yy * width + iFaceIdx * faceWidth + x) * 3];
					f64 G = data[(yy * width + iFaceIdx * faceWidth + x) * 3 + 1];
					f64 B = data[(yy * width + iFaceIdx * faceWidth + x) * 3 + 2];

					for (s32 i = 0; i < NUM_SH_COEFF; i++)
					{
						SHr[i] += R * SHdir[i] * weight;
						SHg[i] += G * SHdir[i] * weight;
						SHb[i] += B * SHdir[i] * weight;
					}

					weightAccum += weight;
				}
			}
		}
		
		CLOVER_DEBUG("%d ---", pc);
		//Normalization - The sum of solid angle should be equal to the solid angle of the sphere (4 PI), so
		// normalize in order our weightAccum exactly match 4 PI.
		for (s32 i = 0; i < NUM_SH_COEFF; ++i)
		{
			SHr[i] *= 4.0 * CP_PI / weightAccum;
			SHg[i] *= 4.0 * CP_PI / weightAccum;
			SHb[i] *= 4.0 * CP_PI / weightAccum;

			floral::vec3f v(SHr[i], SHg[i], SHb[i]);
			fwrite(&v, sizeof(floral::vec3f), 1, fp);

			CLOVER_DEBUG("floral::vec4f(%ff, %ff, %ff, 0.0f);", SHr[i], SHg[i], SHb[i]);
		}
	}

	fclose(fp);
	g_TemporalArena.free_all();
	delete[] data;
}

}
