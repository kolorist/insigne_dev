#include "FormFactorsBaking.h"

#include <insigne/commons.h>
#include <insigne/counters.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_render.h>

#include "Graphics/GeometryBuilder.h"

namespace stone {

static const_cstr s_GeometryVS = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in highp vec3 l_Normal_L;
layout (location = 2) in mediump vec4 l_Color;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_WVP;
};

out mediump vec4 v_Color;
out mediump vec3 v_Normal;

void main() {
	highp vec4 pos_W = vec4(l_Position_L, 1.0f);
	v_Color = l_Color;
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
	//o_Color = vec4(v_Normal, 1.0f);
}
)";

FormFactorsBaking::FormFactorsBaking()
{
	m_MemoryArena = g_PersistanceResourceAllocator.allocate_arena<LinearArena>(SIZE_MB(16));
}

FormFactorsBaking::~FormFactorsBaking()
{
}

void FormFactorsBaking::OnInitialize()
{
	m_GeoVertices.init(2048u, &g_StreammingAllocator);
	m_GeoIndices.init(8192u, &g_StreammingAllocator);
	m_GeoPatchesBottom.init(1024u, &g_StreammingAllocator);
	m_GeoPatchesLeft.init(1024u, &g_StreammingAllocator);
	m_GeoPatchesRight.init(1024u, &g_StreammingAllocator);

	{
		floral::mat4x4f mBottom = floral::construct_translation3d(0.0f, -1.0f, 0.0f);
		floral::mat4x4f mLeft = floral::construct_translation3d(0.0f, 0.0f, 1.5f)
			* floral::construct_quaternion_euler(-90.0f, 0.0f, 0.0f).to_transform();
		floral::mat4x4f mRight = floral::construct_translation3d(0.0f, 0.0f, -1.5f)
			* floral::construct_quaternion_euler(90.0f, 0.0f, 0.0f).to_transform();

		GenQuadTesselated3DPlane_Tris_PNC(
				mBottom,
				2.0f, 3.0f, 0.3f, floral::vec4f(0.3f, 0.3f, 0.3f, 1.0f),
				m_GeoVertices, m_GeoIndices, m_GeoPatchesBottom);
		GenQuadTesselated3DPlane_Tris_PNC(
				mLeft,
				2.0f, 2.0f, 0.3f, floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f),
				m_GeoVertices, m_GeoIndices, m_GeoPatchesLeft);
		GenQuadTesselated3DPlane_Tris_PNC(
				mRight,
				2.0f, 2.0f, 0.3f, floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f),
				m_GeoVertices, m_GeoIndices, m_GeoPatchesRight);
	}

	// upload scene geometry
	{
		insigne::vbdesc_t desc;
		desc.region_size = SIZE_KB(64);
		desc.stride = sizeof(VertexPNC);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::vb_handle_t newVB = insigne::create_vb(desc);

		insigne::update_vb(newVB, &m_GeoVertices[0], m_GeoVertices.get_size(), 0);
		m_VB = newVB;
	}

	{
		insigne::ibdesc_t desc;
		desc.region_size = SIZE_KB(16);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ib_handle_t newIB = insigne::create_ib(desc);

		insigne::update_ib(newIB, &m_GeoIndices[0], m_GeoIndices.get_size(), 0);
		m_IB = newIB;
	}

	// camera
	m_CamView.position = floral::vec3f(5.0f, 1.0f, 0.0f);
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

