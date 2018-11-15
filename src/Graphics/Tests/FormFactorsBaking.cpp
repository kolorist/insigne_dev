#include "FormFactorsBaking.h"

#include <insigne/ut_render.h>

namespace stone {

FormFactorsBaking::FormFactorsBaking()
{
	m_MemoryArena = g_PersistanceResourceAllocator.allocate_arena<LinearArena>(SIZE_MB(16));
}

FormFactorsBaking::~FormFactorsBaking()
{
}

void FormFactorsBaking::OnInitialize()
{
	floral::ray3df r;
	r.o = floral::vec3f(0.0f, 0.0f, 0.0f);
	r.d = floral::vec3f(-1.0f, 0.0f, 0.0f);
	f32 t = 0.0f;


	// camera
	m_CamView.position = floral::vec3f(5.0f, 4.0f, 3.0f);
	m_CamView.look_at = floral::vec3f(0.0f);
	m_CamView.up_direction = floral::vec3f(0.0f, 1.0f, 0.0f);

	m_CamProj.near_plane = 0.01f; m_CamProj.far_plane = 100.0f;
	m_CamProj.fov = 60.0f;
	m_CamProj.aspect_ratio = 16.0f / 9.0f;

	m_WVP = floral::construct_perspective(m_CamProj) * construct_lookat_point(m_CamView);

	m_DebugDrawer.Initialize();
}

void FormFactorsBaking::OnUpdate(const f32 i_deltaMs)
{
	static f32 elapsedTime = 0.0f;
	elapsedTime += i_deltaMs;
	m_DebugDrawer.BeginFrame();
	floral::vec3f p0(-2.0f);
	floral::vec3f p1(-2.0f, -2.0f, 2.0f);
	floral::vec3f p2(0.0f, 2.0f, 0.0f);

	floral::ray3df r;
	r.o = floral::vec3f(0.0f, 0.0f, 0.0f);
	r.d = floral::vec3f(cosf(floral::to_radians(elapsedTime / 20.0f)),
			0.0f, sinf(floral::to_radians(elapsedTime / 20.0f)));

	f32 t = 0.0f;
	const bool hit = floral::ray_triangle_intersect(r, p0, p1, p2, &t);
	floral::vec4f color;
	if (hit && t >= 0.0f)
		color = floral::vec4f(1.0f, 1.0f, 0.0f, 1.0f);
	else color = floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f);

	m_DebugDrawer.DrawLine3D(p0, p1, color);
	m_DebugDrawer.DrawLine3D(p1, p2, color);
	m_DebugDrawer.DrawLine3D(p2, p0, color);

	floral::vec3f rp = r.o + 3.0f * r.d;
	m_DebugDrawer.DrawLine3D(r.o, rp, floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f));


	m_DebugDrawer.EndFrame();
}

void FormFactorsBaking::OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	m_DebugDrawer.Render(m_WVP);
	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void FormFactorsBaking::OnCleanUp()
{
}

}
