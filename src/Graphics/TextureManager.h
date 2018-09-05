#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "ITextureManager.h"
#include "Memory/MemorySystem.h"

namespace stone {

class TextureManager : public ITextureManager {

	struct TextureRegister {
		floral::crc_string						key;
		insigne::texture_handle_t				textureHandle;
	};

	public:
		TextureManager();
		~TextureManager();

		void									Initialize(const u32 i_maxTexturesCount) override;

		insigne::texture_handle_t				CreateTexture(const floral::path& i_texPath) override;
		insigne::texture_handle_t				CreateTexture(const_cstr i_texPath) override;
		insigne::texture_handle_t				CreateTextureCube(const floral::path& i_texPath) override;
		insigne::texture_handle_t				CreateTextureCube(const_cstr i_texPath) override;
		insigne::texture_handle_t				CreateMipmapedProbe(const floral::path& i_texPath) override;
		insigne::texture_handle_t				CreateMipmapedProbe(const_cstr i_texPath) override;
		insigne::texture_handle_t				CreateTexture(const voidptr i_pixels,
													const s32 i_width, const s32 i_height,
													const insigne::texture_format_e i_texFormat) override;
		insigne::texture_handle_t				CreateLUTTexture(const s32 i_width, const s32 i_height) override;

	private:
		LinearArena*							m_MemoryArena;

		floral::fixed_array<TextureRegister, LinearAllocator>	m_CachedTextures;
};

}
