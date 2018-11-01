#include "SHMath.h"

#include <insigne/commons.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_render.h>
#include <insigne/ut_textures.h>

#include "Graphics/GeometryBuilder.h"

namespace stone {

static const_cstr s_CubeMapVS = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in mediump vec4 l_Color;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_XForm;
	highp mat4 iu_WVP;
};

out mediump vec3 v_Normal;

void main() {
	highp vec4 pos_W = iu_XForm * vec4(l_Position_L, 1.0f);
	v_Normal = normalize(l_Position_L);
	gl_Position = iu_WVP * pos_W;
}
)";

static const_cstr s_CubeMapFS = R"(#version 300 es

layout (location = 0) out mediump vec4 o_Color;

uniform mediump samplerCube u_Tex;
uniform mediump samplerCube u_Tex2;
in mediump vec3 v_Normal;

void main()
{
	mediump vec3 color1 = texture(u_Tex, v_Normal).rgb;
	mediump vec3 color2 = texture(u_Tex2, v_Normal).rgb;
	mediump vec3 outColor = color2;
	o_Color = vec4(outColor, 1.0f);
}
)";

static const_cstr s_SHDiffuseVS = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in highp vec3 l_Normal_L;
layout (location = 2) in mediump vec4 l_Color;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_XForm;
	highp mat4 iu_WVP;
};

out mediump vec3 v_Color;

void main() {
	highp vec4 pos_W = iu_XForm * vec4(l_Position_L, 1.0f);
	v_Color = l_Color.xyz;
	gl_Position = iu_WVP * pos_W;
}
)";

static const_cstr s_SHDiffuseFS = R"(#version 300 es

layout (location = 0) out mediump vec4 o_Color;

in mediump vec3 v_Color;

void main()
{
	o_Color = vec4(v_Color, 1.0f);
}
)";

static const_cstr s_VertexShader = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in mediump vec4 l_Color;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_XForm;
	highp mat4 iu_WVP;
};

out mediump vec3 v_Normal;

void main() {
	highp vec4 pos_W = iu_XForm * vec4(l_Position_L, 1.0f);
	v_Normal = normalize(l_Position_L);
	gl_Position = iu_WVP * pos_W;
}
)";

static const_cstr s_FragmentShader = R"(#version 300 es
layout (location = 0) out mediump vec4 o_Color;

layout(std140) uniform ub_SHData
{
	mediump vec4 iu_Coeffs[9];
};

in mediump vec3 v_Normal;

const mediump float c0 = 0.2820947918f;
const mediump float c1 = 0.4886025119f;
const mediump float c2 = 2.185096861f;	// sqrt(15/pi)
const mediump float c3 = 1.261566261f;	// sqrt(5/pi)

mediump vec3 evalSH(in mediump vec3 i_normal)
{
	return
		c0 * iu_Coeffs[0].xyz					// band 0

		- c1 * i_normal.y * iu_Coeffs[1].xyz * 0.667f
		+ c1 * i_normal.z * iu_Coeffs[2].xyz * 0.667f
		- c1 * i_normal.x * iu_Coeffs[3].xyz * 0.667f

		+ c2 * i_normal.x * i_normal.y * 0.5f * iu_Coeffs[4].xyz * 0.25f
		- c2 * i_normal.y * i_normal.z * 0.5f * iu_Coeffs[5].xyz * 0.25f
		+ c3 * (-1.0f + 3.0f * i_normal.z * i_normal.z) * 0.25f * iu_Coeffs[6].xyz * 0.25f
		- c2 * i_normal.x * i_normal.z * 0.5f * iu_Coeffs[7].xyz * 0.25f
		+ c2 * (i_normal.x * i_normal.x - i_normal.y * i_normal.y) * 0.25f * iu_Coeffs[8].xyz * 0.25f;
}

void main()
{
	mediump vec3 c = evalSH(v_Normal);
	o_Color = vec4(c, 1.0f);
}
)";

