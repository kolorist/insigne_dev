#include "LightProbeGI.h"

#include <floral/function/simple_callback.h>
#include <floral/comgeo/shapegen.h>
#include <floral/math/utils.h>

#include <clover/Logger.h>

#include <calyx/context.h>

#include <insigne/system.h>
#include <insigne/ut_render.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_textures.h>

#include "InsigneImGui.h"
#include "Graphics/DebugDrawer.h"
#include "Graphics/PlyLoader.h"

//----------------------------------------------
#include "LightProbeGIShaders.inl"
//----------------------------------------------

namespace stone
{
namespace tech
{

static const_cstr k_SuiteName = "lightprobe gi";

size clamp(size inp, size lo, size hi)
{
	return floral::max(lo, floral::min(inp, hi));
}

//----------------------------------------------

LightProbeGI::LightProbeGI()
	: m_CameraMotion(
		floral::camera_view_t { floral::vec3f(0.0f, 1.0f, -6.0f), floral::vec3f(0.0f, 0.0f, 1.0f), floral::vec3f(0.0f, 1.0f, 0.0f) },
		floral::camera_persp_t { 0.01f, 100.0f, 60.0f, 16.0f / 9.0f })
	, m_SHData(nullptr)
	, m_ResourceArena(nullptr)
	, m_TemporalArena(nullptr)
	, m_SHBakingStarted(false)
	, m_SHReady(false)
	, m_DrawSHProbes(false)
{
}

LightProbeGI::~LightProbeGI()
{
}

const_cstr LightProbeGI::GetName() const
{
	return k_SuiteName;
}

void LightProbeGI::OnInitialize()
{
	CLOVER_VERBOSE("Initializing '%s' TestSuite", k_SuiteName);
	// snapshot begin state
	m_BuffersBeginStateId = insigne::get_buffers_resource_state();
	m_ShadingBeginStateId = insigne::get_shading_resource_state();
	m_TextureBeginStateId = insigne::get_textures_resource_state();
	m_RenderBeginStateId = insigne::get_render_resource_state();

	// register surfaces
	insigne::register_surface_type<SurfacePNC>();
	insigne::register_surface_type<SurfaceP>();
	insigne::register_surface_type<SurfacePT>();

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
			VertexPNC v;
			v.Position = plyData.Position[i];
			v.Normal = plyData.Normal[i];
			v.Color = floral::vec4f(plyData.Color[i], 1.0f);
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
		floral::vec3f bakeMinCorner = minCorner + floral::vec3f(0.1f);
		floral::vec3f bakeMaxCorner = maxCorner - floral::vec3f(0.1f);
		floral::vec3f bakeDimension = (bakeMaxCorner - bakeMinCorner) / stepCount;

		minCorner = minCorner - floral::vec3f(0.1f);
		maxCorner = maxCorner + floral::vec3f(0.1f);
		m_WorldData.BBMinCorner = floral::vec4f(minCorner.x, minCorner.y, minCorner.z, 0.0f);
		m_WorldData.BBDimension = floral::vec4f(
				maxCorner.x - minCorner.x,
				maxCorner.y - minCorner.y,
				maxCorner.z - minCorner.z,
				0.0f) / stepCount;

		m_SHPositions.reserve(64, m_ResourceArena);
		for (size yy = 0; yy < 4; yy++)
		{
			for (size zz = 0; zz < 4; zz++)
			{
				for (size xx = 0; xx < 4; xx++)
				{
#if 0
					floral::vec3f shPos(
							m_WorldData.BBMinCorner.x + xx * m_WorldData.BBDimension.x,
							m_WorldData.BBMinCorner.y + yy * m_WorldData.BBDimension.y,
							m_WorldData.BBMinCorner.z + zz * m_WorldData.BBDimension.z);
#else
					floral::vec3f shPos = bakeMinCorner + floral::vec3f(xx, yy, zz) * bakeDimension;
#endif
					m_SHPositions.push_back(shPos);
				}
			}
		}
	}

