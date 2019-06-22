#include "FormFactorsValidating.h"

#include <floral/comgeo/shapegen.h>

#include <insigne/commons.h>
#include <insigne/counters.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_render.h>
#include <clover.h>

#include <stdlib.h>
#include <time.h>
#include <random>

namespace stone {

static const_cstr s_VertexShader = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in highp vec3 l_Normal_L;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_XForm;
	highp mat4 iu_WVP;
};

void main() {
	highp vec4 pos_W = iu_XForm * vec4(l_Position_L, 1.0f);
	gl_Position = iu_WVP * pos_W;
}
)";

static const_cstr s_FragmentShader = R"(#version 300 es
layout (location = 0) out mediump vec4 o_Color;

void main()
{
	o_Color = vec4(1.0f);
}
)";

FormFactorsValidating::FormFactorsValidating()
	: m_CameraMotion(
			floral::camera_view_t { floral::vec3f(0.0f, 0.3f, 3.0f), floral::vec3f(0.0f, -0.3f, -3.0f), floral::vec3f(0.0f, 1.0f, 0.0f) },
			floral::camera_persp_t { 0.01f, 100.0f, 60.0f, 16.0f / 9.0f })
{
	srand(time(0));
	m_MemoryArena = g_PersistanceResourceAllocator.allocate_arena<LinearArena>(SIZE_MB(16));
}

FormFactorsValidating::~FormFactorsValidating()
{
}

