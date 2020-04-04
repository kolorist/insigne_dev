#include "Empty.h"

#include <insigne/ut_render.h>

#include "InsigneImGui.h"
#include "Graphics/MaterialParser.h"
#include "Memory/MemorySystem.h"

namespace stone
{
namespace perf
{

static const_cstr k_SuiteName = "no_rendering";

Empty::Empty()
{
}

Empty::~Empty()
{
}

const_cstr Empty::GetName() const
{
	return k_SuiteName;
}

void Empty::OnInitialize()
{
	FreelistArena* localArena = g_StreammingAllocator.allocate_arena<FreelistArena>(SIZE_MB(1));
	const_cstr matDescStr =
	R"(_shader
		_vs	gfx/shader/pbr_cook_torrance.vs
		_fs gfx/shader/pbr_cook_torrance.fs
		_enable_feature	DEBUG_FEATURE0
		_enable_feature	DEBUG_FEATURE1
	_end_shader

	_params
		_p_ub ub_CustomMaterial
			int			iu_TestInt		12
			float		iu_TestFloat	23.4
			vec2		iu_TestVec2		1.0 2.0
			vec3		iu_TestVec3		3.0 4.0 5.0
			vec4		iu_TestVec4		6.0 7.0 8.0 9.0
		_end_p_ub

		_p_ub_holder	ub_LateBindMaterial

		_p_tex u_PMREM
			_dim		texcube
			_min_filter	linear_mipmap_linear
			_mag_filter	linear
			_wrap_s		clamp_to_edge
			_wrap_t		clamp_to_edge
			_wrap_r		clamp_to_edge
			_data		gfx/go/textures/demo/test_albedo.prb
		_end_p_tex

		_p_tex u_AlbedoTex
			_dim		tex2d
			_min_filter	linear_mipmap_linear
			_mag_filter	linear
			_wrap_s		clamp_to_edge
			_wrap_t		clamp_to_edge
			_data		gfx/go/textures/demo/test_albedo.cbtex
		_end_p_tex

		_p_tex_holder	u_LateBindTex
	_end_params)";

	mat_parser::MaterialDescription matDesc = mat_parser::ParseMaterial(matDescStr, localArena);
}

void Empty::OnUpdate(const f32 i_deltaMs)
{
}

void Empty::OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	RenderImGui();

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void Empty::OnCleanUp()
{
}

}
}
