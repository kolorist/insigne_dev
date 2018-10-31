#include <stdio.h>
#include <math.h>
#include <floral.h>
#include <clover.h>

#include "Memory/MemorySystem.h"

#include "stb_image.h"
#include "stb_image_resize.h"

#include "Baker2D.h"
#include "BakerCube.h"

namespace texbaker {

struct HDRPixel {
	f32 r, g, b;
};

void ExtractPFMHeader(p8* i_buffer, u32& o_faceW, u32& o_faceH, f32& o_endianess)
{
	c8 header[512];
	c8 magicChars[3];
	u32 i = 0;
	u32 lfCount = 0;
	while (lfCount < 3) {
		if (**i_buffer == 0x0A) lfCount++;
		header[i] = **i_buffer;
		(*i_buffer)++;
		i++;
	}
	header[i] = 0;
	sscanf(header, "%s\n%d %d\n%f", magicChars, &o_faceW, &o_faceH, &o_endianess);
}

void AdjustHDRPixelBuffer(HDRPixel* i_buffer, const u32 i_fullW, const u32 i_fullH)
{
	u32 faceW = i_fullW / 3;
	u32 faceH = i_fullH / 4;

	// flip the image as PFM format list pixels from bottom to top
	for (u32 i = 0; i < i_fullH / 2; i++) {
		for (u32 j = 0; j < i_fullW; j++) {
			HDRPixel tmp = i_buffer[i * i_fullW + j];
			i_buffer[i * i_fullW + j] = i_buffer[(i_fullH - i - 1) * i_fullW + j];
			i_buffer[(i_fullH - i - 1) * i_fullW + j] = tmp;
		}
	}

	// correct the last image
	u32 offsetY = 3 * faceW;
	u32 offsetX = faceH;
	for (u32 i = 0; i < faceH / 2; i++) {
		for (u32 j = 0; j < faceW; j++) {
			u32 srcPos = (i + offsetY) * i_fullW + offsetX + j;
			u32 dstPos = ((faceH - i - 1) + offsetY) * i_fullW + offsetX + (faceW - j - 1);
			HDRPixel tmp = i_buffer[srcPos];
			i_buffer[srcPos] = i_buffer[dstPos];
			i_buffer[dstPos] = tmp;
		}
	}
}

void AppendHDRPixelBufferToCBFormat(HDRPixel* i_buffer, const u32 i_fullW, const u32 i_fullH, FILE* i_outputFile)
{
	u32 faceW = i_fullW / 3;
	u32 faceH = i_fullH / 4;

	u32 faceOffset[] = {
			1, 2,
	1, 0,	0, 1,	2, 1,
			1, 1,
			3, 1 };

	for (u32 fidx = 0; fidx < 6; fidx++) {
		u32 facePosX = i_fullW * faceOffset[fidx * 2] * faceH;
		u32 facePosY = faceW * faceOffset[fidx * 2 + 1];

		for (u32 i = 0; i < faceH; i++) {
			fwrite(&i_buffer[facePosX + i * i_fullW + facePosY], sizeof(HDRPixel), faceW, i_outputFile);
		}
	}
}

void ConvertProbe(const_cstr i_firstMip, const_cstr i_outputPath)
{
	printf("converting probe '%s'\n", i_firstMip);
	c8 firstMipFullPath[1024];
	sprintf(firstMipFullPath, "%s_m%02d.pfm", i_firstMip, 0);

	FILE* fp = fopen(i_outputPath, "wb");
	// magic number
	fwrite("CBFM", 1, 4, fp);

	// HDR or LDR?
	// 1 = HDR
	// 2 = LDR
	s32 colorRange = 1;
	fwrite(&colorRange, sizeof(s32), 1, fp);

	// Color space?
	// 1 = Linear
	// 2 = Gamma-corrected
	s32 colorSpace = 1;
	fwrite(&colorSpace, sizeof(s32), 1, fp);

	// Color channels?
	// 1 = R
	// 2 = RG
	// 3 = RGB
	// 4 = RGBA
	s32 colorChannel = 3;
	fwrite(&colorChannel, sizeof(s32), 1, fp);

	// Encoded-gamma, if any?
	f32 encodeGamma = 1.0f;
	fwrite(&encodeGamma, sizeof(f32), 1, fp);

	s32 mipsCount = 0;

	// get initial information
	{
		floral::file_info firstMipFile = floral::open_file(firstMipFullPath);

		g_TemporalArena.free_all();
		p8 buffer = (p8)g_TemporalArena.allocate(firstMipFile.file_size);
		floral::read_all_file(firstMipFile, (voidptr)buffer);

		f32 endianess = 0.0f;
		u32 resX = 0, resY = 0;
		ExtractPFMHeader(&buffer, resX, resY, endianess);

		// mips count?
		mipsCount = (s32)log2(resX / 3) + 1;
		fwrite(&mipsCount, sizeof(s32), 1, fp);
		printf("mipsCount = %d\n", mipsCount);

		HDRPixel* hdrBuffer = (HDRPixel*)buffer;
		AdjustHDRPixelBuffer(hdrBuffer, resX, resY);

		// now hdrBuffer is ready
		AppendHDRPixelBufferToCBFormat(hdrBuffer, resX, resY, fp);

		floral::close_file(firstMipFile);
	}

	for (s32 i = 1; i < mipsCount; i++)
	{
		c8 mipFullPath[1024];
		sprintf(mipFullPath, "%s_m%02d.pfm", i_firstMip, i);

		floral::file_info mipFile = floral::open_file(mipFullPath);

		g_TemporalArena.free_all();
		p8 buffer = (p8)g_TemporalArena.allocate(mipFile.file_size);
		floral::read_all_file(mipFile, buffer);

		f32 endianess = 0.0f;
		u32 resX = 0, resY = 0;
		ExtractPFMHeader(&buffer, resX, resY, endianess);

		HDRPixel* hdrBuffer = (HDRPixel*)buffer;
		AdjustHDRPixelBuffer(hdrBuffer, resX, resY);

		// now hdrBuffer is ready, write it back!
		AppendHDRPixelBufferToCBFormat(hdrBuffer, resX, resY, fp);

		floral::close_file(mipFile);
	}

	printf("done\n");
	fclose(fp);
}

void ConvertSkybox(const_cstr i_filePath, const_cstr i_outputPath)
{
	printf("converting skybox '%s'\n", i_filePath);

	FILE* fp = fopen(i_outputPath, "wb");
	// magic number
	fwrite("CBFM", 1, 4, fp);

	// HDR or LDR?
	// 1 = HDR
	// 2 = LDR
	s32 colorRange = 1;
	fwrite(&colorRange, sizeof(s32), 1, fp);

	// Color space?
	// 1 = Linear
	// 2 = Gamma-corrected
	s32 colorSpace = 1;
	fwrite(&colorSpace, sizeof(s32), 1, fp);

	// Color channels?
	// 1 = R
	// 2 = RG
	// 3 = RGB
	// 4 = RGBA
	s32 colorChannel = 3;
	fwrite(&colorChannel, sizeof(s32), 1, fp);

	// Encoded-gamma, if any?
	f32 encodeGamma = 1.0f;
	fwrite(&encodeGamma, sizeof(f32), 1, fp);

	floral::file_info skyboxFile = floral::open_file(i_filePath);

	g_TemporalArena.free_all();
	p8 buffer = (p8)g_TemporalArena.allocate(skyboxFile.file_size);
	floral::read_all_file(skyboxFile, (voidptr)buffer);

	f32 endianess = 0.0f;
	u32 resX = 0, resY = 0;
	ExtractPFMHeader(&buffer, resX, resY, endianess);

	// mips count?
	s32 mipsCount = 0;
	fwrite(&mipsCount, sizeof(s32), 1, fp);

	HDRPixel* hdrBuffer = (HDRPixel*)buffer;
	AdjustHDRPixelBuffer(hdrBuffer, resX, resY);

	// now hdrBuffer is ready
	AppendHDRPixelBufferToCBFormat(hdrBuffer, resX, resY, fp);

	floral::close_file(skyboxFile);
}

}

