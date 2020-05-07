#include "Memory/MemorySystem.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace stone
{
namespace stb
{

voidptr malloc(const size i_sz)
{
	return g_STBArena.allocate(i_sz);
}

voidptr realloc(voidptr i_data, const size i_newSize)
{
	return g_STBArena.reallocate(i_data, i_newSize);
}

void free(voidptr i_data)
{
	if (i_data)
	{
		g_STBArena.free(i_data);
	}
}

}
}

#define STBI_MALLOC(sz)							stone::stb::malloc(sz)
#define STBI_REALLOC(p, newsz)					stone::stb::realloc(p, newsz)
#define STBI_FREE(p)							stone::stb::free(p)

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STBIR_MALLOC(size, context)				((void)context, stone::stb::malloc(size))
#define STBIR_FREE(ptr, context)				((void)context, stone::stb::free(ptr))

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

#define STB_DXT_IMPLEMENTATION
#include "stb_dxt.h"