	{
		insigne::vbdesc_t desc;
		desc.region_size = SIZE_KB(32);
		desc.stride = sizeof(VertexPNC);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::static_draw;

		m_VB = insigne::create_vb(desc);
		insigne::copy_update_vb(m_VB, &m_VerticesData[0], m_VerticesData.get_size(), sizeof(VertexPNC), 0);
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
		// ico sphere
		floral::fixed_array<VertexP, FreelistArena> sphereVertices;
		floral::fixed_array<s32, FreelistArena> sphereIndices;

		sphereVertices.reserve(4096, m_TemporalArena);
		sphereIndices.reserve(8192, m_TemporalArena);

		sphereVertices.resize(4096);
		sphereIndices.resize(8192);

		floral::reset_generation_transforms_stack();
		floral::geo_generate_result_t genResult = floral::generate_unit_icosphere_3d(
				0, sizeof(VertexP),
				(s32)floral::geo_vertex_format_e::position,
				&sphereVertices[0], &sphereIndices[0]);
		sphereVertices.resize(genResult.vertices_generated);
		sphereIndices.resize(genResult.indices_generated);

		{
			insigne::vbdesc_t desc;
			desc.region_size = SIZE_KB(32);
			desc.stride = sizeof(VertexP);
			desc.data = nullptr;
			desc.count = 0;
			desc.usage = insigne::buffer_usage_e::static_draw;

			m_ProbeVB = insigne::create_vb(desc);
			insigne::copy_update_vb(m_ProbeVB, &sphereVertices[0], sphereVertices.get_size(), sizeof(VertexP), 0);
		}

		{
			insigne::ibdesc_t desc;
			desc.region_size = SIZE_KB(16);
			desc.data = nullptr;
			desc.count = 0;
			desc.usage = insigne::buffer_usage_e::static_draw;

			m_ProbeIB = insigne::create_ib(desc);
			insigne::copy_update_ib(m_ProbeIB, &sphereIndices[0], sphereIndices.get_size(), 0);
		}
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

	{
		insigne::ubdesc_t desc;
		desc.region_size = 512;
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		m_AlbedoUB = insigne::create_ub(desc);

		desc.region_size = SIZE_KB(32);
		m_ProbeUB = insigne::create_ub(desc);
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

	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Scene", insigne::param_data_type_e::param_ub));

		strcpy(desc.vs, s_AlbedoVS);
		strcpy(desc.fs, s_AlbedoFS);
		desc.vs_path = floral::path("/demo/albedo_vs");
		desc.fs_path = floral::path("/demo/albedo_fs");

		m_AlbedoShader = insigne::create_shader(desc);
		insigne::infuse_material(m_AlbedoShader, m_AlbedoMaterial);

		{
			s32 ubSlot = insigne::get_material_uniform_block_slot(m_AlbedoMaterial, "ub_Scene");
			m_AlbedoMaterial.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 0, 256, m_AlbedoUB };
		}
	}

	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Scene", insigne::param_data_type_e::param_ub));
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_ProbeData", insigne::param_data_type_e::param_ub));

		strcpy(desc.vs, s_ProbeVS);
		strcpy(desc.fs, s_ProbeFS);
		desc.vs_path = floral::path("/demo/probe_vs");
		desc.fs_path = floral::path("/demo/probe_fs");

		m_ProbeShader = insigne::create_shader(desc);
		insigne::infuse_material(m_ProbeShader, m_ProbeMaterial);

		{
			s32 ubSlot = insigne::get_material_uniform_block_slot(m_ProbeMaterial, "ub_Scene");
			m_ProbeMaterial.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 0, 256, m_UB };
		}
		{
			s32 ubSlot = insigne::get_material_uniform_block_slot(m_ProbeMaterial, "ub_ProbeData");
			m_ProbeMaterial.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 0, 256, m_ProbeUB };
		}
	}

	// PBR
	{
		calyx::context_attribs* commonCtx = calyx::get_context_attribs();

		insigne::framebuffer_desc_t desc = insigne::create_framebuffer_desc();
		desc.color_attachments->push_back(insigne::color_attachment_t("main_color", insigne::texture_format_e::hdr_rgb));

		desc.width = commonCtx->window_width;
		desc.height = commonCtx->window_height;

		m_PostFXBuffer = insigne::create_framebuffer(desc);
	}

	floral::inplace_array<VertexPT, 4> ssVertices;
	ssVertices.push_back(VertexPT { { -1.0f, -1.0f }, { 0.0f, 0.0f } });
	ssVertices.push_back(VertexPT { { 1.0f, -1.0f }, { 1.0f, 0.0f } });
	ssVertices.push_back(VertexPT { { 1.0f, 1.0f }, { 1.0f, 1.0f } });
	ssVertices.push_back(VertexPT { { -1.0f, 1.0f }, { 0.0f, 1.0f } });

	floral::inplace_array<s32, 6> ssIndices;
	ssIndices.push_back(0);
	ssIndices.push_back(1);
	ssIndices.push_back(2);
	ssIndices.push_back(2);
	ssIndices.push_back(3);
	ssIndices.push_back(0);

	{
		insigne::vbdesc_t desc;
		desc.region_size = SIZE_KB(2);
		desc.stride = sizeof(VertexPT);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::static_draw;
		
		m_SSVB = insigne::create_vb(desc);
		insigne::copy_update_vb(m_SSVB, &ssVertices[0], ssVertices.get_size(), sizeof(VertexPT), 0);
	}

	{
		insigne::ibdesc_t desc;
		desc.region_size = SIZE_KB(1);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::static_draw;

		m_SSIB = insigne::create_ib(desc);
		insigne::copy_update_ib(m_SSIB, &ssIndices[0], ssIndices.get_size(), 0);
	}

	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.textures->push_back(insigne::shader_param_t("u_Tex", insigne::param_data_type_e::param_sampler2d));

		strcpy(desc.vs, s_ToneMapVS);
		strcpy(desc.fs, s_ToneMapFS);
		desc.vs_path = floral::path("/demo/tonemap_vs");
		desc.fs_path = floral::path("/demo/tonemap_fs");

		m_ToneMapShader = insigne::create_shader(desc);
		insigne::infuse_material(m_ToneMapShader, m_ToneMapMaterial);

		{
			ssize texSlot = insigne::get_material_texture_slot(m_ToneMapMaterial, "u_Tex");
			m_ToneMapMaterial.textures[texSlot].value = insigne::extract_color_attachment(m_PostFXBuffer, 0);
		}
	}

	m_SHBaker.Initialize();
}

