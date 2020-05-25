#include "Samplers.h"

#include <imgui.h>

#include <insigne/system.h>
#include <insigne/ut_render.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_textures.h>
#include <insigne/ut_shading.h>

#include <floral/math/utils.h>

#include <clover/Logger.h>

#include "InsigneImGui.h"
#include "Graphics/DebugDrawer.h"

namespace stone
{
namespace tools
{

//----------------------------------------------

const floral::vec3f get_uniform_sphere_sample(const floral::vec2f& i_uniformInput)
{
	f32 z = 1.0f - 2.0f * i_uniformInput.x;
	f32 r = sqrtf(floral::max(0.0f, 1.0f - z * z));
	f32 phi = 2.0f * floral::pi * i_uniformInput.y;
	return floral::vec3f(r * cosf(phi), z, r * sinf(phi));
}

const floral::vec3f get_uniform_hemisphere_sample(const floral::vec2f& i_uniformInput)
{
	f32 z = i_uniformInput.x;
	f32 r = sqrtf(floral::max(0.0f, 1.0f - z * z));
	f32 phi = 2.0f * floral::pi * i_uniformInput.y;
	return floral::vec3f(r * cosf(phi), z, r * sinf(phi));
}

//----------------------------------------------

static const_cstr k_SuiteName = "Samplers";

Samplers::Samplers()
	: m_CameraMotion(
		floral::camera_view_t { floral::vec3f(6.0f, 6.0f, 6.0f), floral::vec3f(-3.0f, -3.0f, -3.0f), floral::vec3f(0.0f, 1.0f, 0.0f) },
		floral::camera_persp_t { 0.01f, 100.0f, 60.0f, 16.0f / 9.0f })
	, m_RNG(u64(k_SuiteName))
	, m_MemoryArena(nullptr)
{
}

Samplers::~Samplers()
{
}

const_cstr Samplers::GetName() const
{
	return k_SuiteName;
}

void Samplers::OnInitialize(floral::filesystem<FreelistArena>* i_fs)
{
	CLOVER_VERBOSE("Initializing '%s' TestSuite", k_SuiteName);
	// snapshot begin state
	m_BuffersBeginStateId = insigne::get_buffers_resource_state();
	m_ShadingBeginStateId = insigne::get_shading_resource_state();
	m_TextureBeginStateId = insigne::get_textures_resource_state();
	m_RenderBeginStateId = insigne::get_render_resource_state();

	m_MemoryArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_MB(2));

	m_SamplePos.reserve(256, m_MemoryArena);
	for (size i = 0; i < 256; i++)
	{
		floral::vec2f uniInput(0.0f);
		uniInput.x = m_RNG.get_f32();
		uniInput.y = m_RNG.get_f32();
		floral::vec3f samplePos = get_uniform_hemisphere_sample(uniInput);
		//floral::vec3f samplePos = get_uniform_sphere_sample(uniInput);
		m_SamplePos.push_back(samplePos);
	}

	{
		insigne::ubdesc_t desc;
		desc.region_size = SIZE_KB(4);
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ub_handle_t newUB = insigne::create_ub(desc);

		// camera
		m_SceneData.WVP = m_CameraMotion.GetWVP();

		insigne::update_ub(newUB, &m_SceneData, sizeof(SceneData), 0);
		m_UB = newUB;
	}
}

void Samplers::OnUpdate(const f32 i_deltaMs)
{
	// Logic update
	m_CameraMotion.OnUpdate(i_deltaMs);
	m_SceneData.WVP = m_CameraMotion.GetWVP();

	for (size i = 0; i < m_SamplePos.get_size(); i++)
	{
		debugdraw::DrawPoint3D(m_SamplePos[i], 0.1f, i);
	}
}

void Samplers::OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	debugdraw::Render(m_SceneData.WVP);
	RenderImGui();

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void Samplers::OnCleanUp()
{
	CLOVER_VERBOSE("Cleaning up '%s' TestSuite", k_SuiteName);

	g_StreammingAllocator.free(m_MemoryArena);
	m_MemoryArena = nullptr;

	insigne::cleanup_render_resource(m_RenderBeginStateId);
	insigne::cleanup_textures_resource(m_TextureBeginStateId);
	insigne::cleanup_shading_resource(m_ShadingBeginStateId);
	insigne::cleanup_buffers_resource(m_BuffersBeginStateId);

	insigne::dispatch_render_pass();
	insigne::wait_finish_dispatching();
}

}
}