void FormFactorsValidating::OnInitialize()
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
		insigne::vbdesc_t desc;
		desc.region_size = SIZE_KB(256);
		desc.stride = sizeof(VertexPNC);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::vb_handle_t newVB = insigne::create_vb(desc);
		m_VB = newVB;
	}

	{
		insigne::ibdesc_t desc;
		desc.region_size = SIZE_KB(128);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ib_handle_t newIB = insigne::create_ib(desc);
		m_IB = newIB;
	}

	// parallel test
	{
		m_MemoryArena->free_all();

		floral::fixed_array<VertexPNC, LinearAllocator> vertices;
		floral::fixed_array<s32, LinearAllocator> indices;
		vertices.reserve(32, m_MemoryArena); vertices.resize(32);
		indices.reserve(64, m_MemoryArena); indices.resize(64);

		size vtxCount = 0, idxCount = 0;

		// bottom
		floral::reset_generation_transforms_stack();
		floral::push_generation_transform(floral::construct_scaling3d(0.5f, 0.5f, 0.5f));
		floral::geo_generate_result_t bottomGen = floral::generate_unit_plane_3d(
				vtxCount, sizeof(VertexPNC),
				floral::geo_vertex_format_e::position | floral::geo_vertex_format_e::normal,
				&vertices[vtxCount], &indices[idxCount]);

		vtxCount += bottomGen.vertices_generated;
		idxCount += bottomGen.indices_generated;

#if 0
		floral::quaternionf q = floral::construct_quaternion_euler(180.0f, 0.0f, 0.0f);
		floral::push_generation_transform(q.to_transform());
		floral::push_generation_transform(floral::construct_translation3d(0.0f, 1.0f, 0.0f));
#else
		floral::reset_generation_transforms_stack();
		floral::quaternionf q = floral::construct_quaternion_euler(0.0f, 0.0f, 90.0f);
		floral::push_generation_transform(q.to_transform());
		floral::push_generation_transform(floral::construct_scaling3d(0.5f, 0.5f, 0.5f));
		floral::push_generation_transform(floral::construct_translation3d(-0.5f, 0.5f, 0.0f));
#endif
		floral::geo_generate_result_t topGen = floral::generate_unit_plane_3d(
				vtxCount, sizeof(VertexPNC),
				floral::geo_vertex_format_e::position | floral::geo_vertex_format_e::normal,
				&vertices[vtxCount], &indices[idxCount]);

		vtxCount += topGen.vertices_generated;
		idxCount += topGen.indices_generated;

		vertices.resize(vtxCount);
		indices.resize(idxCount);

		insigne::copy_update_vb(m_VB, &vertices[0], vertices.get_size(), sizeof(VertexPNC), 0);
		insigne::copy_update_ib(m_IB, &indices[0], indices.get_size(), 0);

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<f32> dis(0.0f, 1.0f);
		// generate sample for 2 patches (bottom)	
		const s32 stratifiedFactor = 16;
		const f32 gridSize = 1.0f / stratifiedFactor;

		m_Patch1Samples.reserve(stratifiedFactor * stratifiedFactor, &g_StreammingAllocator);
		m_Patch2Samples.reserve(stratifiedFactor * stratifiedFactor, &g_StreammingAllocator);
		m_Rays.reserve(stratifiedFactor * stratifiedFactor, &g_StreammingAllocator);

		if (0)
		{
			for (s32 i = 0; i < stratifiedFactor; i++)
			{
				for (s32 j = 0; j < stratifiedFactor; j++)
				{
					f32 minX = -0.5f + gridSize * i;
					f32 maxX = -0.5f + gridSize * (i + 1);
					f32 minZ = -0.5f + gridSize * j;
					f32 maxZ = -0.5f + gridSize * (j + 1);

					f32 x = minX + (maxX - minX) * dis(gen);
					f32 z = minZ + (maxZ - minZ) * dis(gen);
					m_Patch1Samples.push_back(floral::vec3f(x, 0.0f, z));
					x = minX + (maxX - minX) * dis(gen);
					z = minZ + (maxZ - minZ) * dis(gen);
					m_Patch2Samples.push_back(floral::vec3f(x, 1.0f, z));
				}
			}

			for (size i = 0; i < stratifiedFactor * stratifiedFactor; i++)
			{
				m_Rays.push_back(i);
			}
		}
		else
		{
			for (s32 i = 0; i < stratifiedFactor; i++)
			{
				for (s32 j = 0; j < stratifiedFactor; j++)
				{
					f32 minX = -0.5f + gridSize * i;
					f32 maxX = -0.5f + gridSize * (i + 1);
					f32 minY = gridSize * i;
					f32 maxY = gridSize * (i + 1);
					f32 minZ = -0.5f + gridSize * j;
					f32 maxZ = -0.5f + gridSize * (j + 1);

					f32 x = minX + (maxX - minX) * dis(gen);
					f32 z = minZ + (maxZ - minZ) * dis(gen);
					m_Patch1Samples.push_back(floral::vec3f(x, 0.0f, z));
					f32 y = minY + (maxY - minY) * dis(gen);
					z = minZ + (maxZ - minZ) * dis(gen);
					m_Patch2Samples.push_back(floral::vec3f(-0.5f, y, z));
				}
			}

			for (size i = 0; i < stratifiedFactor * stratifiedFactor; i++)
			{
				m_Rays.push_back(i);
			}
		}

		const floral::vec3f ni(0.0f, 1.0f, 0.0f);
		const floral::vec3f nj(0.0f, -1.0f, 0.0f);
#define METHOD 3
#if (METHOD == 1)
		const s32 raysCount = stratifiedFactor * stratifiedFactor;
		for (s32 k = 0; k < 30; k++)
		{
			std::uniform_int_distribution<size> disInt(0, stratifiedFactor * stratifiedFactor - 1);
			for (size i = 0; i < stratifiedFactor * stratifiedFactor; i++)
			{
				std::swap(m_Rays[i], m_Rays[disInt(gen)]);
			}

			f32 fij = 0.0f;
			for (size i = 0; i < m_Rays.get_size(); i++)
			{
				floral::vec3f org(m_Patch1Samples[i]);			// i
				floral::vec3f dst(m_Patch2Samples[m_Rays[i]]);	// j
				floral::vec3f rij(dst - org);

				f32 len = floral::length(rij);
				f32 cosThetaJ = floral::dot(floral::normalize(rij), ni);
				f32 cosThetaI = floral::dot(-floral::normalize(rij), nj);

				f32 deltaF = cosThetaI * cosThetaJ / (3.141594f * len * len + 1.0f / raysCount);
				if (deltaF > 0.0f)
				{
					fij += deltaF;
				}
			}
			fij /= raysCount;
			CLOVER_DEBUG("fij = %f", fij);
		}
#elif (METHOD == 2)
		f32 fij = 0.0f;
		const s32 raysCount = m_Patch1Samples.get_size() * m_Patch2Samples.get_size();
		for (size i = 0; i < m_Patch1Samples.get_size(); i++)
		{
			floral::vec3f org(m_Patch1Samples[i]);
			for (size j = 0; j < m_Patch2Samples.get_size(); j++)
			{
				floral::vec3f dst(m_Patch2Samples[j]);
				floral::vec3f rij(dst - org);

				f32 len = floral::length(rij);
				f32 cosThetaJ = floral::dot(floral::normalize(rij), ni);
				f32 cosThetaI = floral::dot(floral::normalize(rij), nj);

				f32 deltaF = cosThetaI * cosThetaJ / (3.141594f * len * len + 1.0f / raysCount);
				if (fabs(deltaF) > 0.0f)
				{
					fij += fabs(deltaF);
				}
			}
		}
		fij /= raysCount;
		CLOVER_DEBUG("fij = %f", fij);
#elif (METHOD == 3)
		f64 fij = 0.0;
		for (size i = 0; i < m_Patch1Samples.get_size(); i++)
		{
			floral::vec3f org(m_Patch1Samples[i]);
			f64 df = 0.0;
			f64 dg = 0.0;
			for (size j = 0; j < m_Patch2Samples.get_size(); j++)
			{
				floral::vec3f dst(m_Patch2Samples[j]);
				floral::vec3f rij(dst - org);

				f32 len = floral::length(rij);
				f32 cosThetaJ = floral::dot(floral::normalize(rij), ni);
				f32 cosThetaI = -floral::dot(floral::normalize(rij), nj);

				f64 gVis = cosThetaJ * cosThetaI / (3.141592 * len * len);

				df += gVis;
				dg += gVis;
			}
			f64 lambertG = 0.0;
			{
				const f64 one2Pi = 1.0 / (2.0 * 3.141592);
#if 0
				floral::vec3f p2Vertices[] = {
					floral::vec3f(0.5f, 1.0f, 0.5f),
					floral::vec3f(0.5f, 1.0f, -0.5f),
					floral::vec3f(-0.5f, 1.0f, -0.5f),
					floral::vec3f(-0.5f, 1.0f, 0.5f)
				};
#else
				floral::vec3f p2Vertices[] = {
					floral::vec3f(-0.5f, 0.0f, 0.5f),
					floral::vec3f(-0.5f, 0.0f, -0.5f),
					floral::vec3f(-0.5f, 1.0f, -0.5f),
					floral::vec3f(-0.5f, 1.0f, 0.5f)
				};
#endif
				floral::vec3f rs[4];
				f32 gammas[4];
				for (size i = 0; i < 4; i++)
				{
					rs[i] = floral::normalize(p2Vertices[i] - org);
				}

				gammas[0] = acosf(floral::dot(rs[0], rs[1]));
				gammas[1] = acosf(floral::dot(rs[1], rs[2]));
				gammas[2] = acosf(floral::dot(rs[2], rs[3]));
				gammas[3] = acosf(floral::dot(rs[3], rs[0]));

				floral::vec3f vs[4];
				vs[0] = gammas[0] * floral::normalize(floral::cross(rs[0], rs[1]));
				vs[1] = gammas[1] * floral::normalize(floral::cross(rs[1], rs[2]));
				vs[2] = gammas[2] * floral::normalize(floral::cross(rs[2], rs[3]));
				vs[3] = gammas[3] * floral::normalize(floral::cross(rs[3], rs[0]));

				lambertG = one2Pi * (
						floral::dot(floral::vec3f(0.0f, 1.0f, 0.0f), vs[0])
						+ floral::dot(floral::vec3f(0.0f, 1.0f, 0.0f), vs[1])
						+ floral::dot(floral::vec3f(0.0f, 1.0f, 0.0f), vs[2])
						+ floral::dot(floral::vec3f(0.0f, 1.0f, 0.0f), vs[3]));
			}
			fij +=  1.0 / m_Patch1Samples.get_size() * (df / dg) * lambertG;
		}
		CLOVER_DEBUG("fij = %f", fij);
#endif
	}

	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Scene", insigne::param_data_type_e::param_ub));

		strcpy(desc.vs, s_VertexShader);
		strcpy(desc.fs, s_FragmentShader);
		desc.vs_path = floral::path("/scene/sample_vs");
		desc.fs_path = floral::path("/scene/sample_fs");

		m_Shader = insigne::create_shader(desc);
		insigne::infuse_material(m_Shader, m_Material);

		// static uniform data
		{
			s32 ubSlot = insigne::get_material_uniform_block_slot(m_Material, "ub_Scene");
			m_Material.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 0, 0, m_UB };
		}
	}

	m_DebugDrawer.Initialize();
}

