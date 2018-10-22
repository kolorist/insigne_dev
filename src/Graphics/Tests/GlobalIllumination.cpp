#include "GlobalIllumination.h"

#include <insigne/commons.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_render.h>

#include "Graphics/GeometryBuilder.h"

namespace stone {

static const_cstr s_ShadowVS = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;

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

static const_cstr s_ShadowFS = R"(#version 300 es

void main()
{
	// nothing :)
}
)";

static const_cstr s_VertexShader = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in highp vec3 l_Normal_L;
layout (location = 2) in mediump vec4 l_Color;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_XForm;
	highp mat4 iu_WVP;
	highp vec4 iu_CameraPos;
};

layout(std140) uniform ub_LightScene
{
	highp mat4 iu_LightXForm;
	highp mat4 iu_LightWVP;
};

layout(std140) uniform ub_Light
{
	highp vec4 iu_LightDirection;
	mediump vec4 iu_LightColor;
	mediump vec4 iu_LightRadiance;
};

out mediump vec4 v_VertexColor;
out highp vec3 v_Normal;
out highp vec3 v_LightSpacePos;
out highp vec3 v_ViewDir;

void main() {
	highp vec4 pos_W = iu_XForm * vec4(l_Position_L, 1.0f);
	v_VertexColor = l_Color;
	v_Normal = l_Normal_L;
	v_ViewDir = normalize(iu_CameraPos.xyz - pos_W.xyz);
	highp vec4 pos_LS = iu_LightWVP * iu_LightXForm * vec4(l_Position_L, 1.0f);
	v_LightSpacePos = pos_LS.xyz / pos_LS.w; // perspective division, no need for orthographic anyway
	gl_Position = iu_WVP * pos_W;
}
)";

static const_cstr s_FragmentShader = R"(#version 300 es
layout (location = 0) out mediump vec4 o_Color;

layout(std140) uniform ub_Light
{
	highp vec4 iu_LightDirection;
	mediump vec4 iu_LightColor;
	mediump vec4 iu_LightRadiance;
};

uniform highp sampler2D iu_ShadowMap;

in mediump vec4 v_VertexColor;
in highp vec3 v_Normal;
in highp vec3 v_LightSpacePos;
in highp vec3 v_ViewDir;

void main()
{
	highp vec3 n = normalize(v_Normal);
	highp vec3 l = normalize(iu_LightDirection.xyz);
	highp vec3 v = normalize(v_ViewDir);
	highp vec3 h = normalize(l + v);

	mediump float nol = max(dot(n, l), 0.0f);
	mediump float noh = max(dot(n, h), 0.0f);

	mediump vec3 r = normalize(- l - 2.0f * dot(n, -l) * n);
	mediump float rov = max(dot(r, v), 0.0f);
	mediump vec3 diff = v_VertexColor.xyz * iu_LightRadiance.xyz * nol;
	mediump vec3 spec = v_VertexColor.xyz * iu_LightRadiance.xyz * pow(noh, 90.0f);
	mediump vec3 color = diff + spec;

	// shadow
	highp vec2 smUV = (v_LightSpacePos.xy + vec2(1.0f)) / vec2(2.0f);
	highp float ld = texture(iu_ShadowMap, smUV).r;
	highp float sld = v_LightSpacePos.z * 0.5f + 0.5f;
	mediump float shadowMask = 1.0f - step(0.0002f, sld - ld);

	o_Color = vec4(ld, ld, ld, 1.0f);
}
)";

GlobalIllumination::GlobalIllumination()
{
}

GlobalIllumination::~GlobalIllumination()
{
}

