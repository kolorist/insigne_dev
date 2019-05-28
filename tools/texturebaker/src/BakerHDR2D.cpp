#include "BakerHDR2D.h"

#include <clover/Logger.h>

#include "Memory/MemorySystem.h"
#include "CBFormats.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_resize.h"

namespace texbaker
{

void ConvertTexture2DHDR(const_cstr i_inputTexPath, const_cstr i_outputTexPath, const s32 i_maxMips)
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
	f32* data = stbi_loadf(i_inputTexPath, &x, &y, &n, 0);
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
	s32 noMipmapSize = 0;

	bool generateMipmaps = true;

	if (i_maxMips > 0)
	{
		maxMips = i_maxMips;
		mipOffset = 0;
		if (mipsCount > maxMips)
		{
			mipOffset = mipsCount - maxMips;
			mipsCount = maxMips;
		}
	}
	else if (i_maxMips == 0)
	{
		maxMips = mipsCount;
	}
	else if (i_maxMips == -1)
	{
		generateMipmaps = false;
		noMipmapSize = 1 << (s32)log2(x);
	}

	if (generateMipmaps)
	{
		header.resolution = 1 << mipsCount;
		header.mipsCount = mipsCount;
		CLOVER_DEBUG("Settled at mips count = %d", header.mipsCount);
		fwrite((p8)&header, sizeof(cymbi::CBTexture2DHeader), 1, fp);

		for (s32 i = 1 + mipOffset; i <= mipsCount + mipOffset; i++)
		{
			s32 nx = x >> (i - 1);
			s32 ny = y >> (i - 1);
			CLOVER_DEBUG("Creating mip #%d at size %dx%d...", i - mipOffset, nx, ny);
			if (i == 1)
			{
				fwrite(data, sizeof(f32), nx * ny * n, fp);
			}
			else
			{
				f32* rzdata = g_PersistanceAllocator.allocate_array<f32>(nx * ny * n);
				stbir_resize_float(
						data, x, y, 0,
						rzdata, nx, ny, 0,
						n);
				fwrite(rzdata, sizeof(f32), nx * ny * n, fp);
				g_PersistanceAllocator.free(rzdata);
			}
		}
	}
	else
	{
		header.mipsCount = 0;
		header.resolution = noMipmapSize;
		CLOVER_DEBUG("Settled at no mipmap and resolution = %d", header.resolution);
		fwrite((p8)&header, sizeof(cymbi::CBTexture2DHeader), 1, fp);

		s32 nx = noMipmapSize;
		s32 ny = noMipmapSize;
		f32* rzdata = g_PersistanceAllocator.allocate_array<f32>(nx * ny * n);
		stbir_resize_float(
				data, x, y, 0,
				rzdata, nx, ny, 0,
				n);
		fwrite(rzdata, sizeof(f32), nx * ny * n, fp);
		g_PersistanceAllocator.free(rzdata);
	}

	fclose(fp);
	CLOVER_INFO("All done. Enjoy :D");
}

}