void LightProbeGI::OnUpdate(const f32 i_deltaMs)
{
	// DebugUI
	ImGui::Begin("Controller");
	if (ImGui::Button("Bake SH"))
	{
		if (!m_SHBakingStarted)
		{
			m_SHBakingStarted = true;
			m_SHReady = false;
			floral::simple_callback<void, const floral::mat4x4f&> renderCb;
			renderCb.bind<LightProbeGI, &LightProbeGI::_RenderSceneAlbedo>(this);
			m_SHBaker.StartSHBaking(m_SHPositions, m_SHData, renderCb);
		}
	}
	ImGui::Checkbox("Draw SH", &m_DrawSHProbes);
	ImGui::End();

	// Logic update
	m_CameraMotion.OnUpdate(i_deltaMs);
	m_SceneData.WVP = m_CameraMotion.GetWVP();
}

void LightProbeGI::OnRender(const f32 i_deltaMs)
{
	if (m_SHBakingStarted)
	{
		if (m_SHBaker.FrameUpdate())
		{
			CLOVER_VERBOSE("SH Baking finished");
			m_SHReady = true;
			m_SHBakingStarted = false;

			p8* shProbeData = (p8*)m_TemporalArena->allocate(SIZE_KB(32));
			p8* shData = shProbeData;
			memset(shProbeData, 0, SIZE_KB(32));
			for (size i = 0; i < m_SHPositions.get_size(); i++)
			{
				ProbeData d;
				d.XForm = floral::construct_translation3d(m_SHPositions[i]) * floral::construct_scaling3d(floral::vec3f(0.05f));
				SHCoeffs& coeffs = m_SHData->Probes[i];
				memcpy(d.CoEffs, coeffs.CoEffs, sizeof(SHCoeffs));
				memcpy(shData, &d, sizeof(ProbeData));
				shData = (p8*)((aptr)shData + 256);
			}
			insigne::copy_update_ub(m_ProbeUB, &shProbeData[0], 64 * 256, 0);
			insigne::copy_update_ub(m_SHUB, m_SHData, sizeof(SHData), 0);
			m_TemporalArena->free_all();
		}
	}

	insigne::begin_render_pass(m_PostFXBuffer);
	insigne::draw_surface<SurfacePNC>(m_VB, m_IB, m_Material);

	if (m_DrawSHProbes && m_SHReady)
	{
		for (size i = 0; i < m_SHPositions.get_size(); i++)
		{
			u32 ubSlot = insigne::get_material_uniform_block_slot(m_ProbeMaterial, "ub_ProbeData");
			m_ProbeMaterial.uniform_blocks[ubSlot].value.offset = 256 * i;
			insigne::draw_surface<SurfaceP>(m_ProbeVB, m_ProbeIB, m_ProbeMaterial);
		}
	}
	debugdraw::Render(m_SceneData.WVP);
	insigne::end_render_pass(m_PostFXBuffer);
	insigne::dispatch_render_pass();

	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::draw_surface<SurfacePT>(m_SSVB, m_SSIB, m_ToneMapMaterial);
	RenderImGui();
	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void LightProbeGI::OnCleanUp()
{
	CLOVER_VERBOSE("Cleaning up '%s' TestSuite", k_SuiteName);

	m_SHBaker.CleanUp();

	g_StreammingAllocator.free(m_TemporalArena);
	m_TemporalArena = nullptr;
	g_PersistanceResourceAllocator.free(m_ResourceArena);
	m_ResourceArena = nullptr;

	insigne::unregister_surface_type<SurfacePT>();
	insigne::unregister_surface_type<SurfaceP>();
	insigne::unregister_surface_type<SurfacePNC>();

	insigne::cleanup_render_resource(m_RenderBeginStateId);
	insigne::cleanup_textures_resource(m_TextureBeginStateId);
	insigne::cleanup_shading_resource(m_ShadingBeginStateId);
	insigne::cleanup_buffers_resource(m_BuffersBeginStateId);

	insigne::dispatch_render_pass();
	insigne::wait_finish_dispatching();
}

//----------------------------------------------

void LightProbeGI::_RenderSceneAlbedo(const floral::mat4x4f& i_wvp)
{
	insigne::copy_update_ub(m_AlbedoUB, (const voidptr)&i_wvp, sizeof(floral::mat4x4f), 0);

	insigne::draw_surface<SurfacePNC>(m_VB, m_IB, m_AlbedoMaterial);
}

}
}
