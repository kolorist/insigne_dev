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

		voidptr texData = nullptr;
		insigne::texture_handle_t texHdl = insigne::create_texture2d(width, height,
				insigne::texture_format_e::rgba, width * height * 4, texData);
		dataStream.read_bytes((p8)texData, width * height * 4);

		floral::close_file(texFile);
		m_MemoryArena->free_all();

		return texHdl;
	}

	insigne::texture_handle_t TextureManager::CreateTexture(const voidptr i_pixels,
			const s32 i_width, const s32 i_height,
			const insigne::texture_format_e i_texFormat)
	{
		return insigne::upload_texture2d(i_width, i_height, i_texFormat, i_pixels);
	}
}
