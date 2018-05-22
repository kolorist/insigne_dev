#include "TextureManager.h"

#include <insigne/render.h>

namespace stone {
	TextureManager::TextureManager()
	{
	}

	TextureManager::~TextureManager()
	{
	}

	insigne::texture_handle_t TextureManager::CreateTexture(const_cstr i_texPath)
	{
		return 0;
	}

	insigne::texture_handle_t TextureManager::CreateTexture(const voidptr i_pixels,
			const s32 i_width, const s32 i_height,
			const insigne::texture_format_e i_texFormat)
	{
		return insigne::upload_texture2d(i_width, i_height, i_texFormat, i_pixels);
	}
}
