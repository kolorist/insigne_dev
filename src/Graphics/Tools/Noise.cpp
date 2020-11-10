#include "Noise.h"

#include <clover/Logger.h>
#include <insigne/ut_render.h>

#include "InsigneImGui.h"

#include "Graphics/stb_image_write.h"
#include "Graphics/open-simplex-noise.h"

namespace stone
{
namespace tools
{
//-------------------------------------------------------------------


Noise::Noise()
{
}

Noise::~Noise()
{
}

ICameraMotion* Noise::GetCameraMotion()
{
	return nullptr;
}

const_cstr Noise::GetName() const
{
	return k_name;
}

void Noise::_OnInitialize()
{
	m_MemoryArena = g_StreammingAllocator.allocate_arena<FreelistArena>(SIZE_MB(16));

	// 3 octaves
	osn_context osnContext[3];
	open_simplex_noise(0, &osnContext[0]);
	open_simplex_noise(2, &osnContext[1]);
	open_simplex_noise(4, &osnContext[2]);

	const s32 k_size = 512;
	const f32 k_period = 1.1f;
	const f32 k_lacunarity = 2.0f;
	const f32 k_persistence = 0.0f;
	p8 colorData = (p8)m_MemoryArena->allocate(k_size * k_size * 3);
	for (int y = 0; y < k_size; y++)
	{
		for (int x = 0; x < k_size; x++)
		{
			f32 yy = (f32)y / k_size;
			f32 xx = (f32)x / k_size;

			yy *= 2.0f * floral::pi;
			xx *= 2.0f * floral::pi;

			f32 radius = k_size / (2.0f * floral::pi);

			f32 nx = radius * sinf(xx);
			f32 ny = radius * cosf(xx);
			f32 nz = radius * sinf(yy);
			f32 nw = radius * cosf(yy);

			f32 x2 = nx / k_period;
			f32 y2 = ny / k_period;
			f32 z2 = nz / k_period;
			f32 w2 = nw / k_period;

			f32 amp = 1.0;
			f32 max = 1.0;

			f32 sum = open_simplex_noise4(&osnContext[0], x2, y2, z2, w2);

			int i = 1;
			while (i < 3)
			{
				x2 *= k_lacunarity;
				y2 *= k_lacunarity;
				z2 *= k_lacunarity;
				w2 *= k_lacunarity;

				amp *= k_persistence;
				max += amp;
				sum += open_simplex_noise4(&osnContext[i], x2, y2, z2, w2) * amp;
				i++;
			}
			sum /= max;

			f32 v = sum * 0.5f + 0.5f;
			u8 value8Bit = floral::clamp((s32)(v * 255), 0, 255);
			colorData[(y * k_size + x) * 3] = value8Bit;
			colorData[(y * k_size + x) * 3 + 1] = value8Bit;
			colorData[(y * k_size + x) * 3 + 2] = value8Bit;
		}
	}

	stbi_write_tga("out.tga", k_size, k_size, 3, colorData);
}

void Noise::_OnUpdate(const f32 i_deltaMs)
{
}

void Noise::_OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	RenderImGui();

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void Noise::_OnCleanUp()
{
	g_StreammingAllocator.free(m_MemoryArena);
}

//-------------------------------------------------------------------
}
}
