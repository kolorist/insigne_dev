#pragma once

#include <floral.h>
#include <insigne/commons.h>

namespace stone {
class ITextureManager {
	public:
		virtual void							Initialize(const u32 i_maxTexturesCount) = 0;

		virtual insigne::texture_handle_t		CreateTexture(const floral::path& i_texPath) = 0;
		virtual insigne::texture_handle_t		CreateTexture(const_cstr i_texPath) = 0;
		virtual insigne::texture_handle_t		CreateTextureCube(const floral::path& i_texPath) = 0;
		virtual insigne::texture_handle_t		CreateTextureCube(const_cstr i_texPath) = 0;
		virtual insigne::texture_handle_t		CreateMipmapedProbe(const_cstr i_texPath) = 0;
		virtual insigne::texture_handle_t		CreateTexture(const voidptr i_pixels,
													const s32 i_width, const s32 i_height,
													const insigne::texture_format_e i_texFormat) = 0;
		virtual insigne::texture_handle_t		CreateLUTTexture(const s32 i_width, const s32 i_height) = 0;
};
}
