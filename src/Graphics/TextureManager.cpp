#include "TextureManager.h"

#include <insigne/render.h>

namespace stone {
	TextureManager::TextureManager()
	{
		m_MemoryArena = g_PersistanceAllocator.allocate_arena<LinearArena>(SIZE_MB(64));
	}

	TextureManager::~TextureManager()
	{
	}

	insigne::texture_handle_t TextureManager::CreateTexture(const_cstr i_texPath)
	{
		floral::file_info texFile = floral::open_file(i_texPath);
		floral::file_stream dataStream;

		dataStream.buffer = (p8)m_MemoryArena->allocate(texFile.file_size);
		floral::read_all_file(texFile, dataStream);

		c8 magicChars[4];
		dataStream.read_bytes(magicChars, 4);

		s32 colorRange = 0;
		s32 colorSpace = 0;
		s32 colorChannel = 0;
		f32 encodeGamma = 0.0f;
		s32 mipsCount = 0;
		dataStream.read<s32>(&colorRange);
		dataStream.read<s32>(&colorSpace);
		dataStream.read<s32>(&colorChannel);
		dataStream.read<f32>(&encodeGamma);
		dataStream.read<s32>(&mipsCount);

		s32 width = 1 << (mipsCount - 1);
		s32 height = width;
		size dataSize = ((1 << (2 * mipsCount)) - 1) / 3 * colorChannel;

		voidptr texData = nullptr;
		insigne::texture_handle_t texHdl = insigne::create_texture2d(width, height,
				insigne::texture_format_e::rgb,
				insigne::filtering_e::linear_mipmap_linear, insigne::filtering_e::linear,
				dataSize, texData, true);
		dataStream.read_bytes((p8)texData, dataSize);

		floral::close_file(texFile);
		m_MemoryArena->free_all();

		return texHdl;
	}

	insigne::texture_handle_t TextureManager::CreateTextureCube(const_cstr i_texPath)
	{
		floral::file_info texFile = floral::open_file(i_texPath);
		floral::file_stream dataStream;

		dataStream.buffer = (p8)m_MemoryArena->allocate(texFile.file_size);
		floral::read_all_file(texFile, dataStream);

		c8 magicChars[4];
		dataStream.read_bytes(magicChars, 4);

		s32 colorRange = 0;
		s32 colorSpace = 0;
		s32 colorChannel = 0;
		f32 encodeGamma = 0.0f;
		s32 mipsCount = 0;
		dataStream.read<s32>(&colorRange);
		dataStream.read<s32>(&colorSpace);
		dataStream.read<s32>(&colorChannel);
		dataStream.read<f32>(&encodeGamma);
		dataStream.read<s32>(&mipsCount);

		// TODO: hardcode!!!
		s32 width = 256;
		s32 height = width;
		size dataSizeOneFace = width * height * colorChannel * sizeof(f32);

		voidptr texData = nullptr;
		insigne::texture_handle_t texHdl = insigne::create_texturecube(width, height,
				insigne::texture_format_e::hdr_rgb,
				insigne::filtering_e::linear, insigne::filtering_e::linear,
				dataSizeOneFace, texData, false);
		dataStream.read_bytes((p8)texData, dataSizeOneFace * 6);


		floral::close_file(texFile);
		m_MemoryArena->free_all();

		return texHdl;
	}

	insigne::texture_handle_t TextureManager::CreateMipmapedProbe(const_cstr i_texPath)
	{
		floral::file_info texFile = floral::open_file(i_texPath);
		floral::file_stream dataStream;

		dataStream.buffer = (p8)m_MemoryArena->allocate(texFile.file_size);
		floral::read_all_file(texFile, dataStream);

		c8 magicChars[4];
		dataStream.read_bytes(magicChars, 4);

		s32 colorRange = 0;
		s32 colorSpace = 0;
		s32 colorChannel = 0;
		f32 encodeGamma = 0.0f;
		s32 mipsCount = 0;
		dataStream.read<s32>(&colorRange);
		dataStream.read<s32>(&colorSpace);
		dataStream.read<s32>(&colorChannel);
		dataStream.read<f32>(&encodeGamma);
		dataStream.read<s32>(&mipsCount);

		s32 width = 1 << (mipsCount - 1);
		s32 height = width;
		size dataSizeOneFace = ((1 << (2 * mipsCount)) - 1) / 3 * colorChannel * sizeof(f32);

		voidptr texData = nullptr;
		insigne::texture_handle_t texHdl = insigne::create_texturecube(width, height,
				insigne::texture_format_e::hdr_rgb,
				insigne::filtering_e::linear_mipmap_linear, insigne::filtering_e::linear,
				dataSizeOneFace, texData, true);
		dataStream.read_bytes((p8)texData, dataSizeOneFace * 6);


		floral::close_file(texFile);
		m_MemoryArena->free_all();

		return texHdl;
	}

	insigne::texture_handle_t TextureManager::CreateTexture(const voidptr i_pixels,
			const s32 i_width, const s32 i_height,
			const insigne::texture_format_e i_texFormat)
	{
		return insigne::upload_texture2d(i_width, i_height,
				i_texFormat,
				insigne::filtering_e::nearest, insigne::filtering_e::nearest,
				i_pixels, false);
	}
}
