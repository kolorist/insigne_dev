#pragma once

#include <insigne/commons.h>

namespace stone {
	class ITextureManager {
		public:
			virtual insigne::texture_handle_t	CreateTexture(const_cstr i_texPath) = 0;
			virtual insigne::texture_handle_t	CreateTexture(const voidptr i_pixels,
													const s32 i_width, const s32 i_height,
													const insigne::texture_format_e i_texFormat) = 0;
	};
}
