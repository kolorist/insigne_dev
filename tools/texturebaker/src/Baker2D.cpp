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
void AddCoeffs(floral::fixed_array<f32, LinearArena>& o_result,
		const floral::fixed_array<f32, LinearArena>& i_a,
		const floral::fixed_array<f32, LinearArena>& i_b, const u32 i_order)
{
	u32 numCoeffs = i_order * i_order;
	for (u32 i = 0; i < numCoeffs; i++) {
		o_result[i] = i_a[i] + i_b[i];
	}
}

void ScalarScaleCoeffs(floral::fixed_array<f32, LinearArena>& o_result,
		const floral::fixed_array<f32, LinearArena>& i_a,
		const f32 i_scale, const u32 i_order)
{
	u32 numCoeffs = i_order * i_order;
	for (u32 i = 0; i < numCoeffs; i++) {
		o_result[i] = i_a[i] * i_scale;
	}
}

void computeSHForDirection(floral::fixed_array<f32, LinearArena>& o_result, const u32 i_order, const floral::vec3f& i_dir)
{
   // calculate coefficients for first 3 bands of spherical harmonics
   f64 p_0_0 = 0.282094791773878140;
   f64 p_1_0 = 0.488602511902919920 * i_dir.z;
   f64 p_1_1 = -0.488602511902919920;
   f64 p_2_0 = 0.946174695757560080 * i_dir.z * i_dir.z - 0.315391565252520050;
   f64 p_2_1 = -1.092548430592079200 * i_dir.z;
   f64 p_2_2 = 0.546274215296039590;
   o_result[0] = p_0_0;
   o_result[1] = p_1_1 * i_dir.y;
   o_result[2] = p_1_0;
   o_result[3] = p_1_1 * i_dir.x;
   o_result[4] = p_2_2 * (i_dir.x * i_dir.y + i_dir.y * i_dir.x);
   o_result[5] = p_2_1 * i_dir.y;
   o_result[6] = p_2_0;
   o_result[7] = p_2_1 * i_dir.x;
   o_result[8] = p_2_2 * (i_dir.x * i_dir.x - i_dir.y * i_dir.y);
}