void FormFactorsBaking::OnUpdate(const f32 i_deltaMs)
{
#if 0
	static f32 elapsedTime = 0.0f;
	elapsedTime += i_deltaMs;

	floral::ray3df r;
	r.o = floral::vec3f(0.0f, 0.0f, 0.0f);
	r.d = floral::vec3f(cosf(floral::to_radians(elapsedTime / 20.0f)),
			0.0f, sinf(floral::to_radians(elapsedTime / 20.0f)));

	m_DebugDrawer.BeginFrame();

	floral::vec3f rp = r.o + 3.0f * r.d;
	m_DebugDrawer.DrawLine3D(r.o, rp, floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f));

	{
		floral::vec3f p0(-2.0f);
		floral::vec3f p1(-2.0f, -2.0f, 2.0f);
		floral::vec3f p2(0.0f, 2.0f, 0.0f);

		f32 t = 0.0f;
		const bool hit = floral::ray_triangle_intersect(r, p0, p1, p2, &t);
		floral::vec4f color;
		if (hit && t >= 0.0f)
			color = floral::vec4f(1.0f, 1.0f, 0.0f, 1.0f);
		else color = floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f);

		m_DebugDrawer.DrawLine3D(p0, p1, color);
		m_DebugDrawer.DrawLine3D(p1, p2, color);
		m_DebugDrawer.DrawLine3D(p2, p0, color);
	}

	{
		floral::vec3f p0(0.3f, -0.5f, -0.4f);
		floral::vec3f p1(0.5f, -0.4f, 0.5f);
		floral::vec3f p2(0.6f, 0.5f, 0.5f);
		floral::vec3f p3(0.4f, 0.4f, -0.3f);

		f32 t = 0.0f;
		const bool hit = floral::ray_quad_intersect(r, p0, p1, p2, p3, &t);
		floral::vec4f color;
		if (hit && t >= 0.0f)
			color = floral::vec4f(1.0f, 1.0f, 0.0f, 1.0f);
		else color = floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f);

		m_DebugDrawer.DrawLine3D(p0, p1, color);
		m_DebugDrawer.DrawLine3D(p1, p2, color);
		m_DebugDrawer.DrawLine3D(p2, p3, color);
		m_DebugDrawer.DrawLine3D(p3, p0, color);
	}

	m_DebugDrawer.EndFrame();
#endif
	m_DebugDrawer.BeginFrame();
	static u32 pid = 20;
	floral::vec3f patchCenter = (m_GeoPatchesBottom[0] + m_GeoPatchesBottom[1]
		+ m_GeoPatchesBottom[2] + m_GeoPatchesBottom[3]) / 4.0f;
	floral::vec3f v0 = m_GeoPatchesLeft[pid + 1] - m_GeoPatchesLeft[pid + 0];
	floral::vec3f v1 = m_GeoPatchesLeft[pid + 3] - m_GeoPatchesLeft[pid + 0];
	f32 stepI = floral::length(v0) / 4.0f;
	f32 stepJ = floral::length(v1) / 4.0f;
	v0 = floral::normalize(v0);
	v1 = floral::normalize(v1);
	f32 ff = 0.0f;
	for (u32 i = 0; i < 4; i++) {
		for (u32 j = 0; j < 4; j++) {
			floral::vec3f v = m_GeoPatchesLeft[pid] + v0 * (stepI * i * 0.5f) + v1 * (stepJ * j * 0.5f);
			m_DebugDrawer.DrawLine3D(patchCenter, v, floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f));
			floral::vec3f d1 = floral::normalize(v - patchCenter);
			floral::vec3f d2 = -d1;
			f32 r = floral::length(v - patchCenter);
			f32 cosTheta1 = floral::dot(floral::vec3f(0.0f, 1.0f, 0.0f), d1);
			f32 cosTheta2 = floral::dot(d2, floral::vec3f(0.0f, 0.0f, -1.0f));
			ff += cosTheta1 * cosTheta2 / (3.14f * r * r + stepI * stepJ / 16.0f);
		}
	}
	ff /= 16.0f;
	//pid = pid + 4;
	if (pid >= m_GeoPatchesLeft.get_size()) pid = 0;
	m_DebugDrawer.EndFrame();
}

void FormFactorsBaking::OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	m_DebugDrawer.Render(m_WVP);
	insigne::draw_surface<SurfacePNC>(m_VB, m_IB, m_Material);
	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void FormFactorsBaking::OnCleanUp()
{
}

}
