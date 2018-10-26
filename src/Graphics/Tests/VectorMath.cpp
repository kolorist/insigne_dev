#include "VectorMath.h"

#include <insigne/commons.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_render.h>
#include <insigne/ut_textures.h>

namespace stone {

static const_cstr s_VertexShader = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;

layout(std140) uniform ub_XForm
{
	highp mat4 iu_WVP;
};

out mediump vec3 v_SampleDir_W;

void main() {
	highp vec4 pos_W = vec4(l_Position_L, 1.0f);
	v_SampleDir_W = pos_W.xyz;
	gl_Position = iu_WVP * pos_W;
}
)";

static const_cstr s_FragmentShader = R"(#version 300 es

layout (location = 0) out mediump vec4 o_Color;

uniform mediump samplerCube u_Tex;
in mediump vec3 v_SampleDir_W;

void main()
{
	mediump vec3 outColor = texture(u_Tex, v_SampleDir_W).rgb;
	o_Color = vec4(outColor, 1.0f);
}
)";

VectorMath::VectorMath()
{
	m_MemoryArena = g_PersistanceResourceAllocator.allocate_arena<LinearArena>(SIZE_MB(24));
}

VectorMath::~VectorMath()
{
}

void VectorMath::OnInitialize()
{
	{
		floral::vec2f v1;
		floral::vec2f v2(1.0f);
		floral::vec2f v3(1.0f, 2.0f);
		floral::vec2f v4(v3);

		bool cmp = (v3 == v4);
		floral::vec2f v5;
		v5 = +v2;
		floral::vec2f v6;
		v6 = -v2;

		floral::vec2f v7 = v2 + v3;
		floral::vec2f v8 = v2 - v3;
		floral::vec2f v9 = v2 * v3;
		floral::vec2f v10 = v2 / v3;

		floral::vec2f v11 = v3 * 2.0f;
		floral::vec2f v12 = 2.0f * v3;
		floral::vec2f v13 = v3 / 2.0f;
		floral::vec2f v14 = 2.0f / v3;
		f32 r1 = floral::dot(v2, v3);
		f32 l1 = floral::length(v3);
		floral::vec2f v15 = normalize(v3);

		floral::vec2f v16(1.0f);
		v16 += v3;

		floral::vec2f v17(1.0f);
		v17 -= v3;

		floral::vec2f v18(1.0f);
		v18 *= v3;

		floral::vec2f v19(1.0f);
		v19 /= v3;

		floral::vec2f v20(1.0f, 2.0f);
		v20 *= 2.0f;

		floral::vec2f v21(1.0f, 2.0f);
		v21 /= 2.0f;

		floral::vec2f v22(1.0f, 2.0f);
		floral::vec2f v23 = v22.normalize();
	}

	{
		floral::vec3f v1;
		floral::vec3f v2(1.0f);
		floral::vec3f v3(1.0f, 2.0f, 3.0f);
		floral::vec3f v4(v3);

		bool cmp = (v3 == v4);
		floral::vec3f v5;
		v5 = +v2;
		floral::vec3f v6;
		v6 = -v2;

		floral::vec3f v7 = v2 + v3;
		floral::vec3f v8 = v2 - v3;
		floral::vec3f v9 = v2 * v3;
		floral::vec3f v10 = v2 / v3;

		floral::vec3f v11 = v3 * 2.0f;
		floral::vec3f v12 = 2.0f * v3;
		floral::vec3f v13 = v3 / 2.0f;
		floral::vec3f v14 = 2.0f / v3;
		f32 r1 = floral::dot(v2, v3);
		f32 l1 = floral::length(v3);
		floral::vec3f v15 = normalize(v3);

		floral::vec3f v16(1.0f);
		v16 += v3;

		floral::vec3f v17(1.0f);
		v17 -= v3;

		floral::vec3f v18(1.0f);
		v18 *= v3;

		floral::vec3f v19(1.0f);
		v19 /= v3;

		floral::vec3f v20(1.0f, 2.0f, 3.0f);
		v20 *= 2.0f;

		floral::vec3f v21(1.0f, 2.0f, 3.0f);
		v21 /= 2.0f;

		floral::vec3f v22(1.0f, 2.0f, 3.0f);
		floral::vec3f v23 = v22.normalize();
		floral::vec3f v24 = cross(v2, v3);
	}

	{
		floral::vec4f v1;
		floral::vec4f v2(1.0f);
		floral::vec4f v3(1.0f, 2.0f, 3.0f, 4.0f);
		floral::vec4f v4(v3);

		bool cmp = (v3 == v4);
		floral::vec4f v5;
		v5 = +v2;
		floral::vec4f v6;
		v6 = -v2;

		floral::vec4f v7 = v2 + v3;
		floral::vec4f v8 = v2 - v3;
		floral::vec4f v9 = v2 * v3;
		floral::vec4f v10 = v2 / v3;

		floral::vec4f v11 = v3 * 2.0f;
		floral::vec4f v12 = 2.0f * v3;
		floral::vec4f v13 = v3 / 2.0f;
		floral::vec4f v14 = 2.0f / v3;
		f32 r1 = floral::dot(v2, v3);
		f32 l1 = floral::length(v3);
		floral::vec4f v15 = normalize(v3);

		floral::vec4f v16(1.0f);
		v16 += v3;

		floral::vec4f v17(1.0f);
		v17 -= v3;

		floral::vec4f v18(1.0f);
		v18 *= v3;

		floral::vec4f v19(1.0f);
		v19 /= v3;

		floral::vec4f v20(1.0f, 2.0f, 3.0f, 4.0f);
		v20 *= 2.0f;

		floral::vec4f v21(1.0f, 2.0f, 3.0f, 4.0f);
		v21 /= 2.0f;

		floral::vec4f v22(1.0f, 2.0f, 3.0f, 4.0f);
		floral::vec4f v23 = v22.normalize();
	}

	// camera
	m_CamView.position = floral::vec3f(5.0f, 4.0f, 3.0f);
	m_CamView.look_at = floral::vec3f(0.0f);
	m_CamView.up_direction = floral::vec3f(0.0f, 1.0f, 0.0f);

	m_CamProj.near_plane = 0.01f; m_CamProj.far_plane = 100.0f;
	m_CamProj.fov = 60.0f;
	m_CamProj.aspect_ratio = 16.0f / 9.0f;

	m_WVP = floral::construct_perspective(m_CamProj) * construct_lookat_point(m_CamView);

	// flush the initialization pass
	insigne::dispatch_render_pass();
	m_DebugDrawer.Initialize();

	{
		m_Cube.init(36u, m_MemoryArena);
		floral::vec3f v0(-1.0f, -1.0f, -1.0f);
		floral::vec3f v1(1.0f, -1.0f, -1.0f);
		floral::vec3f v2(1.0f, -1.0f, 1.0f);
		floral::vec3f v3(-1.0f, -1.0f, 1.0f);

		floral::vec3f v4(-1.0f, 1.0f, -1.0f);
		floral::vec3f v5(1.0f, 1.0f, -1.0f);
		floral::vec3f v6(1.0f, 1.0f, 1.0f);
		floral::vec3f v7(-1.0f, 1.0f, 1.0f);
		// bottom
		m_Cube.push_back(v0);
		m_Cube.push_back(v1);
		m_Cube.push_back(v2);
		m_Cube.push_back(v2);
		m_Cube.push_back(v3);
		m_Cube.push_back(v0);
		// top
		m_Cube.push_back(v4);
		m_Cube.push_back(v5);
		m_Cube.push_back(v6);
		m_Cube.push_back(v6);
		m_Cube.push_back(v7);
		m_Cube.push_back(v4);
		// side 1
		m_Cube.push_back(v0);
		m_Cube.push_back(v4);
		m_Cube.push_back(v5);
		m_Cube.push_back(v5);
		m_Cube.push_back(v1);
		m_Cube.push_back(v0);
		// side 2
		m_Cube.push_back(v1);
		m_Cube.push_back(v5);
		m_Cube.push_back(v6);
		m_Cube.push_back(v6);
		m_Cube.push_back(v2);
		m_Cube.push_back(v1);
		// side 3
		m_Cube.push_back(v2);
		m_Cube.push_back(v6);
		m_Cube.push_back(v7);
		m_Cube.push_back(v7);
		m_Cube.push_back(v3);
		m_Cube.push_back(v2);
		// side 4
		m_Cube.push_back(v0);
		m_Cube.push_back(v3);
		m_Cube.push_back(v7);
		m_Cube.push_back(v7);
		m_Cube.push_back(v4);
		m_Cube.push_back(v0);

		m_TranslationCube.init(36u, m_MemoryArena);
		m_ScaleCube.init(36u, m_MemoryArena);
		m_RotationCube.init(36u, m_MemoryArena);
		m_XFormCube.init(36u, m_MemoryArena);
		m_TranslationCube = m_Cube;
		m_ScaleCube = m_Cube;
		m_RotationCube = m_Cube;
		m_XFormCube = m_Cube;
	}

	{
		insigne::framebuffer_desc_t fbDesc = insigne::create_framebuffer_desc();
		fbDesc.clear_color = floral::vec4f(0.3f, 0.4f, 0.5f, 1.0f);
		fbDesc.width = 1024;
		fbDesc.height = 1024;
		fbDesc.scale = 1.0f;
		fbDesc.has_depth = true;
		fbDesc.color_attachments->push_back(insigne::color_attachment_t("color0", insigne::texture_format_e::hdr_rgba));

		m_FbHandle = insigne::create_framebuffer(fbDesc);
	}
	insigne::dispatch_render_pass();
}