int main(int argc, char** argv)
{
	using namespace texbaker;

	helich::init_memory_system();

	clover::Initialize();
	clover::InitializeVSOutput("vs", clover::LogLevel::Verbose);
	clover::InitializeConsoleOutput("console", clover::LogLevel::Verbose);

	CLOVER_INFO("Texture Baker");
	CLOVER_INFO("Build: 0.1.0a");

	if (argc == 1) {
		CLOVER_INFO(
				"Texture Baker Syntax:\n"
				"	> 2D Texture:           texturebaker.exe -t input_texture.png -m 10 -o output_texture.cbtex\n"
				"	> ShadingProbe Texture: texturebaker.exe -p input_cubemap_no_ext -f pfm -o output_cubemap.cbprb\n"
				"	> Spherical Harmonics:	texturebaker.exe -sh input_texture.hdr -o output_shdata.cbsh\n"
				"	> CubeMap Texture:      texturebaker.exe -s input_skybox_no_ext -f pfm -o output_skybox.cbskb");
		return 0;
	}

	if (strcmp(argv[1], "-t") == 0) {
		ConvertTexture2D(argv[2], argv[6], atoi(argv[4]));
	} else if (strcmp(argv[1], "-p") == 0) {
		ConvertProbe(argv[2], argv[6]);
	} else if (strcmp(argv[1], "-s") == 0) {
		// hstrip input, order: +x -x +y -y +z -z
		ConvertTextureCubeHStrip(argv[2], argv[6], atoi(argv[4]));
	} else if (strcmp(argv[1], "-sh") == 0) {
		// hstrip input, order: +x -x +y -y +z -z
		ComputeSH(argv[2], argv[4]);
	}

	return 0;
}