void ComputeSH(const_cstr i_inputTexPath)
{
	CLOVER_INFO("Computing Spherial Harmonics Co-efficients...");
	CLOVER_INFO("Input Texture: %s", i_inputTexPath);

	g_TemporalArena.free_all();

	s32 x, y, n;
	// the stbi_loadf() will load the image with the order from top-down scanlines (y), left-right pixels (x)
	f32* data = stbi_loadf(i_inputTexPath, &x, &y, &n, 0);
	CLOVER_DEBUG(
			"Original image loaded:\n"
			"  - Resolution: %d x %d\n"
			"  - Color Channels: %d",
			x, y, n);
	s32 faceWidth = x / 6;
	f32 mR = 0.0f, mG = 0.0f, mB = 0.0f;
	for (u32 i = 0; i < y; i++)
		for (u32 j = 0; j < x; j++) {
			if (data[i * x + j + 1] > mR) mR = data[i * x + j + 1];
			if (data[i * x + j + 2] > mG) mG = data[i * x + j + 2];
			if (data[i * x + j + 3] > mB) mB = data[i * x + j + 3];
		}

	const u32 order = 3;
	const u32 sqOrder = order * order;
	floral::fixed_array<floral::vec3f, LinearArena> output(sqOrder, &g_TemporalArena);
	output.resize_ex(sqOrder);
	floral::fixed_array<f32, LinearArena> resultR(sqOrder, &g_TemporalArena);
	resultR.resize_ex(sqOrder);
	floral::fixed_array<f32, LinearArena> resultG(sqOrder, &g_TemporalArena);
	resultG.resize_ex(sqOrder);
	floral::fixed_array<f32, LinearArena> resultB(sqOrder, &g_TemporalArena);
	resultB.resize_ex(sqOrder);


	floral::fixed_array<f32, LinearArena> shBuff(sqOrder, &g_TemporalArena);
	shBuff.resize_ex(sqOrder);
	floral::fixed_array<f32, LinearArena> shBuffB(sqOrder, &g_TemporalArena);
	shBuffB.resize_ex(sqOrder);

	for (u32 shc = 0; shc < 6; shc++) {
		CLOVER_DEBUG("---");
		f32 fWt = 0.0f;
		for (u32 i = 0; i < sqOrder; i++) {
			output[i] = floral::vec3f(0.0f);
			resultR[i] = 0.0f;
			resultG[i] = 0.0f;
			resultB[i] = 0.0f;
			shBuff[i] = 0.0f;
			shBuffB[i] = 0.0f;
		}
		for (u32 f = 0; f < 6; f++) {
			// convert texel coordinate to cubemap direction
			f32 invWidth = 1.0f / f32(faceWidth);
			f32 negBound = -1.0f + invWidth;
			f32 invWidthBy2 = 2.0f / f32(faceWidth);
			for (s32 y = 0; y < faceWidth; y++) {
				const f32 fV = negBound + f32(y) + invWidthBy2;
				for (s32 x = 0; x < faceWidth; x++) {
					const f32 fU = negBound + f32(x) + invWidthBy2;
					floral::vec3f dir;
					switch (f) {
						case 0:
							dir.x = 1.0f;
							dir.y = 1.0f - (invWidthBy2 * float(y) + invWidth);
							dir.z = 1.0f - (invWidthBy2 * float(x) + invWidth);
							dir = -dir;
							break;
						case 1:
							dir.x = -1.0f;
							dir.y = 1.0f - (invWidthBy2 * float(y) + invWidth);
							dir.z = -1.0f + (invWidthBy2 * float(x) + invWidth);
							dir = -dir;
							break;
						case 2:
							dir.x = - 1.0f + (invWidthBy2 * float(x) + invWidth);
							dir.y = 1.0f;
							dir.z = - 1.0f + (invWidthBy2 * float(y) + invWidth);
							dir = -dir;
							break;
						case 3:
							dir.x = - 1.0f + (invWidthBy2 * float(x) + invWidth);
							dir.y = - 1.0f;
							dir.z = 1.0f - (invWidthBy2 * float(y) + invWidth);
							dir = -dir;
							break;
						case 4:
							dir.x = - 1.0f + (invWidthBy2 * float(x) + invWidth);
							dir.y = 1.0f - (invWidthBy2 * float(y) + invWidth);
							dir.z = 1.0f;
							break;
						case 5:
							dir.x = 1.0f - (invWidthBy2 * float(x) + invWidth);
							dir.y = 1.0f - (invWidthBy2 * float(y) + invWidth);
							dir.z = - 1.0f;
							break;
						default:
							break;
					}

					dir = dir.normalize();
					const f32 fDiffSolid = 4.0f / ((1.0f + fU*fU + fV*fV) *
							sqrtf(1.0f + fU*fU + fV*fV));
					fWt += fDiffSolid;
					//calculate SH for this direction
					computeSHForDirection(shBuff, order, dir);

					size pixelIdx = (y + shc * faceWidth) * faceWidth + x + f * faceWidth;
					floral::vec3f color(data[pixelIdx] / mR, data[pixelIdx + 1] / mG, data[pixelIdx + 2] / mB);

					//scale and add to accumulated coeffs
					ScalarScaleCoeffs(shBuffB, shBuff, color.x * fDiffSolid, order);
					AddCoeffs(resultR, resultR, shBuffB, order);
					ScalarScaleCoeffs(shBuffB, shBuffB, color.y * fDiffSolid, order);
					AddCoeffs(resultG, resultG, shBuffB, order);
					ScalarScaleCoeffs(shBuffB, shBuffB, color.z * fDiffSolid, order);
					AddCoeffs(resultB, resultB, shBuffB, order);
				}
			}
		}

		// final scale
		const f32 fNormProj = (4.0f * 3.1415f) / fWt;
		ScalarScaleCoeffs(resultR, resultR, fNormProj, order);
		ScalarScaleCoeffs(resultG, resultG, fNormProj, order);
		ScalarScaleCoeffs(resultB, resultB, fNormProj, order);

		for (u32 i = 0; i < sqOrder; i++) {
			output[i].x = resultR[i];
			output[i].y = resultG[i];
			output[i].z = resultB[i];

			CLOVER_DEBUG("coeff %d: (%f %f %f)", i, output[i].x, output[i].y, output[i].z);
		}
	}

	delete[] data;
}

}
