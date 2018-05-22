#pragma once

#include "ITextureManager.h"

#include <insigne/commons.h>

namespace stone {
	class TextureManager : public ITextureManager {
		public:
			TextureManager();
			~TextureManager();

			insigne::texture_handle_t			CreateTexture(const_cstr i_texPath) override;
			insigne::texture_handle_t			CreateTexture(const voidptr i_pixels,
													const s32 i_width, const s32 i_height,
													const insigne::texture_format_e i_texFormat) override;
	};
}