void GlobalIllumination::OnInitialize()
{
	m_Vertices.init(128u, &g_StreammingAllocator);
	m_Indices.init(256u, &g_StreammingAllocator);

	floral::aabb3f sceneBB;

	{
		// bottom
		{
			floral::mat4x4f m =
				floral::construct_translation3d(0.0f, -1.0f, 0.0f);
			Gen3DPlane_Tris_PosNormalColor(floral::vec4f(1.0f, 1.0f, 1.0f, 1.0f), m, m_Vertices, m_Indices);
		}

		// top
		if (0)
		{
			floral::mat4x4f m =
				floral::construct_translation3d(0.0f, 1.0f, 0.0f) *
				floral::construct_quaternion_euler(180.0f, 0.0f, 0.0f).to_transform();
			Gen3DPlane_Tris_PosNormalColor(floral::vec4f(1.0f, 1.0f, 1.0f, 1.0f), m, m_Vertices, m_Indices);
		}

		// right
		{
			floral::mat4x4f m =
				floral::construct_translation3d(0.0f, 0.0f, -1.0f) *
				floral::construct_quaternion_euler(90.0f, 0.0f, 0.0f).to_transform();
			Gen3DPlane_Tris_PosNormalColor(floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f), m, m_Vertices, m_Indices);
		}

		// left
		{
			floral::mat4x4f m =
				floral::construct_translation3d(0.0f, 0.0f, 1.0f) *
				floral::construct_quaternion_euler(-90.0f, 0.0f, 0.0f).to_transform();
			Gen3DPlane_Tris_PosNormalColor(floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f), m, m_Vertices, m_Indices);
		}

		// back
		{
			floral::mat4x4f m =
				floral::construct_translation3d(-1.0f, 0.0f, 0.0f) *
				floral::construct_quaternion_euler(0.0f, 0.0f, -90.0f).to_transform();
			Gen3DPlane_Tris_PosNormalColor(floral::vec4f(1.0f, 1.0f, 1.0f, 1.0f), m, m_Vertices, m_Indices);
		}

		// small box
		{
			floral::mat4x4f m =
				floral::construct_translation3d(0.0f, -0.6f, 0.45f) *
				floral::construct_quaternion_euler(0.0f, 35.0f, 0.0f).to_transform() *
				floral::construct_scaling3d(0.2f, 0.4f, 0.2f);
			GenBox_Tris_PosNormalColor(floral::vec4f(1.0f, 1.0f, 0.0f, 1.0f), m, m_Vertices, m_Indices);
		}

		// large box
		{
			floral::mat4x4f m =
				floral::construct_translation3d(0.2f, -0.4f, -0.4f) *
				floral::construct_quaternion_euler(0.0f, -15.0f, 0.0f).to_transform() *
				floral::construct_scaling3d(0.3f, 0.6f, 0.3f);
			GenBox_Tris_PosNormalColor(floral::vec4f(1.0f, 1.0f, 0.0f, 1.0f), m, m_Vertices, m_Indices);
		}

		// calculate scene bounding box
		{
			sceneBB.min_corner = floral::vec3f(99.9f);
			sceneBB.max_corner = floral::vec3f(-99.9f);
			for (u32 i = 0; i < m_Vertices.get_size(); i++) {
				if (m_Vertices[i].Position.x < sceneBB.min_corner.x) sceneBB.min_corner.x = m_Vertices[i].Position.x;
				if (m_Vertices[i].Position.y < sceneBB.min_corner.y) sceneBB.min_corner.y = m_Vertices[i].Position.y;
				if (m_Vertices[i].Position.z < sceneBB.min_corner.z) sceneBB.min_corner.z = m_Vertices[i].Position.z;

				if (m_Vertices[i].Position.x > sceneBB.max_corner.x) sceneBB.max_corner.x = m_Vertices[i].Position.x;
				if (m_Vertices[i].Position.y > sceneBB.max_corner.y) sceneBB.max_corner.y = m_Vertices[i].Position.y;
				if (m_Vertices[i].Position.z > sceneBB.max_corner.z) sceneBB.max_corner.z = m_Vertices[i].Position.z;
			}
		}
		m_SceneAABB = sceneBB;
	}

	{
		insigne::vbdesc_t desc;
		desc.region_size = SIZE_KB(64);
		desc.stride = sizeof(VertexPNC);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::vb_handle_t newVB = insigne::create_vb(desc);
		insigne::update_vb(newVB, &m_Vertices[0], m_Vertices.get_size(), 0);
		m_VB = newVB;
	}

	{
		insigne::ibdesc_t desc;
		desc.region_size = SIZE_KB(16);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ib_handle_t newIB = insigne::create_ib(desc);
		insigne::update_ib(newIB, &m_Indices[0], m_Indices.get_size(), 0);
		m_IB = newIB;
	}

	{
		// 1024 * 1024
		insigne::framebuffer_desc_t desc = insigne::create_framebuffer_desc();
		desc.width = 1024;
		desc.height = 1024;
		m_ShadowRenderBuffer = insigne::create_framebuffer(desc);
	}

	// camera
	{
		insigne::ubdesc_t desc;
		desc.region_size = SIZE_KB(4);
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ub_handle_t newUB = insigne::create_ub(desc);

		// camera
		m_CamView.position = floral::vec3f(5.0f, 0.5f, 0.0f);
		m_CamView.look_at = floral::vec3f(0.0f, 0.0f, 0.0f);
		m_CamView.up_direction = floral::vec3f(0.0f, 1.0f, 0.0f);

		m_CamProj.near_plane = 0.01f; m_CamProj.far_plane = 100.0f;
		m_CamProj.fov = 60.0f;
		m_CamProj.aspect_ratio = 16.0f / 9.0f;

		m_SceneData.WVP = floral::construct_perspective(m_CamProj) * construct_lookat_point(m_CamView);
		m_SceneData.XForm = floral::mat4x4f(1.0f);
		m_SceneData.CameraPos = floral::vec4f(5.0f, 0.5f, 0.0f, 1.0f);

		insigne::update_ub(newUB, &m_SceneData, sizeof(SceneData), 0);
		m_UB = newUB;
	}

	insigne::dispatch_render_pass();

	// light source
	{
		insigne::ubdesc_t desc;
		desc.region_size = SIZE_KB(4);
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ub_handle_t newUB = insigne::create_ub(desc);

		floral::vec3f d = floral::normalize(floral::vec3f(1.0f, 3.0f, -1.0f));
		m_LightData.Direction = floral::vec4f(d.x, d.y, d.z, 0.0f);
		m_LightData.Color = floral::vec4f(1.0f);
		m_LightData.Radiance = floral::vec4f(50.0f);

		insigne::update_ub(newUB, &m_LightData, sizeof(LightData), 0);
		m_LightDataUB = newUB;
	}

	// light camera
	{
		insigne::ubdesc_t desc;
		desc.region_size = SIZE_KB(4);
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ub_handle_t newUB = insigne::create_ub(desc);

		m_ShadowCamView.position = floral::vec3f(1.0f, 3.0f, -1.0f);
		m_ShadowCamView.look_at = floral::vec3f(0.0f, 0.0f, 0.0f);
		m_ShadowCamView.up_direction = floral::vec3f(0.0f, 1.0f, 0.0f);

		f32 minX = 99.9f, minY = 99.9f;
		f32 maxX = -99.9f, maxY = -99.9f;
		f32 minZ = 99.9f, maxZ = -99.9f;

		{
			floral::inplace_array<floral::vec4f, 8u> bbVertices;
			bbVertices.push_back(floral::vec4f(sceneBB.min_corner.x, sceneBB.min_corner.y, sceneBB.min_corner.z, 1.0f));
			bbVertices.push_back(floral::vec4f(sceneBB.max_corner.x, sceneBB.min_corner.y, sceneBB.min_corner.z, 1.0f));
			bbVertices.push_back(floral::vec4f(sceneBB.max_corner.x, sceneBB.min_corner.y, sceneBB.max_corner.z, 1.0f));
			bbVertices.push_back(floral::vec4f(sceneBB.min_corner.x, sceneBB.min_corner.y, sceneBB.max_corner.z, 1.0f));

			bbVertices.push_back(floral::vec4f(sceneBB.min_corner.x, sceneBB.max_corner.y, sceneBB.min_corner.z, 1.0f));
			bbVertices.push_back(floral::vec4f(sceneBB.max_corner.x, sceneBB.max_corner.y, sceneBB.min_corner.z, 1.0f));
			bbVertices.push_back(floral::vec4f(sceneBB.max_corner.x, sceneBB.max_corner.y, sceneBB.max_corner.z, 1.0f));
			bbVertices.push_back(floral::vec4f(sceneBB.min_corner.x, sceneBB.max_corner.y, sceneBB.max_corner.z, 1.0f));

			floral::mat4x4f v = floral::construct_lookat_point(m_ShadowCamView);
			for (u32 i = 0; i < bbVertices.get_size(); i++) {
				bbVertices[i] = v * bbVertices[i];
				if (bbVertices[i].x > maxX) maxX = bbVertices[i].x;
				if (bbVertices[i].y > maxY) maxY = bbVertices[i].y;
				if (bbVertices[i].z > maxZ) maxZ = bbVertices[i].z;
				if (bbVertices[i].x < minX) minX = bbVertices[i].x;
				if (bbVertices[i].y < minY) minY = bbVertices[i].y;
				if (bbVertices[i].z < minZ) minZ = bbVertices[i].z;
			}
		}

		m_ShadowCamProj.left = minX; m_ShadowCamProj.right = maxX;
		m_ShadowCamProj.top = maxY; m_ShadowCamProj.bottom = minY;
		m_ShadowCamProj.near_plane = 0.01f; m_ShadowCamProj.far_plane = 10.0f;

		m_ShadowSceneData.WVP = floral::construct_orthographic(m_ShadowCamProj) * construct_lookat_point(m_ShadowCamView);
		m_ShadowSceneData.XForm = floral::mat4x4f(1.0f);

		insigne::update_ub(newUB, &m_ShadowSceneData, sizeof(SceneData), 0);
		m_ShadowUB = newUB;
	}

	// shadow shader
	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Scene", insigne::param_data_type_e::param_ub));

		strcpy(desc.vs, s_ShadowVS);
		strcpy(desc.fs, s_ShadowFS);
		desc.vs_path = floral::path("/internal/shadow_vs");
		desc.fs_path = floral::path("/internal/shadow_fs");

		m_ShadowShader = insigne::create_shader(desc);
		insigne::infuse_material(m_ShadowShader, m_ShadowMaterial);

		// static uniform data
		{
			s32 ubSlot = insigne::get_material_uniform_block_slot(m_ShadowMaterial, "ub_Scene");
			m_ShadowMaterial.uniform_blocks[ubSlot].value = m_ShadowUB;
		}
	}

	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Scene", insigne::param_data_type_e::param_ub));
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_LightScene", insigne::param_data_type_e::param_ub));
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Light", insigne::param_data_type_e::param_ub));
		desc.reflection.textures->push_back(insigne::shader_param_t("iu_ShadowMap", insigne::param_data_type_e::param_sampler2d));

		strcpy(desc.vs, s_VertexShader);
		strcpy(desc.fs, s_FragmentShader);
		desc.vs_path = floral::path("/internal/cornel_box_vs");
		desc.fs_path = floral::path("/internal/cornel_box_fs");

		m_Shader = insigne::create_shader(desc);
		insigne::infuse_material(m_Shader, m_Material);

		// static uniform data
		{
			s32 ubSlot = insigne::get_material_uniform_block_slot(m_Material, "ub_Scene");
			m_Material.uniform_blocks[ubSlot].value = m_UB;
		}

		// light uniform data
		{
			s32 ubSlot = insigne::get_material_uniform_block_slot(m_Material, "ub_LightScene");
			m_Material.uniform_blocks[ubSlot].value = m_ShadowUB;
		}

		// light data
		{
			s32 ubSlot = insigne::get_material_uniform_block_slot(m_Material, "ub_Light");
			m_Material.uniform_blocks[ubSlot].value = m_LightDataUB;
		}

		// shadowmap
		{
			insigne::texture_handle_t shadowMapTex = insigne::extract_depth_stencil_attachment(m_ShadowRenderBuffer);
			u32 texSlot = insigne::get_material_texture_slot(m_Material, "iu_ShadowMap");
			m_Material.textures[texSlot].value = shadowMapTex;
		}
	}
	insigne::dispatch_render_pass();

	m_DebugDrawer.Initialize();
}

void GlobalIllumination::OnUpdate(const f32 i_deltaMs)
{
	m_DebugDrawer.BeginFrame();
	m_DebugDrawer.DrawAABB3D(m_SceneAABB, floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f));
	m_DebugDrawer.EndFrame();
}

void GlobalIllumination::OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(m_ShadowRenderBuffer);
	insigne::draw_surface<SurfacePNC>(m_VB, m_IB, m_ShadowMaterial);
	insigne::end_render_pass(m_ShadowRenderBuffer);
	insigne::dispatch_render_pass();

	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::draw_surface<SurfacePNC>(m_VB, m_IB, m_Material);
	m_DebugDrawer.Render(m_SceneData.WVP);
	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void GlobalIllumination::OnCleanUp()
{
}

}
