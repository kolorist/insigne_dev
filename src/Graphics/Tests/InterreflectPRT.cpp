#include "InterreflectPRT.h"

#include <floral/comgeo/shapegen.h>

#include <clover/Logger.h>
#include <clover/SinkTopic.h>

#include <insigne/commons.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_render.h>
#include <insigne/ut_textures.h>

#include "Graphics/prt.h"
#include "Graphics/CBTexDefinitions.h"

namespace stone
{

static const_cstr s_VertexShader = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in highp vec3 l_Normal_L;
layout (location = 2) in mediump vec4 l_Color;

layout (location = 3) in mediump vec3 l_SH0;
layout (location = 4) in mediump vec3 l_SH1;
layout (location = 5) in mediump vec3 l_SH2;
layout (location = 6) in mediump vec3 l_SH3;
layout (location = 7) in mediump vec3 l_SH4;
layout (location = 8) in mediump vec3 l_SH5;
layout (location = 9) in mediump vec3 l_SH6;
layout (location = 10) in mediump vec3 l_SH7;
layout (location = 11) in mediump vec3 l_SH8;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_XForm;
	highp mat4 iu_WVP;
};

layout(std140) uniform ub_Light
{
	mediump vec4 iu_LightSH[9];
};

out mediump vec4 v_VertexColor;

void main() {
	highp vec4 pos_W = iu_XForm * vec4(l_Position_L, 1.0f);

	mediump vec3 vColor = vec3(0.0f);
	vColor = l_SH0 * iu_LightSH[0].rgb;
	vColor += l_SH1 * iu_LightSH[1].rgb;
	vColor += l_SH2 * iu_LightSH[2].rgb;
	vColor += l_SH3 * iu_LightSH[3].rgb;
	vColor += l_SH4 * iu_LightSH[4].rgb;
	vColor += l_SH5 * iu_LightSH[5].rgb;
	vColor += l_SH6 * iu_LightSH[6].rgb;
	vColor += l_SH7 * iu_LightSH[7].rgb;
	vColor += l_SH8 * iu_LightSH[8].rgb;

	v_VertexColor = vec4(vColor, 0.0f);
	gl_Position = iu_WVP * pos_W;
}
)";

static const_cstr s_FragmentShader = R"(#version 300 es

layout (location = 0) out mediump vec4 o_Color;

in mediump vec4 v_VertexColor;

void main()
{
	o_Color = v_VertexColor;
}
)";

static const_cstr s_ToneMapVS = R"(#version 300 es
layout (location = 0) in highp vec2 l_Position;
layout (location = 1) in mediump vec2 l_TexCoord;

out mediump vec2 v_TexCoord;

void main()
{
	v_TexCoord = l_TexCoord;
	gl_Position = vec4(l_Position, 0.0f, 1.0f);
}
)";

static const_cstr s_ToneMapFS = R"(#version 300 es
layout (location = 0) out mediump vec4 o_Color;

uniform mediump sampler2D u_Tex;

in mediump vec2 v_TexCoord;

void main()
{
	mediump float exposure = 0.2f;
	mediump float gamma = 2.2f;
	mediump vec4 hdrColor = texture(u_Tex, v_TexCoord);

	mediump vec3 mapped = vec3(1.0f) - exp(-hdrColor.rgb * exposure);
	mapped = pow(mapped, vec3(1.0f / gamma));

	o_Color = vec4(mapped, hdrColor.a);
}
)";

InterreflectPRT::InterreflectPRT()
	: m_CameraMotion(
			floral::camera_view_t { floral::vec3f(3.0f, 3.0f, 0.0f), floral::vec3f(-3.0f, -3.0f, 0.0f), floral::vec3f(0.0f, 1.0f, 0.0f) },
			floral::camera_persp_t { 0.01f, 100.0f, 60.0f, 16.0f / 9.0f })
{
	m_MemoryArena = g_PersistanceResourceAllocator.allocate_arena<LinearArena>(SIZE_MB(16));
}

InterreflectPRT::~InterreflectPRT()
{
}

