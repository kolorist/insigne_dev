#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <floral/stdaliases.h>
#include <floral/math/utils.h>
#include <floral/gpds/vec.h>
#include <floral/io/filesystem.h>

#include <clover/logger.h>
#include <clover/sink_topic.h>
#include <clover/vs_output_sink.h>
#include <clover/console_output_sink.h>

#include "Memory/MemorySystem.h"

#include "CBTexture.h"

#include "stb_image.h"
#include "stb_image_resize.h"
#include "stb_dxt.h"

#include "etc2comp/Etc/Etc.h"

namespace texbaker
{
// -------------------------------------------------------------------

void ConvertToLinear(p8 i_data, p8 o_data, const s32 i_width, const s32 i_height, const s32 i_numChannels, const f32 i_inputGamma)
{
	const f32 invGamma = 1.0f / i_inputGamma;
	for (s32 y = 0; y < i_height; y++)
	{
		for (s32 x = 0; x < i_width; x++)
		{
			for (s32 c = 0; c < i_numChannels; c++)
			{
				u8 comp = i_data[(y * i_width + x) * i_numChannels + c];
				comp = floral::clamp(s32(powf((f32)comp / 255.0f, invGamma) * 255.0f), 0, 255);
				o_data[(y * i_width + x) * i_numChannels + c] = comp;
			}
		}
	}
}

p8 CompressETC2(p8 i_input, const s32 i_width, const s32 i_height, const s32 i_numChannels, size* o_compressedSize)
{
	Etc::Image::Format dstFormat;
	u32 expectedSize = 0;
	if (i_numChannels == 3)
	{
		expectedSize = ceil((f32)i_width / 4.0f) * ceil((f32)i_height / 4.0f) * 8;
		dstFormat = Etc::Image::Format::RGB8;
	}
	else if (i_numChannels == 4)
	{
		expectedSize = ceil((f32)i_width / 4.0f) * ceil((f32)i_height / 4.0f) * 16;
		dstFormat = Etc::Image::Format::RGBA8;
	}
	else
	{
		FLORAL_ASSERT(false);
	}

	f32 effort = 90.0f;
	Etc::ErrorMetric errorMetric = Etc::ErrorMetric::RGBX;

	size pixelsCount = i_width * i_height;
	// etc2comp always expect alpha channel
	f32* floatImgData = (f32*)g_PersistanceAllocator.allocate(pixelsCount * 4 * sizeof(f32));
	for (size p = 0; p < pixelsCount; p++)
	{
		floatImgData[p * 4 + 3] = 1.0f; // fill default alpha = 1.0f
		for (s32 comp = 0; comp < i_numChannels; comp++)
		{
			floatImgData[p * 4 + comp] = (f32)i_input[p * i_numChannels + comp] / 255.0f;
		}
	}

	p8 dstImage = nullptr;
	u32 encodedBitBytes = 0;
	u32 extendedWidth = 0;
	u32 extendedHeight = 0;
	s32 encodingTime = 0;
	Etc::Encode(floatImgData, i_width, i_height, dstFormat, errorMetric, effort, 4, 4, &dstImage,
			&encodedBitBytes, &extendedWidth, &extendedHeight, &encodingTime, true);
	FLORAL_ASSERT(encodedBitBytes == expectedSize);
	*o_compressedSize = encodedBitBytes;
	g_PersistanceAllocator.free(floatImgData);
	return dstImage;
}

p8 CompressDXT(p8 i_input, const s32 i_width, const s32 i_height, const s32 i_numChannels, size* o_compressedSize)
{
	// 1 block = 4x4 pixels (with uncompressed size of 64 bytes for rgba textures)
	size bytesPerBlock = 0;
	size compressedSize = 0;
	const size pixelsCount = i_width * i_height;

	if (i_numChannels == 4)
	{
		// dxt5
		bytesPerBlock = 16;
		compressedSize = floral::max(pixelsCount, 16ull);
	}
	else
	{
		// dxt1 (only 1 bit for alpha)
		bytesPerBlock = 8;
		compressedSize = floral::max(pixelsCount / 2, 8ull);
	}

	p8 output = (p8)g_PersistanceAllocator.allocate(compressedSize);
	p8 targetBlock = output;

	for (s32 y = 0; y < i_height; y += 4)
	{
		for (s32 x = 0; x < i_width; x += 4)
		{
			u8 rawRGBA[4 * 4 * 4];
			memset(rawRGBA, 0, sizeof(rawRGBA));
			p8 targetPixel = rawRGBA;
			for (s32 py = 0; py < 4; py++)
			{
				for (s32 px = 0; px < 4; px++)
				{
					size ix = x + px;
					size iy = y + py;

					if (ix < i_width && iy < i_height)
					{
						p8 sourcePixel = &i_input[(iy * i_width + ix) * i_numChannels];
						// initialize the alpha channel for this pixel
						targetPixel[3] = 255;
						for (s32 i = 0; i < i_numChannels; i++)
						{
							targetPixel[i] = sourcePixel[i];
						}
						targetPixel += 4;
					}
					else
					{
						targetPixel += 4;
					}
				}
			}

			if (i_numChannels == 4)
			{
				stb_compress_dxt_block(targetBlock, rawRGBA, 1, STB_DXT_HIGHQUAL);
			}
			else
			{
				stb_compress_dxt_block(targetBlock, rawRGBA, 0, STB_DXT_HIGHQUAL);
			}

			targetBlock += bytesPerBlock;
		}
	}

	*(o_compressedSize) = compressedSize;
	return output;
}

void ConvertTexture2DLDR(floral::filesystem<FreelistArena>* i_fs, const_cstr i_inputFilename, const_cstr i_outputFilename,
		const f32 i_inputGamma,	const bool i_generateMipmaps, const cbtex::Compression i_compressionMethod)
{
	g_TemporalArena.free_all();
	FLORAL_ASSERT(i_inputGamma <= 1.0f); // noone encode image to save in HDD with gamma > 1.0f

	floral::relative_path inputPath = floral::build_relative_path(i_inputFilename);
	floral::relative_path outputPath = floral::build_relative_path(i_outputFilename);

	floral::file_info inputFile = floral::open_file_read(i_fs, inputPath);
	voidptr inputFileData = g_TemporalArena.allocate(inputFile.file_size);
	floral::read_all_file(inputFile, inputFileData);
	floral::close_file(inputFile);

	s32 x, y, n;
	p8 data = stbi_load_from_memory((p8)inputFileData, inputFile.file_size, &x, &y, &n, 0);
	FLORAL_ASSERT(x == y);

	floral::file_info outputFile = floral::open_file_write(i_fs, outputPath);
	floral::output_file_stream outputStream;
	floral::map_output_file(outputFile, &outputStream);

	s32 mipsCount = (s32)log2(x) + 1;

	cbtex::TextureHeader header;
	header.textureType = cbtex::Type::Texture2D;
	header.colorRange = cbtex::ColorRange::LDR;
	header.mipsCount = mipsCount;
	header.resolution = x;
	header.compression = i_compressionMethod;

	switch (header.compression)
	{
	case cbtex::Compression::NoCompress:
	{
		header.encodedGamma = i_inputGamma;
		if (header.encodedGamma == 1.0f)
		{
			header.colorSpace = cbtex::ColorSpace::Linear;
		}
		else
		{
			header.colorSpace = cbtex::ColorSpace::GammaCorrected;
		}

		switch (n)
		{
			case 1:
				header.colorChannel = cbtex::ColorChannel::R;
				break;
			case 2:
				header.colorChannel = cbtex::ColorChannel::RG;
				break;
			case 3:
				header.colorChannel = cbtex::ColorChannel::RGB;
				break;
			case 4:
				header.colorChannel = cbtex::ColorChannel::RGBA;
				break;
			default:
				FLORAL_ASSERT(false);
				break;
		}

		break;
	}

	case cbtex::Compression::DXT:
	case cbtex::Compression::ETC:
	{
		// when in DXT and ETC, we will only use Linear color space, R, RG texture is treated as RGB texture
		// shader must be reponsible to sample the texture correctly
		header.colorSpace = cbtex::ColorSpace::Linear;
		header.encodedGamma = 1.0f;
		switch (n)
		{
			case 1:
			case 2:
			case 3:
				header.colorChannel = cbtex::ColorChannel::RGB;
				break;
			case 4:
				header.colorChannel = cbtex::ColorChannel::RGBA;
				break;
			default:
				FLORAL_ASSERT(false);
				break;
		}
		break;
	}
	default:
		FLORAL_ASSERT(false);
		break;
	}

	outputStream.write(header);

	for (s32 i = 1; i <= mipsCount; i++)
	{
		s32 nx = x >> (i - 1);
		s32 ny = y >> (i - 1);
		CLOVER_DEBUG("Creating mip #%d at size %dx%d...", i, nx, ny);
		if (i == 1)
		{
			p8 inpData = data;
			if (i_inputGamma < 1.0f)
			{
				inpData = (p8)g_PersistanceAllocator.allocate(nx * ny * n);
				ConvertToLinear(data, inpData, nx, ny, n, i_inputGamma);
			}

			if (i_compressionMethod == cbtex::Compression::DXT)
			{
				size compressedSize = 0;
				p8 compressedData = CompressDXT(inpData, nx, ny, n, &compressedSize);
				outputStream.write_bytes(compressedData, compressedSize);
				g_PersistanceAllocator.free(compressedData);
			}
			else if (i_compressionMethod == cbtex::Compression::ETC)
			{
				size compressedSize = 0;
				p8 compressedData = CompressETC2(inpData, nx, ny, n, &compressedSize);
				outputStream.write_bytes(compressedData, compressedSize);
				delete[] compressedData;
			}
			else
			{
				outputStream.write_bytes(data, nx * ny * n);
			}

			if (i_inputGamma < 1.0f)
			{
				g_PersistanceAllocator.free(inpData);
			}
		}
		else
		{
			p8 rzdata = g_PersistanceAllocator.allocate_array<u8>(nx * ny * n);
			if (i_inputGamma == 1.0f)
			{
				stbir_resize_uint8(data, x, y, 0, rzdata, nx, ny, 0, n);
			}
			else
			{
				if (header.colorChannel == cbtex::ColorChannel::RGBA)
				{
					stbir_resize_uint8_srgb(data, x, y, 0, rzdata, nx, ny, 0, n, 0, 0);
				}
				else
				{
					stbir_resize_uint8_srgb(data, x, y, 0, rzdata, nx, ny, 0, n, STBIR_ALPHA_CHANNEL_NONE, 0);
				}
			}

			if (i_compressionMethod == cbtex::Compression::DXT)
			{
				p8 inpData = rzdata;
				if (i_inputGamma < 1.0f)
				{
					inpData = (p8)g_PersistanceAllocator.allocate(nx * ny * n);
					ConvertToLinear(rzdata, inpData, nx, ny, n, i_inputGamma);
				}
				size compressedSize = 0;
				p8 compressedData = CompressDXT(inpData, nx, ny, n, &compressedSize);
				outputStream.write_bytes(compressedData, compressedSize);
				g_PersistanceAllocator.free(compressedData);
				if (i_inputGamma < 1.0f)
				{
					g_PersistanceAllocator.free(inpData);
				}
			}
			else if (i_compressionMethod == cbtex::Compression::ETC)
			{
				p8 inpData = rzdata;
				if (i_inputGamma < 1.0f)
				{
					inpData = (p8)g_PersistanceAllocator.allocate(nx * ny * n);
					ConvertToLinear(rzdata, inpData, nx, ny, n, i_inputGamma);
				}
				size compressedSize = 0;
				p8 compressedData = CompressETC2(inpData, nx, ny, n, &compressedSize);
				outputStream.write_bytes(compressedData, compressedSize);
				delete[] compressedData;
				if (i_inputGamma < 1.0f)
				{
					g_PersistanceAllocator.free(inpData);
				}
			}
			else
			{
				outputStream.write_bytes(rzdata, nx * ny * n);
			}
			g_PersistanceAllocator.free(rzdata);
		}
	}

	floral::close_file(outputFile);
}

void ConvertToHalfFloat(f32* i_data, f16* o_data, const s32 i_width, const s32 i_height, const s32 i_numChannels)
{
	for (s32 y = 0; y < i_height; y++)
	{
		for (s32 x = 0; x < i_width; x++)
		{
			for (s32 c = 0; c < i_numChannels; c++)
			{
				f32 comp32 = i_data[(y * i_width + x) * i_numChannels + c];
				f16 comp16 = floral::float_to_half_full(comp32);
				o_data[(y * i_width + x) * i_numChannels + c] = comp16;
			}
		}
	}
}

floral::vec4f RGBMEncode(const floral::vec3f& i_hdrColor)
{
	floral::vec4f rgbm;
	floral::vec3f color = i_hdrColor / 6.0f;
	rgbm.w = floral::clamp(floral::max(floral::max(color.x, color.y), floral::max(color.z, 0.000001f)), 0.0f, 1.0f);
	rgbm.w = ceil(rgbm.w * 255.0f) / 255.0f;
	rgbm.x = color.x / rgbm.w;
	rgbm.y = color.y / rgbm.w;
	rgbm.z = color.z / rgbm.w;
	return rgbm;
}

floral::vec3f RGBMDecode(const floral::vec4f& i_rgbmColor)
{
	floral::vec3f color(i_rgbmColor.x, i_rgbmColor.y, i_rgbmColor.z);
	return 6.0f * color * i_rgbmColor.w;
}

void ConvertToRGBM(f32* i_inpData, p8 o_rgbmData, const s32 i_width, const s32 i_height, const f32 i_gamma)
{
	f32 invGamma = 1.0f / i_gamma;
	for (s32 y = 0; y < i_height; y++)
	{
		for (s32 x = 0; x < i_width; x++)
		{
			s32 pixelIdx = y * i_width + x;
			floral::vec3f hdrColor(i_inpData[pixelIdx * 3], i_inpData[pixelIdx * 3 + 1], i_inpData[pixelIdx * 3 + 2]);

			if (hdrColor.x < 0.0f || hdrColor.y < 0.0f || hdrColor.z < 0.0f)
			{
				CLOVER_WARNING("clamping pixel at (x, y) = (%d, %d) because it has negative component: (%f, %f, %f)", x, y, hdrColor.x, hdrColor.y, hdrColor.z);
				hdrColor.x = floral::max(hdrColor.x, 0.0f);
				hdrColor.y = floral::max(hdrColor.y, 0.0f);
				hdrColor.z = floral::max(hdrColor.z, 0.0f);
			}

			floral::vec3f gammaCorrectedHDRColor;
			gammaCorrectedHDRColor.x = powf(hdrColor.x, i_gamma);
			gammaCorrectedHDRColor.y = powf(hdrColor.y, i_gamma);
			gammaCorrectedHDRColor.z = powf(hdrColor.z, i_gamma);
			floral::vec4f rgbmFloatColor = RGBMEncode(gammaCorrectedHDRColor);
			FLORAL_ASSERT(rgbmFloatColor.x <= 1.0f);
			FLORAL_ASSERT(rgbmFloatColor.y <= 1.0f);
			FLORAL_ASSERT(rgbmFloatColor.z <= 1.0f);
			FLORAL_ASSERT(rgbmFloatColor.w <= 1.0f);
			o_rgbmData[pixelIdx * 4] = ceil(rgbmFloatColor.x * 255.0f);
			o_rgbmData[pixelIdx * 4 + 1] = ceil(rgbmFloatColor.y * 255.0f);
			o_rgbmData[pixelIdx * 4 + 2] = ceil(rgbmFloatColor.z * 255.0f);
			o_rgbmData[pixelIdx * 4 + 3] = ceil(rgbmFloatColor.w * 255.0f);
#if 0
			floral::vec3f constructedHDRGamma = RGBMDecode(rgbmFloatColor);
			constructedHDRGamma.x = powf(constructedHDRGamma.x, invGamma);
			constructedHDRGamma.y = powf(constructedHDRGamma.y, invGamma);
			constructedHDRGamma.z = powf(constructedHDRGamma.z, invGamma);
#endif
		}
	}
}

void ConvertTexture2DHDR(floral::filesystem<FreelistArena>* i_fs, const_cstr i_inputFilename, const_cstr i_outputFilename, const bool i_generateMipmaps, const cbtex::Compression i_compressionMethod, const f32 i_gamma)
{
	g_TemporalArena.free_all();

	floral::relative_path inputPath = floral::build_relative_path(i_inputFilename);
	floral::relative_path outputPath = floral::build_relative_path(i_outputFilename);

	floral::file_info inputFile = floral::open_file_read(i_fs, inputPath);
	voidptr inputFileData = g_TemporalArena.allocate(inputFile.file_size);
	floral::read_all_file(inputFile, inputFileData);
	floral::close_file(inputFile);

	s32 x, y, n;
	f32* data = stbi_loadf_from_memory((p8)inputFileData, inputFile.file_size, &x, &y, &n, 0);
	FLORAL_ASSERT(n == 3);
	FLORAL_ASSERT(x == y);

	floral::file_info outputFile = floral::open_file_write(i_fs, outputPath);
	floral::output_file_stream outputStream;
	floral::map_output_file(outputFile, &outputStream);

	f32 maxRange[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	s32 pixelsCount = x * y;
	for (s32 i = 0; i < pixelsCount; i++)
	{
		for (s32 c = 0; c < n; c++)
		{
			FLORAL_ASSERT(data[i * n + c] >= 0.0f);
			if (data[i * n + c] > maxRange[c])
			{
				maxRange[c] = data[i * n + c];
			}
		}
	}
	FLORAL_ASSERT(maxRange[0] <= 36.0f); // why? because our rgbm range is 0..6 including 2.0 gamma, and 6^2 = 36
	FLORAL_ASSERT(maxRange[1] <= 36.0f);
	FLORAL_ASSERT(maxRange[2] <= 36.0f);

	CLOVER_INFO("%d channels:", n);
	CLOVER_INFO("max (r, g, b, a): (%4.3f, %4.3f, %4.3f, %4.3f)", maxRange[0], maxRange[1], maxRange[2], maxRange[3]);

	cbtex::TextureHeader header;
	header.textureType = cbtex::Type::Texture2D;
	header.colorRange = cbtex::ColorRange::HDR;
	header.encodedGamma = i_gamma;
	header.colorSpace = cbtex::ColorSpace::Linear;

	switch (n)
	{
		case 3:
			header.colorChannel = cbtex::ColorChannel::RGB;
			break;
		case 4:
			header.colorChannel = cbtex::ColorChannel::RGBA;
			break;
		default:
			FLORAL_ASSERT(false);
			break;
	}

	s32 mipsCount = (s32)log2(x) + 1;
	header.mipsCount = mipsCount;
	header.resolution = x;
	header.compression = i_compressionMethod;

	outputStream.write(header);

	for (s32 i = 1; i <= mipsCount; i++)
	{
		s32 nx = x >> (i - 1);
		s32 ny = y >> (i - 1);
		CLOVER_DEBUG("Creating mip #%d at size %dx%d...", i, nx, ny);
		if (i == 1)
		{
			if (header.compression == cbtex::Compression::NoCompress)
			{
				FLORAL_ASSERT(header.encodedGamma == 1.0f);
				f16* halfFloatData = g_PersistanceAllocator.allocate_array<f16>(nx * ny * n);
				ConvertToHalfFloat(data, halfFloatData, nx, ny, n);
				outputStream.write_bytes(halfFloatData, nx * ny * n * sizeof(f16));
				g_PersistanceAllocator.free(halfFloatData);
			}
			else if (header.compression == cbtex::Compression::DXT)
			{
				p8 rgbaData = g_PersistanceAllocator.allocate_array<u8>(nx * ny * 4);
				ConvertToRGBM(data, rgbaData, nx, ny, 0.5f);
				size compressedSize = 0;
				p8 compressedData = CompressDXT(rgbaData, nx, ny, 4, &compressedSize);
				outputStream.write_bytes(compressedData, compressedSize);
				g_PersistanceAllocator.free(compressedData);
				g_PersistanceAllocator.free(rgbaData);
			}
			else
			{
				FLORAL_ASSERT(false);
			}
		}
		else
		{
			if (header.compression == cbtex::Compression::NoCompress)
			{
				FLORAL_ASSERT(header.encodedGamma == 1.0f);
				f32* rzdata = g_PersistanceAllocator.allocate_array<f32>(nx * ny * n);
				s32 result = stbir_resize_float(data, x, y, 0, rzdata, nx, ny, 0, n);
				FLORAL_ASSERT(result);

				f16* halfFloatData = g_PersistanceAllocator.allocate_array<f16>(nx * ny * n);
				ConvertToHalfFloat(rzdata, halfFloatData, nx, ny, n);
				outputStream.write_bytes(halfFloatData, nx * ny * n * sizeof(f16));
				g_PersistanceAllocator.free(halfFloatData);
				g_PersistanceAllocator.free(rzdata);
			}
			else if (header.compression == cbtex::Compression::DXT)
			{
				f32* rzdata = g_PersistanceAllocator.allocate_array<f32>(nx * ny * n);
				s32 result = stbir_resize_float(data, x, y, 0, rzdata, nx, ny, 0, n);
				FLORAL_ASSERT(result);

				p8 rgbaData = g_PersistanceAllocator.allocate_array<u8>(nx * ny * 4);
				ConvertToRGBM(rzdata, rgbaData, nx, ny, 0.5f);
				size compressedSize = 0;
				p8 compressedData = CompressDXT(rgbaData, nx, ny, 4, &compressedSize);
				outputStream.write_bytes(compressedData, compressedSize);
				g_PersistanceAllocator.free(compressedData);
				g_PersistanceAllocator.free(rgbaData);

				g_PersistanceAllocator.free(rzdata);
			}
			else
			{
				FLORAL_ASSERT(false);
			}
		}
	}

	floral::close_file(outputFile);
}

void TrimImage(f32* i_imgData, f32* o_outData, const s32 i_x, const s32 i_y, const s32 i_width, const s32 i_height,
		const s32 i_imgWidth, const s32 i_imgHeight, const s32 i_channelCount)
{
	s32 outScanline = 0;
	for (s32 i = i_y; i < (i_y + i_height); i++)
	{
		memcpy(&o_outData[outScanline * i_width * i_channelCount],
				&i_imgData[(i * i_imgWidth + i_x) * i_channelCount],
				i_height * i_channelCount * sizeof(f32));
		outScanline++;
	}
}

floral::vec3f HDRToneMap(const f32* i_hdrRGB)
{
	floral::vec3f hdrColor(i_hdrRGB[0], i_hdrRGB[1], i_hdrRGB[2]);
	f32 maxLuma = floral::max(hdrColor.x, floral::max(hdrColor.y, hdrColor.z));
	hdrColor.x = hdrColor.x / (1.0f + maxLuma / 35.0f);
	hdrColor.y = hdrColor.y / (1.0f + maxLuma / 35.0f);
	hdrColor.z = hdrColor.z / (1.0f + maxLuma / 35.0f);
	return hdrColor;
}

void ConvertTextureCubeMapHDR(floral::filesystem<FreelistArena>* i_fs, const_cstr i_inputFilename, const_cstr i_outputFilename,
		const bool i_generateMipmaps, const cbtex::Compression i_compression, const f32 i_gamma)
{
	g_TemporalArena.free_all();

	floral::relative_path inputPath = floral::build_relative_path(i_inputFilename);
	floral::relative_path outputPath = floral::build_relative_path(i_outputFilename);

	floral::file_info inputFile = floral::open_file_read(i_fs, inputPath);
	voidptr inputFileData = g_TemporalArena.allocate(inputFile.file_size);
	floral::read_all_file(inputFile, inputFileData);
	floral::close_file(inputFile);

	s32 x, y, n;
	f32* data = stbi_loadf_from_memory((p8)inputFileData, inputFile.file_size, &x, &y, &n, 0);
	FLORAL_ASSERT(n == 3);

	floral::file_info outputFile = floral::open_file_write(i_fs, outputPath);
	floral::output_file_stream outputStream;
	floral::map_output_file(outputFile, &outputStream);

	f32 maxRange[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	s32 pixelsCount = x * y;
	for (s32 i = 0; i < pixelsCount; i++)
	{
		for (s32 c = 0; c < n; c++)
		{
			FLORAL_ASSERT(data[i * n + c] >= 0.0f);
			if (data[i * n + c] > maxRange[c])
			{
				maxRange[c] = data[i * n + c];
			}
		}
	}

	if (maxRange[0] > 36.0f || maxRange[1] > 36.0f || maxRange[2] > 36.0f)
	{
		CLOVER_WARNING("tonemapping the hdr input because the hdr range is outside 36.0f");
		for (s32 i = 0; i < pixelsCount; i++)
		{
			floral::vec3f toneMapHDRColor = HDRToneMap(&data[i * 3]);
			data[i * 3] = toneMapHDRColor.x;
			data[i * 3 + 1] = toneMapHDRColor.y;
			data[i * 3 + 2] = toneMapHDRColor.z;
		}
	}

	CLOVER_INFO("%d channels:", n);
	CLOVER_INFO("max (r, g, b, a): (%4.3f, %4.3f, %4.3f, %4.3f)", maxRange[0], maxRange[1], maxRange[2], maxRange[3]);

	s32 faceSize = y;

	cbtex::TextureHeader header;
	header.textureType = cbtex::Type::CubeMap;
	header.colorRange = cbtex::ColorRange::HDR;
	header.encodedGamma = i_gamma;
	header.colorSpace = cbtex::ColorSpace::Linear;
	header.colorChannel = cbtex::ColorChannel::RGB;

	s32 mipsCount = (s32)log2(faceSize) + 1;
	header.mipsCount = mipsCount;
	header.resolution = faceSize;
	header.compression = i_compression;

	outputStream.write(header);

	for (s32 i = 0; i < 6; i++)
	{
		CLOVER_DEBUG("Processing face %d:", i);
		f32* trimData = g_PersistanceAllocator.allocate_array<f32>(faceSize * faceSize * n);
		TrimImage(data, trimData, i * faceSize, 0, faceSize, faceSize, x, y, n);

		for (s32 m = 1; m <= mipsCount; m++)
		{
			s32 mipSize = faceSize >> (m - 1);
			CLOVER_DEBUG("Creating mip #%d at size %dx%d...", m, mipSize, mipSize);
			if (mipSize == faceSize)
			{
				if (header.compression == cbtex::Compression::NoCompress)
				{
					f16* halfFloatData = g_PersistanceAllocator.allocate_array<f16>(mipSize * mipSize * n);
					ConvertToHalfFloat(trimData, halfFloatData, mipSize, mipSize, n);
					outputStream.write_bytes(halfFloatData, mipSize * mipSize * n * sizeof(f16));
					g_PersistanceAllocator.free(halfFloatData);
				}
				else if (header.compression == cbtex::Compression::DXT)
				{
					p8 rgbaData = g_PersistanceAllocator.allocate_array<u8>(mipSize * mipSize * 4);
					ConvertToRGBM(trimData, rgbaData, mipSize, mipSize, 0.5f);
					size compressedSize = 0;
					p8 compressedData = CompressDXT(rgbaData, mipSize, mipSize, 4, &compressedSize);
					outputStream.write_bytes(compressedData, compressedSize);
					g_PersistanceAllocator.free(compressedData);
					g_PersistanceAllocator.free(rgbaData);
				}
				else
				{
					FLORAL_ASSERT(false);
				}
			}
			else
			{
				if (header.compression == cbtex::Compression::NoCompress)
				{
					f32* rzData = g_PersistanceAllocator.allocate_array<f32>(mipSize * mipSize * n);
					stbir_resize_float(trimData, faceSize, faceSize, 0, rzData, mipSize, mipSize, 0, n);
					f16* halfFloatData = g_PersistanceAllocator.allocate_array<f16>(mipSize * mipSize * n);
					ConvertToHalfFloat(rzData, halfFloatData, mipSize, mipSize, n);
					outputStream.write_bytes(halfFloatData, mipSize * mipSize * n * sizeof(f16));
					g_PersistanceAllocator.free(halfFloatData);
					g_PersistanceAllocator.free(rzData);
				}
				else if (header.compression == cbtex::Compression::DXT)
				{
					f32* rzData = g_PersistanceAllocator.allocate_array<f32>(mipSize * mipSize * n);
					s32 result = stbir_resize_float(trimData, faceSize, faceSize, 0, rzData, mipSize, mipSize, 0, n);
					FLORAL_ASSERT(result);

					p8 rgbaData = g_PersistanceAllocator.allocate_array<u8>(mipSize * mipSize * 4);
					ConvertToRGBM(rzData, rgbaData, mipSize, mipSize, 0.5f);
					size compressedSize = 0;
					p8 compressedData = CompressDXT(rgbaData, mipSize, mipSize, 4, &compressedSize);
					outputStream.write_bytes(compressedData, compressedSize);
					g_PersistanceAllocator.free(compressedData);
					g_PersistanceAllocator.free(rgbaData);

					g_PersistanceAllocator.free(rzData);
				}
				else
				{
					FLORAL_ASSERT(false);
				}
			}
		}

		g_PersistanceAllocator.free(trimData);
	}

	floral::close_file(outputFile);
}

// -------------------------------------------------------------------
}

int main(int argc, char** argv)
{
	using namespace texbaker;

	helich::init_memory_system();

	clover::Initialize("main_thread", clover::LogLevel::Verbose);
	clover::InitializeVSOutput("vs", clover::LogLevel::Verbose);
	clover::InitializeConsoleOutput("console", clover::LogLevel::Verbose);

	floral::absolute_path workingDir = floral::get_application_directory();
	floral::filesystem<FreelistArena>* fileSystem = floral::create_filesystem(workingDir, &g_FilesystemArena);

	CLOVER_INFO("Texture Baker");
	CLOVER_INFO("Build: 0.5.0a");

	if (argc == 1)
	{
		CLOVER_INFO(
				"texturebaker.exe --input input.tga --dim 2d --color-range ldr --input-gamma 0.454545 --generate-mipmaps on --compression dxt --output output.cbtex");
		return 0;
	}

	const_cstr inputFilePath = nullptr;
	const_cstr outputFilePath = nullptr;
	f32 inputGamma = 1.0f;
	bool generateMipmaps = true;
	cbtex::ColorRange colorRange = cbtex::ColorRange::Undefined;
	cbtex::Type texType = cbtex::Type::Undefined;
	cbtex::Compression compression = cbtex::Compression::NoCompress;

	for (s32 i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "--input") == 0)
		{
			i++;
			inputFilePath = argv[i];
		}

		if (strcmp(argv[i], "--output") == 0)
		{
			i++;
			outputFilePath = argv[i];
		}

		if (strcmp(argv[i], "--dim") == 0)
		{
			i++;
			if (strcmp(argv[i], "2d") == 0)
			{
				texType = cbtex::Type::Texture2D;
			}
			else if (strcmp(argv[i], "cubemap") == 0)
			{
				texType = cbtex::Type::CubeMap;
			}
		}

		if (strcmp(argv[i], "--color-range") == 0)
		{
			i++;
			if (strcmp(argv[i], "ldr") == 0)
			{
				colorRange = cbtex::ColorRange::LDR;
			}
			else if (strcmp(argv[i], "hdr") == 0)
			{
				colorRange = cbtex::ColorRange::HDR;
			}
		}

		if (strcmp(argv[i], "--input-gamma") == 0)
		{
			i++;
			inputGamma = atof(argv[i]);
		}

		if (strcmp(argv[i], "--generate-mipmaps") == 0)
		{
			i++;
			if (strcmp(argv[i], "on") == 0)
			{
				generateMipmaps = true;
			}
			else if (strcmp(argv[i], "off") == 0)
			{
				generateMipmaps = false;
			}
		}

		if (strcmp(argv[i], "--compression") == 0)
		{
			i++;
			if (strcmp(argv[i], "dxt") == 0)
			{
				compression = cbtex::Compression::DXT;
			}
			else if (strcmp(argv[i], "etc") == 0)
			{
				compression = cbtex::Compression::ETC;
			}
			else if (strcmp(argv[i], "no-compress") == 0)
			{
				compression = cbtex::Compression::NoCompress;
			}
		}
	}

	if (inputFilePath == nullptr || outputFilePath == nullptr)
	{
		return 0;
	}

	if (texType == cbtex::Type::Texture2D)
	{
		if (colorRange == cbtex::ColorRange::LDR)
		{
			texbaker::ConvertTexture2DLDR(fileSystem, inputFilePath, outputFilePath, inputGamma, generateMipmaps, compression);
		}
		else if (colorRange == cbtex::ColorRange::HDR)
		{
			texbaker::ConvertTexture2DHDR(fileSystem, inputFilePath, outputFilePath, generateMipmaps, compression, inputGamma);
		}
	}
	else if (texType == cbtex::Type::CubeMap)
	{
		texbaker::ConvertTextureCubeMapHDR(fileSystem, inputFilePath, outputFilePath, generateMipmaps, compression, inputGamma);
	}

	floral::destroy_filesystem(&fileSystem);
	return 0;
}
