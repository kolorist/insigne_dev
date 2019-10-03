#include "FragmentPartition.h"

#include <clover/Logger.h>

#include <insigne/system.h>
#include <insigne/ut_render.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_textures.h>

#include "InsigneImGui.h"
#include "Graphics/DebugDrawer.h"
#include "Graphics/PlyLoader.h"

//----------------------------------------------
#include "FragmentPartitionShaders.inl"
//----------------------------------------------

namespace stone
{
namespace tech
{

static const_cstr k_SuiteName = "fragment partition";

//----------------------------------------------

FragmentPartition::FragmentPartition()
	: m_CameraMotion(
		floral::camera_view_t { floral::vec3f(0.0f, 1.0f, -6.0f), floral::vec3f(0.0f, 0.0f, 1.0f), floral::vec3f(0.0f, 1.0f, 0.0f) },
		floral::camera_persp_t { 0.01f, 100.0f, 60.0f, 16.0f / 9.0f })
	, m_SHData(nullptr)
	, m_ResourceArena(nullptr)
	, m_TemporalArena(nullptr)
{
}

FragmentPartition::~FragmentPartition()
{
}

const_cstr FragmentPartition::GetName() const
{
	return k_SuiteName;
}

void FragmentPartition::OnInitialize()
{
	CLOVER_VERBOSE("Initializing '%s' TestSuite", k_SuiteName);
	// snapshot begin state
	m_BuffersBeginStateId = insigne::get_buffers_resource_state();
	m_ShadingBeginStateId = insigne::get_shading_resource_state();
	m_TextureBeginStateId = insigne::get_textures_resource_state();
	m_RenderBeginStateId = insigne::get_render_resource_state();

	// register surfaces
	insigne::register_surface_type<SurfaceP>();

	// memory arena
	m_ResourceArena = g_PersistanceResourceAllocator.allocate_arena<LinearArena>(SIZE_MB(2));
	m_TemporalArena = g_StreammingAllocator.allocate_arena<FreelistArena>(SIZE_MB(2));

	{
		floral::vec3f minCorner(9999.0f);
		floral::vec3f maxCorner(-9999.0f);

		PlyData<FreelistArena> plyData = LoadFromPly(floral::path("gfx/envi/models/demo/cornel_box_triangles_pp.ply"), m_TemporalArena);
		CLOVER_INFO("Mesh loaded");
		size vtxCount = plyData.Position.get_size();
		size idxCount = plyData.Indices.get_size();

		m_VerticesData.reserve(vtxCount, m_ResourceArena);
		m_IndicesData.reserve(idxCount, m_ResourceArena);

		for (size i = 0; i < vtxCount; i++)
		{
			VertexP v;
			v.Position = plyData.Position[i];
			m_VerticesData.push_back(v);
			if (v.Position.x < minCorner.x) minCorner.x = v.Position.x;
			if (v.Position.y < minCorner.y) minCorner.y = v.Position.y;
			if (v.Position.z < minCorner.z) minCorner.z = v.Position.z;
			if (v.Position.x > maxCorner.x) maxCorner.x = v.Position.x;
			if (v.Position.y > maxCorner.y) maxCorner.y = v.Position.y;
			if (v.Position.z > maxCorner.z) maxCorner.z = v.Position.z;
		}

		for (size i = 0; i < idxCount; i++)
		{
			m_IndicesData.push_back(plyData.Indices[i]);
		}

		f32 stepCount = 3.0f;
		minCorner = minCorner - floral::vec3f(0.1f);
		maxCorner = maxCorner + floral::vec3f(0.1f);
		m_WorldData.BBMinCorner = floral::vec4f(minCorner.x, minCorner.y, minCorner.z, 0.0f);
		m_WorldData.BBDimension = floral::vec4f(
				maxCorner.x - minCorner.x,
				maxCorner.y - minCorner.y,
				maxCorner.z - minCorner.z,
				0.0f) / stepCount;
	}

	{
		insigne::vbdesc_t desc;
		desc.region_size = SIZE_KB(32);
		desc.stride = sizeof(VertexP);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::static_draw;

		m_VB = insigne::create_vb(desc);
		insigne::copy_update_vb(m_VB, &m_VerticesData[0], m_VerticesData.get_size(), sizeof(VertexP), 0);
	}

	{
		insigne::ibdesc_t desc;
		desc.region_size = SIZE_KB(16);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::static_draw;

		m_IB = insigne::create_ib(desc);
		insigne::copy_update_ib(m_IB, &m_IndicesData[0], m_IndicesData.get_size(), 0);
	}

	{
		insigne::ubdesc_t desc;
		desc.region_size = 512;
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		m_UB = insigne::create_ub(desc);

		m_SceneData.WVP = m_CameraMotion.GetWVP();
		insigne::copy_update_ub(m_UB, &m_SceneData, sizeof(SceneData), 0);
		insigne::copy_update_ub(m_UB, &m_WorldData, sizeof(WorldData), 256);
	}

	// sh data
	{
		size shDataSize = sizeof(SHData);
		m_SHData = m_ResourceArena->allocate<SHData>();

		memset(m_SHData, 0, shDataSize);
		for (size i = 0; i < 64; i++)
		{
			m_SHData->Probes[i].CoEffs[0] = floral::vec4f(f32(i) / 64.0f, 0.0f, 0.0f, 1.0f);
		}

		insigne::ubdesc_t desc;
		desc.region_size = shDataSize;
		desc.data = m_SHData;
		desc.data_size = shDataSize;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		m_SHUB = insigne::create_ub(desc);
	}

	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Scene", insigne::param_data_type_e::param_ub));
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_World", insigne::param_data_type_e::param_ub));
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_SHData", insigne::param_data_type_e::param_ub));

		strcpy(desc.vs, s_VertexShaderCode);
		strcpy(desc.fs, s_FragmentShaderCode);
		desc.vs_path = floral::path("/demo/simple_vs");
		desc.fs_path = floral::path("/demo/simple_fs");

		m_Shader = insigne::create_shader(desc);
		insigne::infuse_material(m_Shader, m_Material);

		{
			s32 ubSlot = insigne::get_material_uniform_block_slot(m_Material, "ub_Scene");
			m_Material.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 0, 256, m_UB };
		}
		{
			s32 ubSlot = insigne::get_material_uniform_block_slot(m_Material, "ub_World");
			m_Material.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 256, 256, m_UB };
		}
		{
			s32 ubSlot = insigne::get_material_uniform_block_slot(m_Material, "ub_SHData");
			m_Material.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 0, 0, m_SHUB };
		}
	}
}

void FragmentPartition::OnUpdate(const f32 i_deltaMs)
{
	// Logic update
	m_CameraMotion.OnUpdate(i_deltaMs);
	m_SceneData.WVP = m_CameraMotion.GetWVP();
}

void FragmentPartition::OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::draw_surface<SurfaceP>(m_VB, m_IB, m_Material);
	debugdraw::Render(m_SceneData.WVP);
	RenderImGui();
	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void FragmentPartition::OnCleanUp()
{
	CLOVER_VERBOSE("Cleaning up '%s' TestSuite", k_SuiteName);

	g_StreammingAllocator.free(m_TemporalArena);
	m_TemporalArena = nullptr;
	g_PersistanceResourceAllocator.free(m_ResourceArena);
	m_ResourceArena = nullptr;

	insigne::unregister_surface_type<SurfaceP>();

	insigne::cleanup_render_resource(m_RenderBeginStateId);
	insigne::cleanup_textures_resource(m_TextureBeginStateId);
	insigne::cleanup_shading_resource(m_ShadingBeginStateId);
	insigne::cleanup_buffers_resource(m_BuffersBeginStateId);

	insigne::dispatch_render_pass();
	insigne::wait_finish_dispatching();
}

}
}