void VectorMath::OnUpdate(const f32 i_deltaMs)
{
	m_DebugDrawer.BeginFrame();
	//m_DebugDrawer.DrawLine3D(floral::vec3f(0.0f), floral::vec3f(0.0f, 2.0f, 2.0f), floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f));
	//m_DebugDrawer.DrawAABB3D(floral::aabb3f(floral::vec3f(0.8f), floral::vec3f(1.5f)), floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f));
	//m_DebugDrawer.DrawIcosahedron3D(floral::vec3f(0.0f, 0.0f, -3.0f), 0.5, floral::vec4f(0.0f, 0.0f, 1.0f, 1.0f));
	//m_DebugDrawer.DrawPolygon3D(m_Cube, floral::vec4f(1.0f, 0.0f, 1.0f, 1.0f));
	// cpu translation test
	static f32 s_elapsedTime = 0.0f;
	s_elapsedTime += i_deltaMs;
	{
		floral::vec3f delta(-2.0f, 0.0f, 2.0f * sinf(floral::to_radians(s_elapsedTime / 10.0f)));
		floral::mat4x4f m = floral::construct_translation3d(delta);
		for (u32 i = 0; i < m_Cube.get_size(); i++) {
			floral::vec4f v = m * floral::vec4f(m_Cube[i] * 0.5f, 1.0f);
			m_TranslationCube[i] = floral::vec3f(v.x, v.y, v.z);
		}
		m_DebugDrawer.DrawPolygon3D(m_TranslationCube, floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f));
	}
	{
		floral::vec3f delta(
				0.2f + 0.3f * (1.0f + sinf(floral::to_radians(s_elapsedTime / 10.0f)) / 2.0f),
				0.2f + 0.3f * (1.0f + cosf(floral::to_radians(s_elapsedTime / 10.0f)) / 2.0f),
				0.2f + 0.3f * (1.0f + sinf(floral::to_radians(s_elapsedTime / 10.0f)) / 2.0f));
		floral::mat4x4f m = floral::construct_scaling3d(delta);
		for (u32 i = 0; i < m_Cube.get_size(); i++) {
			floral::vec4f v = m * floral::vec4f(m_Cube[i] * 0.5f, 1.0f);
			m_ScaleCube[i] = floral::vec3f(v.x, v.y, v.z) + floral::vec3f(1.0f, 0.0f, -2.0f);
		}
		m_DebugDrawer.DrawPolygon3D(m_ScaleCube, floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f));
	}
	{
		floral::vec3f deltaEuler1(720.0f * sinf(floral::to_radians(s_elapsedTime / 50.0f)), 0.0f, 0.0f);
		floral::vec3f deltaEuler2(0.0f, 720.0f * sinf(floral::to_radians(s_elapsedTime / 50.0f)), 0.0f);
		floral::vec3f deltaEuler3(0.0f, 0.0f, 720.0f * sinf(floral::to_radians(s_elapsedTime / 50.0f)));
		floral::vec3f deltaEuler4(720.0f * sinf(floral::to_radians(s_elapsedTime / 50.0f)),
					720.0f * cosf(floral::to_radians(s_elapsedTime / 50.0f)),
					-720.0f * sinf(floral::to_radians(s_elapsedTime / 50.0f)));
		floral::quaternionf q1 = floral::construct_quaternion_euler(deltaEuler1);
		floral::quaternionf q2 = floral::construct_quaternion_euler(deltaEuler2);
		floral::quaternionf q3 = floral::construct_quaternion_euler(deltaEuler3);
		floral::quaternionf q4 = floral::construct_quaternion_euler(deltaEuler4);
		floral::mat4x4f m1 = q1.to_transform();
		floral::mat4x4f m2 = q2.to_transform();
		floral::mat4x4f m3 = q3.to_transform();
		floral::mat4x4f m4 = q4.to_transform();

		for (u32 i = 0; i < m_Cube.get_size(); i++) {
			floral::vec4f v = m1 * floral::vec4f(m_Cube[i] * floral::vec3f(0.1f, 0.2f, 0.1f), 1.0f);
			m_RotationCube[i] = floral::vec3f(v.x, v.y, v.z) + floral::vec3f(1.0f, 0.0f, 2.0f);
		}
		m_DebugDrawer.DrawPolygon3D(m_RotationCube, floral::vec4f(0.0f, 0.0f, 1.0f, 1.0f));

		for (u32 i = 0; i < m_Cube.get_size(); i++) {
			floral::vec4f v = m2 * floral::vec4f(m_Cube[i] * floral::vec3f(0.1f, 0.1f, 0.2f), 1.0f);
			m_RotationCube[i] = floral::vec3f(v.x, v.y, v.z) + floral::vec3f(1.0f, 0.0f, 1.5f);
		}
		m_DebugDrawer.DrawPolygon3D(m_RotationCube, floral::vec4f(0.0f, 0.0f, 1.0f, 1.0f));

		for (u32 i = 0; i < m_Cube.get_size(); i++) {
			floral::vec4f v = m3 * floral::vec4f(m_Cube[i] * floral::vec3f(0.2f, 0.1f, 0.1f), 1.0f);
			m_RotationCube[i] = floral::vec3f(v.x, v.y, v.z) + floral::vec3f(1.0f, 0.0f, 1.0f);
		}
		m_DebugDrawer.DrawPolygon3D(m_RotationCube, floral::vec4f(0.0f, 0.0f, 1.0f, 1.0f));

		for (u32 i = 0; i < m_Cube.get_size(); i++) {
			floral::vec4f v = m4 * floral::vec4f(m_Cube[i] * floral::vec3f(0.1f, 0.1f, 0.1f), 1.0f);
			m_RotationCube[i] = floral::vec3f(v.x, v.y, v.z) + floral::vec3f(1.0f, 0.0f, 0.5f);
		}
		m_DebugDrawer.DrawPolygon3D(m_RotationCube, floral::vec4f(0.0f, 0.0f, 1.0f, 1.0f));
	}
	{
		floral::vec3f deltaPos(0.0f, 0.0f, 2.0f * sinf(floral::to_radians(s_elapsedTime / 10.0f)));
		floral::vec3f deltaScl(
				0.2f + 0.3f * (1.0f + sinf(floral::to_radians(s_elapsedTime / 10.0f)) / 2.0f),
				0.2f + 0.3f * (1.0f + cosf(floral::to_radians(s_elapsedTime / 10.0f)) / 2.0f),
				0.2f + 0.3f * (1.0f + sinf(floral::to_radians(s_elapsedTime / 10.0f)) / 2.0f));
		floral::vec3f deltaRot(720.0f * sinf(floral::to_radians(s_elapsedTime / 50.0f)),
					720.0f * cosf(floral::to_radians(s_elapsedTime / 50.0f)),
					-720.0f * sinf(floral::to_radians(s_elapsedTime / 50.0f)));
		floral::mat4x4f t = floral::construct_translation3d(deltaPos);
		floral::mat4x4f s = floral::construct_scaling3d(deltaScl);
		floral::quaternionf q = floral::construct_quaternion_euler(deltaRot);
		floral::mat4x4f r = q.to_transform();
		floral::mat4x4f m = t * r * s;

		for (u32 i = 0; i < m_Cube.get_size(); i++) {
			floral::vec4f v = m * floral::vec4f(m_Cube[i] * 0.4f, 1.0f);
			m_XFormCube[i] = floral::vec3f(v.x, v.y, v.z);
		}
		m_DebugDrawer.DrawPolygon3D(m_XFormCube, floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f));
	}
	m_DebugDrawer.EndFrame();
}

void VectorMath::OnRender(const f32 i_deltaMs)
{
	//insigne::begin_render_pass(m_FbHandle);
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	// render here
	m_DebugDrawer.Render(m_WVP);
	//insigne::end_render_pass(m_FbHandle);
	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void VectorMath::OnCleanUp()
{
}

}