void InterreflectPRT::OnInitialize()
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

	ComputeLightSH();

	{
		insigne::ubdesc_t desc;
		desc.region_size = SIZE_KB(4);
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ub_handle_t newUB = insigne::create_ub(desc);

		insigne::update_ub(newUB, &m_SceneLight, sizeof(SceneLight), 0);
		m_LightUB = newUB;
	}

	{
		m_Vertices.reserve(4096, &g_StreammingAllocator);
		m_Indices.reserve(8192, &g_StreammingAllocator);
		m_MnfVertices.reserve(4096, &g_StreammingAllocator);
		m_MnfIndices.reserve(8192, &g_StreammingAllocator);

		m_Vertices.resize(4096);
		m_Indices.resize(8192);
		m_MnfVertices.resize(4096);
		m_MnfIndices.resize(8192);

		u32 vtxCount = 0, idxCount = 0;
		u32 mnfVtxCount = 0, mnfIdxCount = 0;

		// bottom
		{
			floral::reset_generation_transforms_stack();
			floral::manifold_geo_generate_result_t genResult = floral::generate_manifold_quadtes_unit_plane_3d(
					vtxCount, sizeof(VertexPNCSH),
					floral::geo_vertex_format_e::position | floral::geo_vertex_format_e::normal, 0.1f,
					&m_Vertices[vtxCount], &m_Indices[idxCount],

					mnfVtxCount, sizeof(VertexP), 0.05f,
					(s32)floral::geo_vertex_format_e::position,
					&m_MnfVertices[mnfVtxCount], &m_MnfIndices[mnfIdxCount]);

			for (size i = vtxCount; i < vtxCount + genResult.vertices_generated; i++)
			{
				m_Vertices[i].Color = floral::vec4f(1.0f);
			}

			vtxCount += genResult.vertices_generated;
			idxCount += genResult.indices_generated;
			mnfVtxCount += genResult.manifold_vertices_generated;
			mnfIdxCount += genResult.manifold_indices_generated;

		}
		// right
		{
			floral::reset_generation_transforms_stack();
			floral::quaternionf q = floral::construct_quaternion_euler(90.0f, 0.0f, 0.0f);
			floral::push_generation_transform(q.to_transform());
			floral::push_generation_transform(floral::construct_translation3d(0.0f, 1.0f, -1.0f));

			floral::manifold_geo_generate_result_t genResult = floral::generate_manifold_quadtes_unit_plane_3d(
					vtxCount, sizeof(VertexPNCSH),
					floral::geo_vertex_format_e::position | floral::geo_vertex_format_e::normal, 0.2f,
					&m_Vertices[vtxCount], &m_Indices[idxCount],

					mnfVtxCount, sizeof(VertexP), 0.05f,
					(s32)floral::geo_vertex_format_e::position,
					&m_MnfVertices[mnfVtxCount], &m_MnfIndices[mnfIdxCount]);

			for (size i = vtxCount; i < vtxCount + genResult.vertices_generated; i++)
			{
				m_Vertices[i].Color = floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f);
			}

			vtxCount += genResult.vertices_generated;
			idxCount += genResult.indices_generated;
			mnfVtxCount += genResult.manifold_vertices_generated;
			mnfIdxCount += genResult.manifold_indices_generated;
		}
		// back
		{
			floral::reset_generation_transforms_stack();
			floral::quaternionf q = floral::construct_quaternion_euler(0.0f, 0.0f, -90.0f);
			floral::push_generation_transform(q.to_transform());
			floral::push_generation_transform(floral::construct_translation3d(-1.0f, 1.0f, 0.0f));

			floral::manifold_geo_generate_result_t genResult = floral::generate_manifold_quadtes_unit_plane_3d(
					vtxCount, sizeof(VertexPNCSH),
					floral::geo_vertex_format_e::position | floral::geo_vertex_format_e::normal, 0.2f,
					&m_Vertices[vtxCount], &m_Indices[idxCount],

					mnfVtxCount, sizeof(VertexP), 0.05f,
					(s32)floral::geo_vertex_format_e::position,
					&m_MnfVertices[mnfVtxCount], &m_MnfIndices[mnfIdxCount]);

			for (size i = vtxCount; i < vtxCount + genResult.vertices_generated; i++)
			{
				m_Vertices[i].Color = floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f);
			}

			vtxCount += genResult.vertices_generated;
			idxCount += genResult.indices_generated;
			mnfVtxCount += genResult.manifold_vertices_generated;
			mnfIdxCount += genResult.manifold_indices_generated;
		}
		// sphere
		{
			floral::reset_generation_transforms_stack();
			floral::push_generation_transform(floral::construct_scaling3d(0.4f, 0.4f, 0.4f));
			floral::push_generation_transform(floral::construct_translation3d(0.0f, 0.4f, -0.4f));

			floral::manifold_geo_generate_result_t genResult = floral::generate_manifold_icosphere_3d(
					vtxCount, sizeof(VertexPNCSH),
					floral::geo_vertex_format_e::position | floral::geo_vertex_format_e::normal,
					&m_Vertices[vtxCount], &m_Indices[idxCount],

					mnfVtxCount, sizeof(VertexP), 0.05f,
					(s32)floral::geo_vertex_format_e::position,
					&m_MnfVertices[mnfVtxCount], &m_MnfIndices[mnfIdxCount]);

			for (size i = vtxCount; i < vtxCount + genResult.vertices_generated; i++)
			{
				m_Vertices[i].Color = floral::vec4f(1.0f);
			}

			vtxCount += genResult.vertices_generated;
			idxCount += genResult.indices_generated;
			mnfVtxCount += genResult.manifold_vertices_generated;
			mnfIdxCount += genResult.manifold_indices_generated;
		}

		m_Vertices.resize(vtxCount);
		m_Indices.resize(idxCount);
		m_MnfVertices.resize(mnfVtxCount);
		m_MnfIndices.resize(mnfIdxCount);
	}

	ComputePRT();

	{
		insigne::vbdesc_t desc;
		desc.region_size = SIZE_KB(256);
		desc.stride = sizeof(VertexPNCSH);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::vb_handle_t newVB = insigne::create_vb(desc);
		insigne::update_vb(newVB, &m_Vertices[0], m_Vertices.get_size(), 0);
		m_VB = newVB;
	}

	{
		insigne::ibdesc_t desc;
		desc.region_size = SIZE_KB(128);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ib_handle_t newIB = insigne::create_ib(desc);
		insigne::update_ib(newIB, &m_Indices[0], m_Indices.get_size(), 0);
		m_IB = newIB;
	}

	// full screen quad
	{
		VertexPT vertices[4];
		vertices[0].Position = floral::vec2f(-1.0f, -1.0f);
		vertices[0].TexCoord = floral::vec2f(0.0f, 0.0f);

		vertices[1].Position = floral::vec2f(1.0f, -1.0f);
		vertices[1].TexCoord = floral::vec2f(1.0f, 0.0f);

		vertices[2].Position = floral::vec2f(1.0f, 1.0f);
		vertices[2].TexCoord = floral::vec2f(1.0f, 1.0f);

		vertices[3].Position = floral::vec2f(-1.0f, 1.0f);
		vertices[3].TexCoord = floral::vec2f(0.0f, 1.0f);

		insigne::vbdesc_t desc;
		desc.region_size = SIZE_KB(4);
		desc.stride = sizeof(VertexPT);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::vb_handle_t newVB = insigne::create_vb(desc);
		insigne::copy_update_vb(newVB, vertices, 4, sizeof(VertexPT), 0);
		m_SSVB = newVB;
	}

	{
		u32 indices[6];
		indices[0] = 0;
		indices[1] = 1;
		indices[2] = 2;
		indices[3] = 2;
		indices[4] = 3;
		indices[5] = 0;

		insigne::ibdesc_t desc;
		desc.region_size = SIZE_KB(4);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ib_handle_t newIB = insigne::create_ib(desc);
		insigne::copy_update_ib(newIB, indices, 6, 0);
		m_SSIB = newIB;
	}

	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Scene", insigne::param_data_type_e::param_ub));
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Light", insigne::param_data_type_e::param_ub));

		strcpy(desc.vs, s_VertexShader);
		strcpy(desc.fs, s_FragmentShader);
		desc.vs_path = floral::path("/scene/cornel_box_vs");
		desc.fs_path = floral::path("/scene/cornel_box_fs");

		m_Shader = insigne::create_shader(desc);
		insigne::infuse_material(m_Shader, m_Material);

		// static uniform data
		{
			s32 ubSlot = insigne::get_material_uniform_block_slot(m_Material, "ub_Scene");
			m_Material.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 0, 0, m_UB };
		}
		{
			s32 ubSlot = insigne::get_material_uniform_block_slot(m_Material, "ub_Light");
			m_Material.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 0, 0, m_LightUB };
		}
	}

	{
		// 1280x720
		insigne::framebuffer_desc_t desc = insigne::create_framebuffer_desc();
		desc.color_attachments->push_back(insigne::color_attachment_t("main_color", insigne::texture_format_e::hdr_rgba));
		desc.width = 1280; desc.height = 720;
		m_HDRBuffer = insigne::create_framebuffer(desc);
	}

	// tonemap shader
	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.textures->push_back(insigne::shader_param_t("u_Tex", insigne::param_data_type_e::param_sampler2d));

		strcpy(desc.vs, s_ToneMapVS);
		strcpy(desc.fs, s_ToneMapFS);
		desc.vs_path = floral::path("/scene/tonemap_vs");
		desc.fs_path = floral::path("/scene/tonemap_fs");

		m_ToneMapShader = insigne::create_shader(desc);
		insigne::infuse_material(m_ToneMapShader, m_ToneMapMaterial);

		// static uniform data
		{
			s32 texSlot = insigne::get_material_texture_slot(m_ToneMapMaterial, "u_Tex");
			m_ToneMapMaterial.textures[texSlot].value = insigne::extract_color_attachment(m_HDRBuffer, 0);
		}
	}

	m_DebugDrawer.Initialize();
}

