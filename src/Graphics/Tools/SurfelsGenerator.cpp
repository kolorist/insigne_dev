#include "SurfelsGenerator.h"

#include <imgui.h>

#include <insigne/system.h>
#include <insigne/ut_render.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_textures.h>
#include <insigne/ut_shading.h>

#include <floral/math/coordinate.h>

#include <clover/Logger.h>

#include "InsigneImGui.h"
#include "Graphics/DebugDrawer.h"
#include "Graphics/PlyLoader.h"

namespace stone
{
namespace tools
{

static const_cstr k_SuiteName = "Surfels Generator";

SurfelsGenerator::SurfelsGenerator()
	: m_CameraMotion(
		floral::camera_view_t { floral::vec3f(6.0f, 6.0f, 6.0f), floral::vec3f(-3.0f, -3.0f, -3.0f), floral::vec3f(0.0f, 1.0f, 0.0f) },
		floral::camera_persp_t { 0.01f, 100.0f, 60.0f, 16.0f / 9.0f })
	, m_RNG(u64(k_SuiteName))
	, m_MemoryArena(nullptr)
{
}

SurfelsGenerator::~SurfelsGenerator()
{
}

const_cstr SurfelsGenerator::GetName() const
{
	return k_SuiteName;
}

void SurfelsGenerator::OnInitialize(floral::filesystem<FreelistArena>* i_fs)
{
	CLOVER_VERBOSE("Initializing '%s' TestSuite", k_SuiteName);
	// snapshot begin state
	m_BuffersBeginStateId = insigne::get_buffers_resource_state();
	m_ShadingBeginStateId = insigne::get_shading_resource_state();
	m_TextureBeginStateId = insigne::get_textures_resource_state();
	m_RenderBeginStateId = insigne::get_render_resource_state();

	m_MemoryArena = g_StreammingAllocator.allocate_arena<FreelistArena>(SIZE_MB(2));
	m_ResourceArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_MB(2));

	floral::fixed_array<f32, FreelistArena> triangleAreas;
	f32 totalArea = 0.0f;
	{
		PlyData<LinearArena> plyData = LoadFromPly(floral::path("gfx/envi/models/demo/cornel_box_triangles_pp.ply"), m_ResourceArena);
		CLOVER_INFO("Mesh loaded");
		size vtxCount = plyData.Position.get_size();
		size idxCount = plyData.Indices.get_size();

		m_Vertices.reserve(idxCount, m_MemoryArena);
		triangleAreas.reserve(idxCount / 3, m_MemoryArena);
		for (size i = 0; i < idxCount; i += 3)
		{
			VertexPC v0, v1, v2;
			v0.Position = plyData.Position[plyData.Indices[i]];
			v0.Color = floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f);
			v1.Position = plyData.Position[plyData.Indices[i + 1]];
			v1.Color = floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f);
			v2.Position = plyData.Position[plyData.Indices[i + 2]];
			v2.Color = floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f);

			m_Vertices.push_back(v0);
			m_Vertices.push_back(v1);
			m_Vertices.push_back(v2);

			f32 s = floral::get_triangle_area(v0.Position, v1.Position, v2.Position);
			FLORAL_ASSERT(s > 0.0f);
			triangleAreas.push_back(s);
			totalArea += s;
		}
	}

	size sampleArrSize = 2048;
	size numTri = m_Vertices.get_size() / 3;
	m_SamplePos.reserve(sampleArrSize, m_MemoryArena);
	for (size i = 0; i < sampleArrSize; i++)
	{
		f32 as = m_RNG.get_f32() * totalArea;
		size triIdx = triangleAreas.get_size() - 1;
		for (size j = 0; j < triangleAreas.get_size(); j++)
		{
			as -= triangleAreas[j];
			if (as <= 0.0f)
			{
				triIdx = j;
				break;
			}
		}

		const VertexPC& v0 = m_Vertices[triIdx * 3];
		const VertexPC& v1 = m_Vertices[triIdx * 3 + 1];
		const VertexPC& v2 = m_Vertices[triIdx * 3 + 2];

		f32 sqrR1 = sqrtf(m_RNG.get_f32());
		f32 r2 = m_RNG.get_f32();

		floral::vec3f samplePos = (1.0f - sqrR1) * v0.Position + sqrR1 * (1.0f - r2) * v1.Position + sqrR1 * r2 * v2.Position;
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

void SurfelsGenerator::OnUpdate(const f32 i_deltaMs)
{
	//ImGui::ShowTestWindow();

	// Logic update
	m_CameraMotion.OnUpdate(i_deltaMs);
	m_SceneData.WVP = m_CameraMotion.GetWVP();

	//debugdraw::DrawLine3D(floral::vec3f(0.0f), floral::vec3f(3.0f, 0.0f, 0.0f), floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f));
	for (size i = 0; i < m_Vertices.get_size(); i += 3)
	{
		const VertexPC& v0 = m_Vertices[i];
		const VertexPC& v1 = m_Vertices[i + 1];
		const VertexPC& v2 = m_Vertices[i + 2];

		debugdraw::DrawLine3D(v0.Position, v1.Position, v0.Color);
		debugdraw::DrawLine3D(v1.Position, v2.Position, v1.Color);
		debugdraw::DrawLine3D(v2.Position, v0.Position, v2.Color);
	}

	for (size i = 0; i < m_SamplePos.get_size(); i++)
	{
		debugdraw::DrawPoint3D(m_SamplePos[i], 0.02f, i);
	}
}

void SurfelsGenerator::OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	debugdraw::Render(m_SceneData.WVP);
	RenderImGui();

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void SurfelsGenerator::OnCleanUp()
{
	CLOVER_VERBOSE("Cleaning up '%s' TestSuite", k_SuiteName);

	g_StreammingAllocator.free(m_ResourceArena);
	g_StreammingAllocator.free(m_MemoryArena);
	m_ResourceArena = nullptr;
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
