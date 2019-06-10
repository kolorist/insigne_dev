#include "UnshadowedPRT.h"

#include <clover/Logger.h>

#include <insigne/commons.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_render.h>
#include <insigne/ut_textures.h>

#include "Graphics/prt.h"
#include "Graphics/GeometryBuilder.h"
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

#if 0
	//vColor = l_SH0 * iu_LightSH[0].rgb;
	vColor += l_SH1 * iu_LightSH[1].rgb;
	//vColor += l_SH2 * iu_LightSH[2].rgb;
	//vColor += l_SH3 * iu_LightSH[3].rgb;
	//vColor += l_SH4 * iu_LightSH[4].rgb;
	//vColor += l_SH5 * iu_LightSH[5].rgb;
	//vColor += l_SH6 * iu_LightSH[6].rgb;
	//vColor += l_SH7 * iu_LightSH[7].rgb;
	vColor += l_SH8 * iu_LightSH[8].rgb;
#else
	vColor = l_SH0 * iu_LightSH[0].rgb;
	vColor += l_SH1 * iu_LightSH[1].rgb;
	vColor += l_SH2 * iu_LightSH[2].rgb;
	vColor += l_SH3 * iu_LightSH[3].rgb;
	vColor += l_SH4 * iu_LightSH[4].rgb;
	vColor += l_SH5 * iu_LightSH[5].rgb;
	vColor += l_SH6 * iu_LightSH[6].rgb;
	vColor += l_SH7 * iu_LightSH[7].rgb;
	vColor += l_SH8 * iu_LightSH[8].rgb;
#endif

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
	mediump float exposure = 0.4f;
	mediump float gamma = 2.2f;
	mediump vec4 hdrColor = texture(u_Tex, v_TexCoord);

	mediump vec3 mapped = vec3(1.0f) - exp(-hdrColor.rgb * exposure);
	mapped = pow(mapped, vec3(1.0f / gamma));

	o_Color = vec4(mapped, hdrColor.a);
}
)";

UnshadowedPRT::UnshadowedPRT()
	: m_CameraMotion(
			floral::camera_view_t { floral::vec3f(0.0f, 0.3f, 3.0f), floral::vec3f(0.0f, -0.3f, -3.0f), floral::vec3f(0.0f, 1.0f, 0.0f) },
			floral::camera_persp_t { 0.01f, 100.0f, 60.0f, 16.0f / 9.0f })
{
	m_MemoryArena = g_PersistanceResourceAllocator.allocate_arena<LinearArena>(SIZE_MB(16));
}

UnshadowedPRT::~UnshadowedPRT()
{
}

