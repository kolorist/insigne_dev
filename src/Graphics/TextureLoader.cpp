#include "TextureLoader.h"

#include <floral/assert/assert.h>

#include <insigne/ut_textures.h>

#include "Graphics/stb_image.h"
#include "Graphics/stb_image_resize.h"

namespace tex_loader
{
// ----------------------------------------------------------------------------

const insigne::texture_handle_t LoadLDRTexture2D(const floral::path& i_path, insigne::texture_desc_t& io_desc, const bool i_createMipmaps /* = false */)
{
	s32 width = 0, height = 0, numChannels = 0;
	p8 imgData = stbi_load(i_path.pm_PathStr, &width, &height, &numChannels, 0);

	if (i_createMipmaps)
	{
		if (width != height)
		{
			FLORAL_ASSERT_MSG(false, "Cannot create mipmap NPOT texture");
			return insigne::texture_handle_t();
		}
	}

	io_desc.width = width;
	io_desc.height = height;
	if (numChannels == 2)
	{
		io_desc.format = insigne::texture_format_e::rg;
	}
	else if (numChannels == 3)
	{
		io_desc.format = insigne::texture_format_e::rgb;
	}
	else if (numChannels == 4)
	{
		io_desc.format = insigne::texture_format_e::rgba;
	}
	else
	{
		FLORAL_ASSERT_MSG(false, "Bad number of channels");
		return insigne::texture_handle_t();
	}
	io_desc.dimension = insigne::texture_dimension_e::tex_2d;
	io_desc.has_mipmap = i_createMipmaps;
	insigne::prepare_texture_desc(io_desc);
	aptr readSize = 0;

	// copy mip 0
	memcpy((p8)io_desc.data, imgData, width * height * numChannels);
	readSize += width * height * numChannels;

	if (i_createMipmaps)
	{
		s32 mipsCount = (s32)log2(width) + 1;
		for (s32 i = 1; i < mipsCount; i++)
		{
			s32 nx = width >> i;
			p8 rzdata = (p8)((aptr)io_desc.data + readSize);
			stbir_resize_uint8(imgData, width, width, 0,
					rzdata, nx, nx, 0, numChannels);
			readSize += nx * nx * numChannels;
		}
	}

	stbi_image_free(imgData);
	return insigne::create_texture(io_desc);
}

// ----------------------------------------------------------------------------
}
