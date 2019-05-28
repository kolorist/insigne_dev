#include "SHTest.h"

#include <clover/Logger.h>

#include <insigne/commons.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_render.h>
#include <insigne/ut_textures.h>

#include "Graphics/prt.h"
#include "Graphics/GeometryBuilder.h"
#include "Graphics/CBTexDefinitions.h"

namespace stone
{

SHTest::SHTest()
	: m_CameraMotion(
			floral::camera_view_t { floral::vec3f(3.0f, 3.0f, 3.0f), floral::vec3f(-3.0f, -3.0f, -3.0f), floral::vec3f(0.0f, 1.0f, 0.0f) },
			floral::camera_persp_t { 0.01f, 100.0f, 60.0f, 16.0f / 9.0f })
	, m_Counter(1)
{
	m_MemoryArena = g_PersistanceResourceAllocator.allocate_arena<LinearArena>(SIZE_MB(16));
}

SHTest::~SHTest()
{
}

void SHTest::OnInitialize()
{
	{
		insigne::ubdesc_t desc;
		desc.region_size = SIZE_KB(4);
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ub_handle_t newUB = insigne::create_ub(desc);

		// camera
		m_SceneData.WVP = m_CameraMotion.GetWVP();
		m_SceneData.XForm = floral::mat4x4f(1.0f);

		insigne::update_ub(newUB, &m_SceneData, sizeof(SceneData), 0);
		m_UB = newUB;
	}

	{
		m_MemoryArena->free_all();
		floral::file_info texFile = floral::open_file("gfx/envi/textures/demo/grace_probe.cbtex");
		floral::file_stream dataStream;
		dataStream.buffer = (p8)m_MemoryArena->allocate(texFile.file_size);
		floral::read_all_file(texFile, dataStream);
		floral::close_file(texFile);

		cb::CBTexture2DHeader header;
		dataStream.read(&header);

		FLORAL_ASSERT(header.colorRange == cb::ColorRange::HDR);

		insigne::texture_desc_t texDesc;
		texDesc.width = header.resolution;
		texDesc.height = header.resolution;
		texDesc.format = insigne::texture_format_e::hdr_rgb;
		texDesc.min_filter = insigne::filtering_e::linear;
		texDesc.mag_filter = insigne::filtering_e::linear;
		texDesc.dimension = insigne::texture_dimension_e::tex_2d;
		texDesc.has_mipmap = false;

		const size dataSize = insigne::prepare_texture_desc(texDesc);

		// TODO: memcpy? really?
		dataStream.read_bytes(texDesc.data, dataSize);
		m_Texture = insigne::create_texture(texDesc);
	}

	m_DebugDrawer.Initialize();
}

void SHTest::OnUpdate(const f32 i_deltaMs)
{
	m_CameraMotion.OnUpdate(i_deltaMs);

	m_DebugDrawer.BeginFrame();

	// ground grid cover [-2.0..2.0]
	floral::vec4f gridColor(0.3f, 0.4f, 0.5f, 1.0f);
	for (s32 i = -2; i < 3; i++)
	{
		for (s32 j = -2; j < 3; j++)
		{
			floral::vec3f startX(1.0f * i, 0.0f, 1.0f * j);
			floral::vec3f startZ(1.0f * j, 0.0f, 1.0f * i);
			floral::vec3f endX(1.0f * i, 0.0f, -1.0f * j);
			floral::vec3f endZ(-1.0f * j, 0.0f, 1.0f * i);
			m_DebugDrawer.DrawLine3D(startX, endX, gridColor);
			m_DebugDrawer.DrawLine3D(startZ, endZ, gridColor);
		}
	}

	// coordinate unit vectors
	m_DebugDrawer.DrawLine3D(floral::vec3f(0.1f, 0.0f, 0.1f), floral::vec3f(0.5f, 0.0f, 0.1f), floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f));
	m_DebugDrawer.DrawLine3D(floral::vec3f(0.1f, 0.0f, 0.1f), floral::vec3f(0.1f, 0.5f, 0.1f), floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f));
	m_DebugDrawer.DrawLine3D(floral::vec3f(0.1f, 0.0f, 0.1f), floral::vec3f(0.1f, 0.0f, 0.5f), floral::vec4f(0.0f, 0.0f, 1.0f, 1.0f));
	m_DebugDrawer.EndFrame();
}

void SHTest::OnDebugUIUpdate(const f32 i_deltaMs)
{
	ImGui::Begin("DebugUITest Controller");
	ImGui::Text("grace_probe");
	ImGui::Image(&m_Texture, ImVec2(128, 128));
	if (ImGui::Button("Test Task"))
	{
		CLOVER_DEBUG("Task enqueued");
		m_Counter.store(1);
		refrain2::Task newTask;
		newTask.pm_Instruction = &SHTest::ComputeSHCoeffs;
		newTask.pm_Data = nullptr;
		newTask.pm_Counter = &m_Counter;
		refrain2::g_TaskManager->PushTask(newTask);
	}
	if (refrain2::CheckForCounter(m_Counter, 0))
	{
		ImGui::Text("Finished");
	}
	ImGui::End();
}

void SHTest::OnRender(const f32 i_deltaMs)
{
	// camera
	m_SceneData.WVP = m_CameraMotion.GetWVP();
	insigne::update_ub(m_UB, &m_SceneData, sizeof(SceneData), 0);

	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	m_DebugDrawer.Render(m_SceneData.WVP);

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void SHTest::OnCleanUp()
{
}

//----------------------------------------------
refrain2::Task SHTest::ComputeSHCoeffs(voidptr i_data)
{
	f32 t = 0.0f;
	for (u32 i = 0; i < 1000000000; i++)
	{
		t += i;
	}
	CLOVER_VERBOSE("finish");
	return refrain2::Task();
}

}