void UnshadowedPRT::OnInitialize()
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

	m_Vertices.init(2048u, &g_StreammingAllocator);
	m_SHVertices.init(2048u, &g_StreammingAllocator);
	m_Indices.init(8192u, &g_StreammingAllocator);

	if (0)
	{
		floral::mat4x4f mIco = floral::construct_scaling3d(0.3f, 0.3f, 0.3f);
		GenIcosphere_Tris_PNC(mIco, floral::vec4f(1.0f), m_Vertices, m_Indices);
	}
	else
	{
		floral::mat4x4f mBottom = floral::construct_translation3d(0.0f, -1.0f, 0.0f);
		floral::mat4x4f mLeft = floral::construct_translation3d(0.0f, 0.0f, 1.5f)
			* floral::construct_quaternion_euler(-90.0f, 0.0f, 0.0f).to_transform();
		floral::mat4x4f mRight = floral::construct_translation3d(0.0f, 0.0f, -1.5f)
			* floral::construct_quaternion_euler(90.0f, 0.0f, 0.0f).to_transform();
		floral::mat4x4f mBack = floral::construct_translation3d(-1.0f, 0.0f, 0.0f)
			* floral::construct_quaternion_euler(0.0f, 0.0f, -90.0f).to_transform();

		GenTesselated3DPlane_Tris_PNC(
				mBottom,
				2.0f, 3.0f, 0.3f, floral::vec4f(1.0f),
				m_Vertices, m_Indices);
		GenTesselated3DPlane_Tris_PNC(
				mLeft,
				2.0f, 2.0f, 0.3f, floral::vec4f(1.0f),
				m_Vertices, m_Indices);
		GenTesselated3DPlane_Tris_PNC(
				mRight,
				2.0f, 2.0f, 0.3f, floral::vec4f(1.0f),
				m_Vertices, m_Indices);
		GenTesselated3DPlane_Tris_PNC(
				mBack,
				2.0f, 3.0f, 0.3f, floral::vec4f(1.0f),
				m_Vertices, m_Indices);

		floral::mat4x4f mB1Top = floral::construct_translation3d(0.0f, 0.4f, 0.0f)
			* floral::construct_quaternion_euler(0.0f, 43.0f, 0.0f).to_transform();
		floral::mat4x4f mB1Front = floral::construct_quaternion_euler(0.0f, 43.0f, 0.0f).to_transform()
			* floral::construct_translation3d(0.35f, -0.3f, 0.0f)
			* floral::construct_quaternion_euler(0.0f, 0.0f, -90.0f).to_transform();
		floral::mat4x4f mB1Back = floral::construct_quaternion_euler(0.0f, 43.0f, 0.0f).to_transform()
			* floral::construct_translation3d(-0.35f, -0.3f, 0.0f)
			* floral::construct_quaternion_euler(0.0f, 0.0f, 90.0f).to_transform();
		floral::mat4x4f mB1Left = floral::construct_quaternion_euler(0.0f, -47.0f, 0.0f).to_transform()
			* floral::construct_translation3d(0.35f, -0.3f, 0.0f)
			* floral::construct_quaternion_euler(0.0f, 0.0f, -90.0f).to_transform();
		floral::mat4x4f mB1Right = floral::construct_quaternion_euler(0.0f, -47.0f, 0.0f).to_transform()
			* floral::construct_translation3d(-0.35f, -0.3f, 0.0f)
			* floral::construct_quaternion_euler(0.0f, 0.0f, 90.0f).to_transform();
		GenTesselated3DPlane_Tris_PNC(
				mB1Top,
				0.7f, 0.7f, 0.3f, floral::vec4f(1.0f),
				m_Vertices, m_Indices);
		GenTesselated3DPlane_Tris_PNC(
				mB1Front,
				1.4f, 0.7f, 0.3f, floral::vec4f(1.0f),
				m_Vertices, m_Indices);
		GenTesselated3DPlane_Tris_PNC(
				mB1Back,
				1.4f, 0.7f, 0.3f, floral::vec4f(1.0f),
				m_Vertices, m_Indices);
		GenTesselated3DPlane_Tris_PNC(
				mB1Left,
				1.4f, 0.7f, 0.3f, floral::vec4f(1.0f),
				m_Vertices, m_Indices);
		GenTesselated3DPlane_Tris_PNC(
				mB1Right,
				1.4f, 0.7f, 0.3f, floral::vec4f(1.0f),
				m_Vertices, m_Indices);
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
		insigne::update_vb(newVB, &m_SHVertices[0], m_SHVertices.get_size(), 0);
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

void UnshadowedPRT::OnUpdate(const f32 i_deltaMs)
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

void UnshadowedPRT::OnDebugUIUpdate(const f32 i_deltaMs)
{
}

void UnshadowedPRT::OnRender(const f32 i_deltaMs)
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

void UnshadowedPRT::OnCleanUp()
{
}

//----------------------------------------------

void UnshadowedPRT::ComputeLightSH()
{
	m_MemoryArena->free_all();
	floral::file_info texFile = floral::open_file("gfx/envi/textures/demo/grace_probe.cbtex");
	//floral::file_info texFile = floral::open_file("gfx/envi/textures/demo/Alexs_Apt_2k_lightprobe.cbtex");
	//floral::file_info texFile = floral::open_file("gfx/envi/textures/demo/ArboretumInBloom_lightprobe.cbtex");
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

void UnshadowedPRT::ComputePRT()
{
	s32 sqrtNSamples = 100;
	s32 NSamples = sqrtNSamples * sqrtNSamples;
	sh_sample* samples = m_MemoryArena->allocate_array<sh_sample>(NSamples);
	sh_setup_spherical_samples(samples, sqrtNSamples);

	highp_vec3_t coeffs[9];

	for (u32 i = 0; i < m_Vertices.get_size(); i++)
	{
		memset(coeffs, 0, sizeof(coeffs));
		for (s32 j = 0; j < NSamples; j++)
		{
			const sh_sample& sample = samples[j];
			highp_vec3_t normal = highp_vec3_t(m_Vertices[i].Normal.x, m_Vertices[i].Normal.y, m_Vertices[i].Normal.z);
			f64 cosineTerm = floral::dot(normal, sample.vec);
			if (cosineTerm > 0.0)
			{
				const floral::vec4f& vtxColor = m_Vertices[i].Color;
				for (u32 k = 0; k < 9; k++)
				{
					highp_vec3_t color(vtxColor.x, vtxColor.y, vtxColor.z); // highp computation
					f64 shCoeff = sample.coeff[k];
					coeffs[k] += color * shCoeff * cosineTerm;
				}
			}
		}

		f64 weight = 4.0 * 3.141593;
		f64 scale = weight / NSamples;
		VertexPNCSH vtf;
		vtf.Position = m_Vertices[i].Position;
		vtf.Normal = m_Vertices[i].Normal;
		vtf.Color = m_Vertices[i].Color;
		for (u32 j = 0; j < 9; j++)
		{
			coeffs[j] *= scale;
			vtf.SH[j] = floral::vec3f((f32)coeffs[j].x, (f32)coeffs[j].y, (f32)coeffs[j].z);
		}
		m_SHVertices.push_back(vtf);
	}
}

}