SHMath::SHMath()
{
	m_MemoryArena = g_PersistanceResourceAllocator.allocate_arena<LinearArena>(SIZE_MB(48));
}

SHMath::~SHMath()
{
}

void SHMath::OnInitialize()
{
	m_Vertices.init(1024u, &g_StreammingAllocator);
	m_Indices.init(4096u, &g_StreammingAllocator);
	m_SurfVertices.init(512u, &g_StreammingAllocator);
	m_SurfIndices.init(1024u, &g_StreammingAllocator);

	floral::vec3f cubeCoord = floral::texel_coord_to_cube_coord(2, 128, 128, 256);

	{
		{
			floral::mat4x4f m = floral::construct_scaling3d(0.4f, 0.4f, 0.4f);
			GenIcosphere_Tris_PosColor(floral::vec4f(1.0f, 1.0f, 0.0f, 1.0f), m, m_Vertices, m_Indices);
		}
		{
			floral::mat4x4f m = floral::construct_scaling3d(0.3f, 0.3f, 2.0f);
			GenBox_Tris_PosNormalColor(floral::vec4f(1.0f, 1.0f, 1.0f, 1.0f), m, m_SurfVertices, m_SurfIndices);
		}
	}

	// sphere
	{
		insigne::vbdesc_t desc;
		desc.region_size = SIZE_KB(64);
		desc.stride = sizeof(DemoVertex);
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

	// surface
	{
		insigne::vbdesc_t desc;
		desc.region_size = SIZE_KB(64);
		desc.stride = sizeof(VertexPNC);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::vb_handle_t newVB = insigne::create_vb(desc);
		m_SurfVB = newVB;
	}

	{
		insigne::ibdesc_t desc;
		desc.region_size = SIZE_KB(16);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ib_handle_t newIB = insigne::create_ib(desc);
		insigne::update_ib(newIB, &m_SurfIndices[0], m_SurfIndices.get_size(), 0);
		m_SurfIB = newIB;
	}

	{
		insigne::ubdesc_t desc;
		desc.region_size = SIZE_KB(4);
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ub_handle_t newUB = insigne::create_ub(desc);

		// camera
		m_CamView.position = floral::vec3f(7.0f, 0.5f, 0.0f);
		m_CamView.look_at = floral::vec3f(0.0f, 0.0f, 0.0f);
		m_CamView.up_direction = floral::vec3f(0.0f, 1.0f, 0.0f);

		m_CamProj.near_plane = 0.01f; m_CamProj.far_plane = 100.0f;
		m_CamProj.fov = 60.0f;
		m_CamProj.aspect_ratio = 16.0f / 9.0f;

		m_SceneData.WVP = floral::construct_perspective(m_CamProj) * construct_lookat_point(m_CamView);
		m_SceneData.XForm = floral::mat4x4f(1.0f);
		m_DebugWVP = m_SceneData.WVP;

		insigne::update_ub(newUB, &m_SceneData, sizeof(SceneData), 0);
		m_UB = newUB;
	}

	{
		insigne::ubdesc_t desc;
		desc.region_size = SIZE_KB(4);
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ub_handle_t newUB = insigne::create_ub(desc);
		m_SHUB = newUB;

		m_RedSH = ReadSHDataFromFile("gfx/envi/textures/demo/posy_shr.cbsh");
		m_GreenSH = ReadSHDataFromFile("gfx/envi/textures/demo/posy_shg.cbsh");
	}

	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Scene", insigne::param_data_type_e::param_ub));
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_SHData", insigne::param_data_type_e::param_ub));

		strcpy(desc.vs, s_VertexShader);
		strcpy(desc.fs, s_FragmentShader);
		desc.vs_path = floral::path("/internal/sh_probe");
		desc.fs_path = floral::path("/internal/sh_probe");

		m_Shader = insigne::create_shader(desc);
		insigne::infuse_material(m_Shader, m_Material);

		// static uniform data
		{
			s32 ubSlot = insigne::get_material_uniform_block_slot(m_Material, "ub_Scene");
			m_Material.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 0, 0, m_UB };
		}

		{
			s32 ubSlot = insigne::get_material_uniform_block_slot(m_Material, "ub_SHData");
			m_Material.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 0, 0, m_SHUB };
		}
	}

	// cubemap
	{
		floral::file_info texFile = floral::open_file("gfx/envi/textures/demo/monvalley.cbskb");
		floral::file_stream dataStream;

		dataStream.buffer = (p8)m_MemoryArena->allocate(texFile.file_size);
		floral::read_all_file(texFile, dataStream);
		floral::close_file(texFile);

		c8 magicChars[4];
		dataStream.read_bytes(magicChars, 4);

		s32 colorRange = 0;
		s32 colorSpace = 0;
		s32 colorChannel = 0;
		f32 encodeGamma = 0.0f;
		s32 mipsCount = 0;
		dataStream.read<s32>(&colorRange);
		dataStream.read<s32>(&colorSpace);
		dataStream.read<s32>(&colorChannel);
		dataStream.read<f32>(&encodeGamma);
		dataStream.read<s32>(&mipsCount);

		insigne::texture_desc_t demoTexDesc;
		demoTexDesc.width = 512;
		demoTexDesc.height = 512;
		demoTexDesc.format = insigne::texture_format_e::hdr_rgb;
		demoTexDesc.min_filter = insigne::filtering_e::linear_mipmap_linear;
		demoTexDesc.mag_filter = insigne::filtering_e::linear;
		demoTexDesc.dimension = insigne::texture_dimension_e::tex_cube;
		demoTexDesc.has_mipmap = true;
		const size dataSize = insigne::prepare_texture_desc(demoTexDesc);
		p8 pData = (p8)demoTexDesc.data;
		// > This is where it get *really* interesting
		// 	Totally opposite of normal 2D texture mapping, CubeMapping define the origin of the texture sampling coordinate
		// 	from the lower left corner. OmegaLUL
		// > Reason: historical reason (from Renderman)
		dataStream.read_bytes((p8)demoTexDesc.data, dataSize);

		m_Texture = insigne::create_texture(demoTexDesc);

		m_MemoryArena->free_all();
	}

	insigne::dispatch_render_pass();

	{
		floral::file_info texFile = floral::open_file("gfx/envi/textures/demo/cubeuvchecker.cbskb");
		floral::file_stream dataStream;

		dataStream.buffer = (p8)m_MemoryArena->allocate(texFile.file_size);
		floral::read_all_file(texFile, dataStream);
		floral::close_file(texFile);

		c8 magicChars[4];
		dataStream.read_bytes(magicChars, 4);

		s32 colorRange = 0;
		s32 colorSpace = 0;
		s32 colorChannel = 0;
		f32 encodeGamma = 0.0f;
		s32 mipsCount = 0;
		dataStream.read<s32>(&colorRange);
		dataStream.read<s32>(&colorSpace);
		dataStream.read<s32>(&colorChannel);
		dataStream.read<f32>(&encodeGamma);
		dataStream.read<s32>(&mipsCount);

		insigne::texture_desc_t demoTexDesc;
		demoTexDesc.width = 512;
		demoTexDesc.height = 512;
		demoTexDesc.format = insigne::texture_format_e::hdr_rgb;
		demoTexDesc.min_filter = insigne::filtering_e::linear_mipmap_linear;
		demoTexDesc.mag_filter = insigne::filtering_e::linear;
		demoTexDesc.dimension = insigne::texture_dimension_e::tex_cube;
		demoTexDesc.has_mipmap = true;
		const size dataSize = insigne::prepare_texture_desc(demoTexDesc);
		p8 pData = (p8)demoTexDesc.data;
		// > This is where it get *really* interesting
		// 	Totally opposite of normal 2D texture mapping, CubeMapping define the origin of the texture sampling coordinate
		// 	from the lower left corner. OmegaLUL
		// > Reason: historical reason (from Renderman)
		dataStream.read_bytes((p8)demoTexDesc.data, dataSize);

		m_Texture2 = insigne::create_texture(demoTexDesc);

		m_MemoryArena->free_all();
	}

	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Scene", insigne::param_data_type_e::param_ub));
		desc.reflection.textures->push_back(insigne::shader_param_t("u_Tex", insigne::param_data_type_e::param_sampler_cube));
		desc.reflection.textures->push_back(insigne::shader_param_t("u_Tex2", insigne::param_data_type_e::param_sampler_cube));

		strcpy(desc.vs, s_CubeMapVS);
		strcpy(desc.fs, s_CubeMapFS);

		m_CubeShader = insigne::create_shader(desc);
		insigne::infuse_material(m_CubeShader, m_CubeMaterial);

		{
			s32 ubSlot = insigne::get_material_uniform_block_slot(m_CubeMaterial, "ub_Scene");
			m_CubeMaterial.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 0, 0, m_UB };
		}
		{
			s32 texSlot = insigne::get_material_texture_slot(m_CubeMaterial, "u_Tex");
			m_CubeMaterial.textures[texSlot].value = m_Texture;
		}
		{
			s32 texSlot = insigne::get_material_texture_slot(m_CubeMaterial, "u_Tex2");
			m_CubeMaterial.textures[texSlot].value = m_Texture2;
		}
	}

	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Scene", insigne::param_data_type_e::param_ub));

		strcpy(desc.vs, s_SHDiffuseVS);
		strcpy(desc.fs, s_SHDiffuseFS);

		m_SurfShader = insigne::create_shader(desc);
		insigne::infuse_material(m_SurfShader, m_SurfMaterial);

		{
			s32 ubSlot = insigne::get_material_uniform_block_slot(m_CubeMaterial, "ub_Scene");
			m_SurfMaterial.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 0, 0, m_UB };
		}
	}

	// eval sh
	{
		for (u32 i = 0; i < m_SurfVertices.get_size(); i++) {
			VertexPNC& vtx = m_SurfVertices[i];
			SHData shData = LinearInterpolate(m_GreenSH, m_RedSH, vtx.Position.z / 4.0f + 0.5f);
			floral::vec3f shColor = EvalSH(shData, vtx.Normal);
			vtx.Color = floral::vec4f(shColor.x, shColor.y, shColor.z, 1.0f);
		}
	}
	insigne::update_vb(m_SurfVB, &m_SurfVertices[0], m_SurfVertices.get_size(), 0);

	m_DebugDrawer.Initialize();
}

