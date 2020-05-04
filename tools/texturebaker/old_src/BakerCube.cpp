#include "BakerCube.h"

#include <clover.h>

#include "Memory/MemorySystem.h"
#include "CBFormats.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_resize.h"

namespace texbaker {

/*
 * Coordinate system in baking process has the origin (0, 0) at the topleft corner
 */

void TrimImage(f32* i_imgData, f32* o_outData, const s32 i_x, const s32 i_y, const s32 i_width, const s32 i_height,
		const s32 i_imgWidth, const s32 i_imgHeight, const s32 i_channelCount)
{
	s32 outScanline = 0;
	for (s32 i = i_y; i < (i_y + i_height); i++) {
		memcpy(
				&o_outData[outScanline * i_width * i_channelCount],
				&i_imgData[(i * i_imgWidth + i_x) * i_channelCount],
				i_height * i_channelCount * sizeof(f32));
		outScanline++;
	}
}

void ConvertTextureCubeHStrip(const_cstr i_inputTexPath, const_cstr i_outputTexPath, const s32 i_maxMips)
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
	f32 *data = stbi_loadf(i_inputTexPath, &x, &y, &n, 0);

	CLOVER_DEBUG(
			"Original image loaded:\n"
			"  - Resolution: %d x %d\n"
			"  - Color Channels: %d",
			x, y, n);

	s32 faceSize = y;
	s32 stripWidth = x;
	s32 stripHeight = y;

	FILE* fp = fopen(i_outputTexPath, "wb");

	cymbi::CBTexture2DHeader header;
	memcpy(header.magicCharacters, "CBFM", 4);
	header.colorRange = cymbi::ColorRange::HDR;
	header.colorSpace = cymbi::ColorSpace::Linear;
	header.colorChannel = cymbi::ColorChannel::RGB;
	header.encodedGamma = 1.0f;

	// mips count?
	s32 mipsCount = (s32)log2(faceSize) + 1;
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

	for (s32 i = 0; i < 6; i++) {
		CLOVER_DEBUG("Processing face %d:", i);
		f32* trimData = g_PersistanceAllocator.allocate_array<f32>(faceSize * faceSize * n);
		TrimImage(data, trimData, i * faceSize, 0, faceSize, faceSize, stripWidth, stripHeight, n);
		
		for (s32 m = 1 + mipOffset; m <= mipsCount + mipOffset; m++) {
			s32 mipSize = faceSize >> (m - 1);
			CLOVER_DEBUG("Creating mip #%d at size %dx%d...", m - mipOffset, mipSize, mipSize);
			if (mipSize == faceSize) {
				// stbimage read into memory from top scanline to bottom scanline
				// but OpenGL prefer the order from bottom to top, we have to reorder them
				s32 scanlineSize = mipSize * n;
				//for (s32 j = mipSize - 1; j >= 0; j--) {
				for (s32 j = 0; j < mipSize; j++) {
					fwrite(&trimData[j * scanlineSize], sizeof(f32), scanlineSize, fp);
				}
			} else {
				f32* rzData = g_PersistanceAllocator.allocate_array<f32>(mipSize * mipSize * n);
				stbir_resize_float(trimData, faceSize, faceSize, 0,
						rzData, mipSize, mipSize, 0, n);
				s32 scanlineSize = mipSize * n;
				//for (s32 j = mipSize - 1; j >= 0; j--) {
				for (s32 j = 0; j < mipSize; j++) {
					fwrite(&rzData[j * scanlineSize], sizeof(f32), scanlineSize, fp);
				}
				g_PersistanceAllocator.free(rzData);
			}
		}
		g_PersistanceAllocator.free(trimData);
	}

	fclose(fp);
	CLOVER_INFO("All done. Enjoy :D");
}

}
