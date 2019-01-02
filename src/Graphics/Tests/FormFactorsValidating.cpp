#include "FormFactorsValidating.h"

#include <insigne/commons.h>
#include <insigne/counters.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_render.h>
#include <clover.h>

#include "Graphics/GeometryBuilder.h"

namespace stone {

static const_cstr s_GeometryVS = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in highp vec3 l_Normal_L;
layout (location = 2) in mediump vec4 l_Color0;
layout (location = 3) in mediump vec4 l_Color1;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_WVP;
};

out mediump vec4 v_Color;
out mediump vec3 v_Normal;

void main() {
	highp vec4 pos_W = vec4(l_Position_L, 1.0f);
	v_Color = l_Color0;
	v_Normal = l_Normal_L;
	gl_Position = iu_WVP * pos_W;
}
)";

static const_cstr s_GeometryFS = R"(#version 300 es
layout (location = 0) out mediump vec4 o_Color;

in mediump vec4 v_Color;
in mediump vec3 v_Normal;

void main()
{
	o_Color = v_Color;
}
)";

FormFactorsValidating::FormFactorsValidating()
{
	m_MemoryArena = g_PersistanceResourceAllocator.allocate_arena<LinearArena>(SIZE_MB(16));
}

FormFactorsValidating::~FormFactorsValidating()
{
}

void FormFactorsValidating::OnInitialize()
{
	m_GeoVertices.init(2048u, &g_StreammingAllocator);
	m_GeoIndices.init(8192u, &g_StreammingAllocator);
	m_GeoPatches.init(1024u, &g_StreammingAllocator);

	floral::mat4x4f mBottom = floral::construct_translation3d(0.0f, -0.5f, 0.0f);
	GenQuadTesselated3DPlane_Tris_PNCC(
			mBottom,
			1.0f, 1.0f, 1.0f, floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f),
			m_GeoVertices, m_GeoIndices, m_GeoPatches);

#if 0
	// parallel test
	floral::mat4x4f mTop = floral::construct_translation3d(0.0f, 0.5f, 0.0f)
		* floral::construct_quaternion_euler(180.0f, 0.0f, 0.0f).to_transform();

	GenQuadTesselated3DPlane_Tris_PNCC(
			mTop,
			1.0f, 1.0f, 1.0f, floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f),
			m_GeoVertices, m_GeoIndices, m_GeoPatches);
#else
	// perpendicular test
	floral::mat4x4f mSide = floral::construct_translation3d(0.0f, 0.0f, 0.5f)
		* floral::construct_quaternion_euler(-90.0f, 0.0f, 0.0f).to_transform();

	GenQuadTesselated3DPlane_Tris_PNCC(
			mSide,
			1.0f, 1.0f, 1.0f, floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f),
			m_GeoVertices, m_GeoIndices, m_GeoPatches);
#endif

#if 0
	CalculateFormFactors_Regular();
#else
	CalculateFormFactors_Stratified();
#endif

	{
		for (u32 i = 0; i < m_GeoPatches.get_size(); i++) {
			floral::vec4f rColor = m_GeoPatches[i].RadiosityColor;
			rColor.w = 1.0f;
			m_GeoVertices[i * 4].Color1 = rColor;
			m_GeoVertices[i * 4 + 1].Color1 = rColor;
			m_GeoVertices[i * 4 + 2].Color1 = rColor;
			m_GeoVertices[i * 4 + 3].Color1 = rColor;
		}
	}

	// upload scene geometry
	{
		insigne::vbdesc_t desc;
		desc.region_size = SIZE_KB(256);
		desc.stride = sizeof(VertexPNCC);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::vb_handle_t newVB = insigne::create_vb(desc);

		insigne::update_vb(newVB, &m_GeoVertices[0], m_GeoVertices.get_size(), 0);
		m_VB = newVB;
	}

	{
		insigne::ibdesc_t desc;
		desc.region_size = SIZE_KB(128);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ib_handle_t newIB = insigne::create_ib(desc);

		insigne::update_ib(newIB, &m_GeoIndices[0], m_GeoIndices.get_size(), 0);
		m_IB = newIB;
	}

	// camera
	m_CamView.position = floral::vec3f(5.0f, 1.0f, -1.0f);
	m_CamView.look_at = floral::vec3f(0.0f);
	m_CamView.up_direction = floral::vec3f(0.0f, 1.0f, 0.0f);

	m_CamProj.near_plane = 0.01f; m_CamProj.far_plane = 100.0f;
	m_CamProj.fov = 60.0f;
	m_CamProj.aspect_ratio = 16.0f / 9.0f;
	m_WVP = floral::construct_perspective(m_CamProj) * construct_lookat_point(m_CamView);

	// upload scene data
	{
		insigne::ubdesc_t desc;
		desc.region_size = SIZE_KB(4);
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ub_handle_t newUB = insigne::create_ub(desc);

		SceneData sceneData;
		sceneData.WVP = m_WVP;

		insigne::copy_update_ub(newUB, &sceneData, sizeof(SceneData), 0);
		m_UB = newUB;
	}

	// scene shaders
	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Scene", insigne::param_data_type_e::param_ub));

		strcpy(desc.vs, s_GeometryVS);
		strcpy(desc.fs, s_GeometryFS);

		m_Shader = insigne::create_shader(desc);
		insigne::infuse_material(m_Shader, m_Material);

		{
			u32 ubSlot = insigne::get_material_uniform_block_slot(m_Material, "ub_Scene");
			m_Material.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 0, 0, m_UB };
		}
	}

	m_DebugDrawer.Initialize();
}

void FormFactorsValidating::OnUpdate(const f32 i_deltaMs)
{
}

void FormFactorsValidating::OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	m_DebugDrawer.Render(m_WVP);
	insigne::draw_surface<SurfacePNCC>(m_VB, m_IB, m_Material);
	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void FormFactorsValidating::OnCleanUp()
{
}

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

}