void InterreflectPRT::OnUpdate(const f32 i_deltaMs)
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

void InterreflectPRT::OnDebugUIUpdate(const f32 i_deltaMs)
{
	ImGui::Begin("Interreflect PRT");
	ImGui::Text("grace_probe and cornell box");
	ImGui::End();
}

void InterreflectPRT::OnRender(const f32 i_deltaMs)
{
	// camera
	m_SceneData.WVP = m_CameraMotion.GetWVP();
	insigne::update_ub(m_UB, &m_SceneData, sizeof(SceneData), 0);

	insigne::begin_render_pass(m_HDRBuffer);
	insigne::draw_surface<SurfacePNCSH>(m_VB, m_IB, m_Material);
	m_DebugDrawer.Render(m_SceneData.WVP);
	insigne::end_render_pass(m_HDRBuffer);
	insigne::dispatch_render_pass();

	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::draw_surface<SurfacePT>(m_SSVB, m_SSIB, m_ToneMapMaterial);
	IDebugUI::OnFrameRender(i_deltaMs);
	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void InterreflectPRT::OnCleanUp()
{
}

//----------------------------------------------
void InterreflectPRT::ComputeLightSH()
{
	m_MemoryArena->free_all();
	//floral::file_info texFile = floral::open_file("gfx/envi/textures/demo/grace_probe.cbtex");
	//floral::file_info texFile = floral::open_file("gfx/envi/textures/demo/Alexs_Apt_2k_lightprobe.cbtex");
	floral::file_info texFile = floral::open_file("gfx/envi/textures/demo/ArboretumInBloom_lightprobe.cbtex");
	//floral::file_info texFile = floral::open_file("gfx/envi/textures/demo/CharlesRiver_lightprobe.cbtex");
	//floral::file_info texFile = floral::open_file("gfx/envi/textures/demo/uffizi_probe.cbtex");
	floral::file_stream dataStream;
	dataStream.buffer = (p8)m_MemoryArena->allocate(texFile.file_size);
	floral::read_all_file(texFile, dataStream);
	floral::close_file(texFile);

	cb::CBTexture2DHeader header;
	dataStream.read(&header);

	FLORAL_ASSERT(header.colorRange == cb::ColorRange::HDR);
	u32 dataSize = header.resolution * header.resolution * 3 * sizeof(f32);

	f32* texData = (f32*)m_MemoryArena->allocate(dataSize);
	dataStream.read_bytes(texData, dataSize);

	s32 sqrtNSamples = 100;
	s32 NSamples = sqrtNSamples * sqrtNSamples;
	sh_sample* samples = m_MemoryArena->allocate_array<sh_sample>(NSamples);
	sh_setup_spherical_samples(samples, sqrtNSamples);

	highp_vec3_t shResult[9];
	sh_project_light_image(texData, header.resolution, NSamples, 9, samples, shResult);

	for (u32 i = 0; i < 9; i++)
	{
		//shResult[i] = highp_vec3_t(1.0);
		CLOVER_DEBUG("(%f; %f; %f)", shResult[i].x, shResult[i].y, shResult[i].z);
		m_SceneLight.LightSH[i] = floral::vec4f((f32)shResult[i].x, (f32)shResult[i].y, (f32)shResult[i].z, 0.0f);
	}
}

void InterreflectPRT::ComputePRT()
{
	LOG_TOPIC("prt");

	s32 sqrtNSamples = 10;
	s32 NSamples = sqrtNSamples * sqrtNSamples;
	sh_sample* samples = m_MemoryArena->allocate_array<sh_sample>(NSamples);
	sh_setup_spherical_samples(samples, sqrtNSamples);

	m_MemoryArena->free_all();
	bool *hitSelf = m_MemoryArena->allocate_array<bool>(NSamples * m_Vertices.get_size());
	memset(hitSelf, 0, NSamples * m_Vertices.get_size() * sizeof(bool));

	highp_vec3_t coeffs[9];

	for (size i = 0; i < m_Vertices.get_size(); i++)
	{
		CLOVER_VERBOSE("Tracing vertex #%d with %d rays...", i, NSamples);
		memset(coeffs, 0, sizeof(coeffs));
		u32 upperRays = 0;
		u32 shadowedRays = 0;
		u32 selfHitRays = 0;

		for (s32 j = 0; j < NSamples; j++)
		{
			hitSelf[i * NSamples + j] = false;
			const sh_sample& sample = samples[j];

			highp_vec3_t normal = highp_vec3_t(m_Vertices[i].Normal.x, m_Vertices[i].Normal.y, m_Vertices[i].Normal.z);
			f64 cosineTerm = floral::dot(normal, sample.vec);
			if (cosineTerm > 0.0f) // upper hemisphere
			{
				upperRays++;
				floral::ray3df ray;
				ray.o = m_Vertices[i].Position;
				ray.d = floral::vec3f((f32)sample.vec.x, (f32)sample.vec.y, (f32)sample.vec.z);
				bool rayHitGeometry = false;

				for (size idx = 0; idx < m_MnfIndices.get_size(); idx += 3)
				{
					u32 tIdx0 = m_MnfIndices[idx];
					u32 tIdx1 = m_MnfIndices[idx + 1];
					u32 tIdx2 = m_MnfIndices[idx + 2];

					float t = 0.0f;
					const bool isHit = ray_triangle_intersect(
							ray,
							m_MnfVertices[tIdx0].Position,
							m_MnfVertices[tIdx1].Position,
							m_MnfVertices[tIdx2].Position,
							&t);

					if (isHit && t > 0.001f)
					{
						shadowedRays++;
						rayHitGeometry = true;
						break;
					}
				}

				if (!rayHitGeometry)
				{
					const floral::vec4f& vtxColor = m_Vertices[i].Color;
					highp_vec3_t color(vtxColor.x, vtxColor.y, vtxColor.z); // highp computation
					for (u32 k = 0; k < 9; k++)
					{
						f64 shCoeff = sample.coeff[k];
						coeffs[k] += color * shCoeff * cosineTerm;
					}
				}
				else
				{
					for (size idx = 0; idx < m_Indices.get_size(); idx += 3)
					{
						u32 tIdx0 = m_Indices[idx];
						u32 tIdx1 = m_Indices[idx + 1];
						u32 tIdx2 = m_Indices[idx + 2];

						if (tIdx0 == i || tIdx1 == i || tIdx2 == i) continue;

						float t = 0.0f;
						const bool isHit = ray_triangle_intersect(
								ray,
								m_Vertices[tIdx0].Position,
								m_Vertices[tIdx1].Position,
								m_Vertices[tIdx2].Position,
								&t);

						if (isHit && t > 0.001f)
						{
							selfHitRays++;
							hitSelf[i * NSamples + j] = true;
							break;
						}
					}
				}
			}
		}

		CLOVER_VERBOSE("Tracing vertex #%d finished: %d upper rays - %d shadowed rays - %d self hit rays",
				i, upperRays, shadowedRays, selfHitRays);

		f64 weight = 4.0 * 3.141593;
		f64 scale = weight / NSamples;
		VertexPNCSH& vtf = m_Vertices[i];
		for (u32 j = 0; j < 9; j++)
		{
			coeffs[j] *= scale;
			vtf.SH[j] = floral::vec3f((f32)coeffs[j].x, (f32)coeffs[j].y, (f32)coeffs[j].z);
		}
	}

	CLOVER_VERBOSE("Begin interreflect phase");

	// interrefect pass
	highp_vec3_t* shBuffer[2];

	shBuffer[0] = m_MemoryArena->allocate_array<highp_vec3_t>(9 * m_Vertices.get_size());
	shBuffer[1] = m_MemoryArena->allocate_array<highp_vec3_t>(9 * m_Vertices.get_size());
	for (size i = 0; i < m_Vertices.get_size(); i++)
	{
		for (size k = 0; k < 9; k++)
		{
			floral::vec3f lowpSH = m_Vertices[i].SH[k];
			shBuffer[0][9 * i + k] = highp_vec3_t(lowpSH.x, lowpSH.y, lowpSH.z);
			shBuffer[1][9 * i + k] = highp_vec3_t(0.0f);
		}
	}

	for (size i = 0; i < m_Vertices.get_size(); i++)
	{
		CLOVER_VERBOSE("Interreflecting vertex #%d...", i);
		floral::vec4f vtxColor = m_Vertices[i].Color;
		highp_vec3_t color(vtxColor.x, vtxColor.y, vtxColor.z);

		for (size j = 0; j < NSamples; j++)
		{
			if (hitSelf[i * NSamples + j])
			{
				const sh_sample& sample = samples[j];

				highp_vec3_t normal = highp_vec3_t(m_Vertices[i].Normal.x, m_Vertices[i].Normal.y, m_Vertices[i].Normal.z);
				f64 cosineTerm = floral::dot(normal, sample.vec);
				if (cosineTerm > 0.0f)
				{
					floral::ray3df ray;
					ray.o = m_Vertices[i].Position;
					ray.d = floral::vec3f((f32)sample.vec.x, (f32)sample.vec.y, (f32)sample.vec.z);

					f64 b0 = 0.0f, b1 = 0.0f, b2 = 0.0f;
					u32 ix0 = 0, ix1 = 0, ix2 = 0;
					bool rayHit = false;
					float t = 9999.0f;
					for (size idx = 0; idx < m_Indices.get_size(); idx+=3)
					{
						u32 tIdx0 = m_Indices[idx];
						u32 tIdx1 = m_Indices[idx + 1];
						u32 tIdx2 = m_Indices[idx + 2];

						float tt = 0.0f, bb0 = 0.0f, bb1 = 0.0f;
						const bool isHit = ray_triangle_intersect(
								ray,
								m_Vertices[tIdx0].Position,
								m_Vertices[tIdx1].Position,
								m_Vertices[tIdx2].Position,
								&tt, &bb0, &bb1);

						if (isHit && tt > 0.001f & tt < t)
						{
							t = tt;
							b0 = bb0; b1 = bb1;
							b2 = 1.0f - b0 - b1;
							ix0 = tIdx0; ix1 = tIdx1; ix2 = tIdx2;
							rayHit = true;
						}
					}

					if (rayHit)
					{
						FLORAL_ASSERT(b0 > 0.0 && b1 > 0.0 && b2 > 0.0);
						highp_vec3_t sh[9];
						for (size k = 0; k < 9; k++)
						{
							highp_vec3_t sh0(m_Vertices[ix0].SH[k].x, m_Vertices[ix0].SH[k].y, m_Vertices[ix0].SH[k].z);
							highp_vec3_t sh1(m_Vertices[ix1].SH[k].x, m_Vertices[ix1].SH[k].y, m_Vertices[ix1].SH[k].z);
							highp_vec3_t sh2(m_Vertices[ix2].SH[k].x, m_Vertices[ix2].SH[k].y, m_Vertices[ix2].SH[k].z);

							sh[k] =	b0 * sh0 + b1 * sh1 + b2 * sh2;
						}

						for (size k = 0; k < 9; k++)
						{
							shBuffer[1][9 * i + k] += color * cosineTerm * sh[k];
						}
					}
				} // hemisphere test
			} // hitSelft
		} // for all samples

		f64 weight = 4.0 * 3.141593;
		f64 scale = weight / NSamples;
		for (size k = 0; k < 9; k++)
		{
			shBuffer[1][9 * i + k] *= scale;
		}
	} // each vertex

	CLOVER_VERBOSE("Summing all bounces...");
	// sum all bounces
	{
		for (size i = 0; i < m_Vertices.get_size(); i++)
		{
			for (size k = 0; k < 9; k++)
			{
				highp_vec3_t sh = shBuffer[0][9 * i + k] + shBuffer[1][9 * i + k];
				m_Vertices[i].SH[k] = floral::vec3f(sh.x, sh.y, sh.z);
			}
		}
	}
}

}
