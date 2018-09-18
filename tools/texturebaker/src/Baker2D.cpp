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

}