void FormFactorsValidating::OnDebugUIUpdate(const f32 i_deltaMs)
{
}

void FormFactorsValidating::OnUpdate(const f32 i_deltaMs)
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

	if (0)
	{
		for (size i = 0; i < m_Patch1Samples.get_size(); i++)
		{
			floral::vec3f org(m_Patch1Samples[i]);
			floral::vec3f dst(org.x, 0.3f, org.z);
			m_DebugDrawer.DrawLine3D(org, dst, floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f));
		}

		for (size i = 0; i < m_Patch2Samples.get_size(); i++)
		{
			floral::vec3f org(m_Patch2Samples[i]);
			floral::vec3f dst(org.x, 0.7, org.z);
			m_DebugDrawer.DrawLine3D(org, dst, floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f));
		}
	}
	else
	{
		for (size i = 0; i < m_Rays.get_size(); i++)
		{
			floral::vec3f org(m_Patch1Samples[i]);
			floral::vec3f dst(m_Patch2Samples[m_Rays[i]]);
			m_DebugDrawer.DrawLine3D(org, dst, floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f));
		}
	}
	
	m_DebugDrawer.EndFrame();
}

void FormFactorsValidating::OnRender(const f32 i_deltaMs)
{
	// camera
	m_SceneData.WVP = m_CameraMotion.GetWVP();
	insigne::update_ub(m_UB, &m_SceneData, sizeof(SceneData), 0);

	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::draw_surface<SurfacePNC>(m_VB, m_IB, m_Material);
	m_DebugDrawer.Render(m_SceneData.WVP);
	IDebugUI::OnFrameRender(i_deltaMs);
	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void FormFactorsValidating::OnCleanUp()
{
}

#if 0
// ---------------------------------------------
void FormFactorsValidating::CalculateFormFactors_Regular()
{
	// pre-allocate links
	for (u32 i = 0; i < m_GeoPatches.get_size(); i++) {
		GeoQuad& qi = m_GeoPatches[i];
		qi.PatchLinks.init(m_GeoPatches.get_size(), &g_StreammingAllocator);
		qi.FormFactors.init(m_GeoPatches.get_size(), &g_StreammingAllocator);
	}

	// ray-cast form factors
	for (u32 i = 0; i < m_GeoPatches.get_size(); i++) {
		GeoQuad& qi = m_GeoPatches[i];
		for (u32 j = 0; j < m_GeoPatches.get_size(); j++) {
			if (i == j) continue;

			GeoQuad& qj = m_GeoPatches[j];
			floral::vec3f patchCenter = (qi.Vertices[0] + qi.Vertices[1] + qi.Vertices[2] + qi.Vertices[3]) / 4.0f;
			floral::vec3f v0 = qj.Vertices[1] - qj.Vertices[0];
			floral::vec3f v1 = qj.Vertices[3] - qj.Vertices[0];
			f32 stepI = floral::length(v0) / 4.0f;
			f32 stepJ = floral::length(v1) / 4.0f;
			v0 = floral::normalize(v0);
			v1 = floral::normalize(v1);
			f32 ff = 0.0f;
			bool hasUpperHemiRay = false;
			for (u32 ri = 0; ri < 4; ri++) {
				for (u32 rj = 0; rj < 4; rj++) {
					floral::vec3f v = qj.Vertices[0] + v0 * (stepI * ri * 0.5f) + v1 * (stepJ * rj * 0.5f);
					floral::vec3f d1 = floral::normalize(v - patchCenter);
					floral::vec3f d2 = -d1;
					f32 dist = floral::length(v - patchCenter);

					if (floral::dot(d1, qi.Normal) > 0.0f)
					{
						hasUpperHemiRay = true;
						floral::ray3df r;
						r.o = patchCenter;
						r.d = d1;
						bool hit = false;
						for (u32 k = 0; k < m_GeoPatches.get_size(); k++)
						{
							if (k == i || k == j) continue;
							GeoQuad& qk = m_GeoPatches[k];
							f32 t = 0.0f;
							const bool rh = floral::ray_quad_intersect(r,
									qk.Vertices[0], qk.Vertices[1], qk.Vertices[2], qk.Vertices[3], &t);
							if (rh && t >= 0.0f && t <= dist)
							{
								hit = true;
								break;
							}
						}

						f32 cosTheta1 = floral::dot(qi.Normal, d1);
						f32 cosTheta2 = floral::dot(d2, qj.Normal);
						if (!hit && cosTheta1 * cosTheta2 >= 0.0f) {
							ff += cosTheta1 * cosTheta2 / (3.14f * dist * dist + stepI * stepJ);
						}
					}
				}
			}
			if (hasUpperHemiRay) {
				ff = ff * stepI * stepJ;
				qi.PatchLinks.push_back(j);
				qi.FormFactors.push_back(ff);
			}
		}
		CLOVER_DEBUG("Ray-cast form factor progress: %4.2f %%", (f32)i / (f32)m_GeoPatches.get_size() * 100.0f);
	}
}

void GenerateStratifiedSamples(floral::inplace_array<floral::vec2f, 16u>& o_samples)
{
	// range: [0.0f .. 1.0f]
	f32 gridSize = 1.0f / 4.0f;
	for (u32 i = 0; i < 4; i++)
	{
		f32 minX = gridSize * i;
		f32 maxX = minX + gridSize;
		for (u32 j = 0; j < 4; j++)
		{
			f32 minY = gridSize * j;
			f32 maxY = minY + gridSize;
			f32 rfx = (f32)rand() / (f32)RAND_MAX;
			f32 rfy = (f32)rand() / (f32)RAND_MAX;
			floral::vec2f p(minX + gridSize * rfx, minY + gridSize * rfy);
			o_samples.push_back(p);
		}
	}

	// shuffle
	for (u32 i = 0; i < o_samples.get_size(); i++)
	{
		for (u32 j = 0; j < o_samples.get_size(); j++)
		{
			floral::vec2f tmp = o_samples[i];
			o_samples[i] = o_samples[j];
			o_samples[j] = tmp;
		}
	}
}

void FormFactorsValidating::CalculateFormFactors_Stratified()
{
	// pre-allocate links
	for (u32 i = 0; i < m_GeoPatches.get_size(); i++) {
		GeoQuad& qi = m_GeoPatches[i];
		qi.PatchLinks.init(m_GeoPatches.get_size(), &g_StreammingAllocator);
		qi.FormFactors.init(m_GeoPatches.get_size(), &g_StreammingAllocator);
	}

	// ray-cast form factors
	for (u32 i = 0; i < m_GeoPatches.get_size(); i++) {
		GeoQuad& qi = m_GeoPatches[i];
		floral::vec3f vi0 = qi.Vertices[1] - qi.Vertices[0];
		floral::vec3f vi1 = qi.Vertices[3] - qi.Vertices[0];

		for (u32 j = 0; j < m_GeoPatches.get_size(); j++) {
			if (i == j) continue;

			GeoQuad& qj = m_GeoPatches[j];
			floral::inplace_array<floral::vec2f, 16u> pISamples;
			floral::inplace_array<floral::vec2f, 16u> pJSamples;

			GenerateStratifiedSamples(pISamples);
			GenerateStratifiedSamples(pJSamples);

			floral::vec3f vj0 = qj.Vertices[1] - qj.Vertices[0];
			floral::vec3f vj1 = qj.Vertices[3] - qj.Vertices[0];

			f32 area = floral::length(vj0) * floral::length(vj1);

			f32 ff = 0.0f;
			bool hasUpperHemiRay = false;
			for (u32 r = 0; r < 16; r++)
			{
				floral::vec3f vi = qi.Vertices[0] + vi0 * pISamples[r].x + vi1 * pISamples[r].y;
				floral::vec3f vj = qj.Vertices[0] + vj0 * pJSamples[r].x + vj1 * pJSamples[r].y;

				m_SampleLines.push_back(vi);
				m_SampleLines.push_back(vj);

				floral::vec3f d1 = floral::normalize(vj - vi);
				floral::vec3f d2 = -d1;
				f32 dist = floral::length(vi - vj);

				if (floral::dot(d1, qi.Normal) > 0.0f)
				{
					hasUpperHemiRay = true;
					floral::ray3df r;
					r.o = vj;
					r.d = d1;
					bool hit = false;
					for (u32 k = 0; k < m_GeoPatches.get_size(); k++)
					{
						if (k == i || k == j) continue;
						GeoQuad& qk = m_GeoPatches[k];
						f32 t = 0.0f;
						const bool rh = floral::ray_quad_intersect(r,
								qk.Vertices[0], qk.Vertices[1], qk.Vertices[2], qk.Vertices[3], &t);
						if (rh && t >= 0.0f && t <= dist)
						{
							hit = true;
							break;
						}
					}

					if (!hit)
					{
						f32 cosTheta1 = floral::dot(qi.Normal, d1);
						f32 cosTheta2 = floral::dot(d2, qj.Normal);
						//f32 deltaF = cosTheta1 * cosTheta2 / (3.14f * dist * dist + area / 16.0f);
						f32 deltaF = cosTheta1 * cosTheta2 / (3.14f * dist * dist);
						if (deltaF > 0)
						{
							ff += deltaF;
						}
					}
				}
			}
			if (hasUpperHemiRay) {
				ff = ff * area / 16.0f;
				qi.PatchLinks.push_back(j);
				qi.FormFactors.push_back(ff);
			}
		}
		CLOVER_DEBUG("Ray-cast form factor progress: %4.2f %%", (f32)i / (f32)m_GeoPatches.get_size() * 100.0f);
	}
}
#endif


}