void SHMath::OnUpdate(const f32 i_deltaMs)
{
	m_DebugDrawer.BeginFrame();
	m_DebugDrawer.EndFrame();
}

void SHMath::OnRender(const f32 i_deltaMs)
{
	static f32 elapsedTime = 0.0f;
	elapsedTime += i_deltaMs;
	f32 xPos = 2.0f * sinf(floral::to_radians(elapsedTime / 10.0f));

	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	{
		SceneData sceneData;
		sceneData.XForm = floral::construct_translation3d(0.0f, 1.5f, -2.0f) * m_CameraMotion.GetRotation().normalize().to_transform();
		sceneData.WVP = m_SceneData.WVP;
		insigne::copy_update_ub(m_UB, &sceneData, sizeof(SceneData), 0);
		insigne::copy_update_ub(m_SHUB, &m_RedSH, sizeof(SHData), 0);
		insigne::draw_surface<DemoSurface>(m_VB, m_IB, m_Material);
		insigne::dispatch_render_pass();
	}

	{
		SceneData sceneData;
		sceneData.XForm = floral::construct_translation3d(0.0f, 1.5f, 2.0f) * m_CameraMotion.GetRotation().normalize().to_transform();
		sceneData.WVP = m_SceneData.WVP;
		insigne::copy_update_ub(m_UB, &sceneData, sizeof(SceneData), 0);
		insigne::copy_update_ub(m_SHUB, &m_GreenSH, sizeof(SHData), 0);
		insigne::draw_surface<DemoSurface>(m_VB, m_IB, m_Material);
		insigne::dispatch_render_pass();
	}

	{
		SceneData sceneData;
		sceneData.XForm = floral::construct_translation3d(0.0f, 0.5f, xPos) * m_CameraMotion.GetRotation().normalize().to_transform();
		sceneData.WVP = m_SceneData.WVP;
		insigne::copy_update_ub(m_UB, &sceneData, sizeof(SceneData), 0);
		m_AvgSH = LinearInterpolate(m_GreenSH, m_RedSH, xPos / 4.0f + 0.5f);
		insigne::copy_update_ub(m_SHUB, &m_AvgSH, sizeof(SHData), 0);
		insigne::draw_surface<DemoSurface>(m_VB, m_IB, m_Material);
		insigne::dispatch_render_pass();
	}

	{
		SceneData sceneData;
		sceneData.XForm = floral::construct_translation3d(0.0f, -0.5f, 0.0f) * m_CameraMotion.GetRotation().normalize().to_transform();
		sceneData.WVP = m_SceneData.WVP;
		insigne::copy_update_ub(m_UB, &sceneData, sizeof(SceneData), 0);
		insigne::draw_surface<SurfacePNC>(m_SurfVB, m_SurfIB, m_SurfMaterial);
		insigne::dispatch_render_pass();
	}
	m_DebugDrawer.Render(m_DebugWVP);
	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void SHMath::OnCleanUp()
{
}

// ---------------------------------------------
SHMath::SHData SHMath::ReadSHDataFromFile(const_cstr i_filePath)
{
	floral::file_info shFile = floral::open_file(i_filePath);
	floral::file_stream dataStream;

	dataStream.buffer = (p8)m_MemoryArena->allocate(shFile.file_size);
	floral::read_all_file(shFile, dataStream);
	floral::close_file(shFile);

	floral::inplace_array<SHData, 27u> shDatas;

	SHData shData;
	for (u32 j = 0; j < 9; j++) {
		floral::vec3f shBand;
		dataStream.read_bytes((p8)&shBand, sizeof(floral::vec3f));
		shData.CoEffs[j] = floral::vec4f(shBand.x, shBand.y, shBand.z, 0.0f);
	}

	m_MemoryArena->free_all();

	return shData;
}

SHMath::SHData SHMath::LinearInterpolate(const SHData& d0, const SHData& d1, const f32 weight)
{
	SHData d;
	for (u32 i = 0; i < 9; i++) {
		d.CoEffs[i] = d0.CoEffs[i] * weight + d1.CoEffs[i] * (1.0f - weight);
	}
	return d;
}

floral::vec3f SHMath::EvalSH(const SHData& i_shData, const floral::vec3f& i_normal)
{
	const f32 c0 = 0.2820947918f;
	const f32 c1 = 0.4886025119f;
	const f32 c2 = 2.185096861f;	// sqrt(15/pi)
	const f32 c3 = 1.261566261f;	// sqrt(5/pi)

	floral::vec4f color =
		c0 * i_shData.CoEffs[0]

		- c1 * i_normal.y * i_shData.CoEffs[1] * 0.667f
		+ c1 * i_normal.z * i_shData.CoEffs[2] * 0.667f
		- c1 * i_normal.x * i_shData.CoEffs[3] * 0.667f

		+ c2 * i_normal.x * i_normal.y * 0.5f * i_shData.CoEffs[4] * 0.25f
		- c2 * i_normal.y * i_normal.z * 0.5f * i_shData.CoEffs[5] * 0.25f
		+ c3 * (-1.0f + 3.0f * i_normal.z * i_normal.z) * 0.25f * i_shData.CoEffs[6] * 0.25f
		- c2 * i_normal.x * i_normal.z * 0.5f * i_shData.CoEffs[7] * 0.25f
		+ c2 * (i_normal.x * i_normal.x - i_normal.y * i_normal.y) * 0.25f * i_shData.CoEffs[8] * 0.25f;
	return floral::vec3f(color.x, color.y, color.z);
}

}
